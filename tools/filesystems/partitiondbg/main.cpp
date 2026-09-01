/***************************************************************************
 *   Copyright (C) 2026 by Radu Raul                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
#include <cstdio>

#include "interfaces/bsp.h"
#include "miosix.h"
#include "drivers/sdmmc/stm32f2_f4_f7_sd.h"
#include "MBR/mbr.h"
#include "GPT/gpt.h"

using namespace miosix;

void tryMBR() 
{
    iprintf("Trying MBR\n");
    auto mbrReaderResult=MBR::MBRReader::readMBR(SDIODriver::instance());
    size_t retryCount=0;
    while(!mbrReaderResult && retryCount<5) 
    {
        iprintf("Error while reading from device\n");
        mbrReaderResult=MBR::MBRReader::readMBR(SDIODriver::instance());
        retryCount++;
        Thread::sleep(1);
    }

    if (!mbrReaderResult) 
    {
        iprintf("Excedeed the retry count while reading the device.\n");
        return;
    }

    auto mbrReader=*mbrReaderResult;

    if (!mbrReader.isValidMBR()) 
    {
        iprintf("Invalid MBR. Expected: 0xAA55, Got: 0x%04X\n", mbrReader.mbrSignature());
        return;
    }
    
    mbrReader.printMBRInfo();
    iprintf("Finished MBR\n");
}

void tryGPT() 
{
    iprintf("Trying GPT\n");
    auto gptReaderResult = GPT::GPTReader::readGPT(SDIODriver::instance());
    size_t retryCount=0;
    while(!gptReaderResult && retryCount<5) 
    {
        iprintf("Error while reading from device\n");
        gptReaderResult=GPT::GPTReader::readGPT(SDIODriver::instance());
        retryCount++;
        Thread::sleep(1);
    }

    if (!gptReaderResult) 
    {
        iprintf("Excedeed the retry count while reading the device.\n");
        return;
    }

    auto gptReader=std::move(*gptReaderResult);

    auto checkResult=gptReader.checkGPT();
    if (checkResult != GPT::ReaderResult::Ok) 
    {
        iprintf("Error checking GPT, reasonID: %d\n", static_cast<int>(checkResult));
        return;
    }

    gptReader.printGPTInfo();
}

int main()
{
    iprintf("Starting partitiondbg\n");
    iprintf("Reading SDIO device size...\n");

    off_t cardSize=0;
    SDIODriver::instance()->ioctl(IOCTL_GET_VOLUME_SIZE, &cardSize);

    iprintf("SDIO device size: %llu bytes\n", cardSize);
    
    tryMBR();
    tryGPT();

    for(;;) 
    {
        ledOn();
        Thread::sleep(500);
        ledOff();
        Thread::sleep(500);
    }
}
