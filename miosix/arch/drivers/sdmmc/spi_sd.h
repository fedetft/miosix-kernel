/***************************************************************************
 *   Copyright (C) 2014 by Terraneo Federico                               *
 *   Copyright (C) 2025 by Daniele Cattaneo                                *
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
/*
 * Integration in Miosix by Terraneo Federico.
 * Based on code by Martin Thomas to initialize SD cards from LPC2000
 */

#pragma once

#include <cstdarg>
#include <memory>
#include "kernel/sync.h"
#include "filesystem/devfs/devfs.h"
#include "filesystem/ioctl.h"

namespace miosix {

/**
 * Driver for interfacing to an SD card through SPI
 */
template <class SPI>
class SPISD: public Device
{
public:
    // Constructor
    SPISD(std::unique_ptr<SPI> spi, GpioPin cs);
    
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    virtual int ioctl(int cmd, void *arg);
private:
    //Note: enabling debugging might cause deadlock when using sleep() or reboot()
    //The bug won't be fixed because debugging is only useful for driver development

    ///\internal When set to 1, enables error messages.
    ///When set to 2, enables debug messages as well.
    static const int dbgLevel=0;

    /**
     * \internal Debug print, for normal conditions
     */
    static inline void dbg(const char *fmt, ...) noexcept
    {
        if(dbgLevel<2) return;
        va_list arg;
        va_start(arg,fmt);
        viprintf(fmt,arg);
        va_end(arg);
    }

    /**
     * \internal Debug print, for errors only
     */
    static inline void dbgerr(const char *fmt, ...) noexcept
    {
        if(dbgLevel<1) return;
        va_list arg;
        va_start(arg,fmt);
        viprintf(fmt,arg);
        va_end(arg);
    }

    /**
     * \internal
     * Used for debugging, print 8 bit error code from SD card
     */
    static void printErrorCode(unsigned char value) noexcept;

    /**
     * \internal
     * Return 1 if card is OK, otherwise print 16 bit error code from SD card
     */
    char readSdStatus() noexcept;

    /**
     * \internal
     * Wait until card is ready
     */
    unsigned char waitForCardReady() noexcept;

    /**
     * \internal
     * Send a command to the SD card
     * \param cmd one among the #define'd commands
     * \param arg command's argument
     * \return command's r1 response. If command returns a longer response, the user
     * can continue reading the response with spi->sendRecv(0xff)
     */
    unsigned char sendCmd(unsigned char cmd, unsigned int arg) noexcept;

    /**
     * \internal
     * Receive a data packet from the SD card
     * \param buf data buffer to store received data
     * \param byte count (must be multiple of 4)
     * \return true on success, false on failure
     */
    bool readDataPacket(unsigned char *buf, unsigned int btr) noexcept;

    /**
     * \internal
     * Send a data packet to the SD card
     * \param buf 512 byte data block to be transmitted
     * \param token data start/stop token
     * \return true on success, false on failure
     */
    bool sendDataPacket(const unsigned char *buf, unsigned char token) noexcept;

    /*
     * Definitions for MMC/SDC command.
     * A command has the following format:
     * - 1  bit @ 0 (start bit)
     * - 1  bit @ 1 (transmission bit)
     * - 6  bit which identify command index (CMD0..CMD63)
     * - 32 bit command argument
     * - 7 bit CRC
     * - 1 bit @ 1 (end bit)
     * In addition, ACMDxx are the sequence of two commands, CMD55 and CMDxx
     * These constants have the following meaninig:
     * - bit #7 @ 1 indicates that it is an ACMD. sendCmd() will send CMD55, then
     *   clear this bit and send the command with this bit @ 0 (which is start bit)
     * - bit #6 always @ 1, because it is the transmission bit
     * - remaining 6 bit represent command index
     */
    unsigned char CMD0 = (0x40 + 0);    // GO_IDLE_STATE
    unsigned char CMD1 = (0x40 + 1);    // SEND_OP_COND (MMC)
    unsigned char ACMD41 = (0xC0 + 41); // SEND_OP_COND (SDC)
    unsigned char CMD8 = (0x40 + 8);    // SEND_IF_COND
    unsigned char CMD9 = (0x40 + 9);    // SEND_CSD
    unsigned char CMD10 = (0x40 + 10);  // SEND_CID
    unsigned char CMD12 = (0x40 + 12);  // STOP_TRANSMISSION
    unsigned char CMD13 = (0x40 + 13);  // SEND_STATUS
    unsigned char ACMD13 = (0xC0 + 13); // SD_STATUS (SDC)
    unsigned char CMD16 = (0x40 + 16);  // SET_BLOCKLEN
    unsigned char CMD17 = (0x40 + 17);  // READ_SINGLE_BLOCK
    unsigned char CMD18 = (0x40 + 18);  // READ_MULTIPLE_BLOCK
    unsigned char CMD23 = (0x40 + 23);  // SET_BLOCK_COUNT (MMC)
    unsigned char ACMD23 = (0xC0 + 23); // SET_WR_BLK_ERASE_COUNT (SDC)
    unsigned char CMD24 = (0x40 + 24);  // WRITE_BLOCK
    unsigned char CMD25 = (0x40 + 25);  // WRITE_MULTIPLE_BLOCK
    unsigned char CMD42 = (0x40 + 42);  // LOCK_UNLOCK
    unsigned char CMD55 = (0x40 + 55);  // APP_CMD
    unsigned char CMD58 = (0x40 + 58);  // READ_OCR

    std::unique_ptr<SPI> spi;
    GpioPin cs;
    KernelMutex mutex;
    ///\internal Type of card
    /// - 0=no card
    /// - (1<<0)=MMC
    /// - (1<<1)=SDv1
    /// - (1<<2)=SDv2
    /// - (1<<2)|(1<<3)=SDHC
    unsigned char cardType=0;
};

template <class SPI>
void SPISD<SPI>::printErrorCode(unsigned char value) noexcept
{
    switch(value)
    {
        case 0x40:
            dbgerr("Parameter error\n");
            break;
        case 0x20:
            dbgerr("Address error\n");
            break;
        case 0x10:
            dbgerr("Erase sequence error\n");
            break;
        case 0x08:
            dbgerr("CRC error\n");
            break;
        case 0x04:
            dbgerr("Illegal command\n");
            break;
        case 0x02:
            dbgerr("Erase reset\n");
            break;
        case 0x01:
            dbgerr("Card is initializing\n");
            break;
        default:
            dbgerr("Unknown error 0x%x\n",value);
            break;
    }
}

template <class SPI>
char SPISD<SPI>::readSdStatus() noexcept
{
    short value=sendCmd(CMD13,0);
    value<<=8;
    value|=spi->sendRecv(0xff);

    switch(value)
    {
        case 0x0000:
            return 1;
        case 0x0001:
            dbgerr("Card is Locked\n");
            /*//Try to fix the problem by erasing all the SD card.
            char e=sendCmd(CMD16,1);
            if(e!=0) printErrorCode(e);
            e=sendCmd(CMD42,0);
            if(e!=0) printErrorCode(e);
            spi->sendRecv(0xfe); // Start block
            spi->sendRecv(1<<3); //Erase all disk command
            spi->sendRecv(0xff); // Checksum part 1
            spi->sendRecv(0xff); // Checksum part 2
            e=spi->sendRecv(0xff);
            iprintf("Reached here 0x%x\n",e);//Should return 0xe5
            while(spi->sendRecv(0xff)!=0xff);*/
            break;
        case 0x0002:
            dbgerr("WP erase skip or lock/unlock cmd failed\n");
            break;
        case 0x0004:
            dbgerr("General error\n");
            break;
        case 0x0008:
            dbgerr("Internal card controller error\n");
            break;
        case 0x0010:
            dbgerr("Card ECC failed\n");
            break;
        case 0x0020:
            dbgerr("Write protect violation\n");
            break;
        case 0x0040:
            dbgerr("Invalid selection for erase\n");
            break;
        case 0x0080:
            dbgerr("Out of range or CSD overwrite\n");
            break;
        default:
            if(value>0x00FF)
                printErrorCode((unsigned char)(value>>8));
            else
                dbgerr("Unknown error 0x%x\n",value);
            break;
    }
    return -1;
}

template <class SPI>
unsigned char SPISD<SPI>::waitForCardReady() noexcept
{
    unsigned char result;
    // Backoff of 10us; do not use Thread::sleep, too few cycles
    for(int i=0;i<10;i++)
    {
        result=spi->sendRecv(0xff);
        if(result==0xff) return 0xff;
        delayUs(10);
    }
    long long t=0, backoff=10*1000;
    while(t<=500*1000*1000) // Timeout 500ms
    {
        result=spi->sendRecv(0xff);
        if(result==0xff) return 0xff;
        if(result!=0) { delayUs(10); continue; }
        // exponential backoff
        Thread::nanoSleep(backoff);
        t+=backoff;
        backoff=backoff+backoff/16; // backoff*=1.0625;
    }
    dbgerr("Error: waitForCardReady() timeout\n");
    return 0;
}

template <class SPI>
unsigned char SPISD<SPI>::sendCmd(unsigned char cmd, unsigned int arg) noexcept
{
    unsigned char n, res;
    if(cmd & 0x80)
    {	// ACMD<n> is the command sequence of CMD55-CMD<n>
        cmd&=0x7f;
        res=sendCmd(CMD55,0);
        if(res>1) return res;
    }

    // Select the card and wait for ready
    cs.high();
    cs.low();

    if(cmd==CMD0)
    {
        //waitForCardReady on CMD0 seems to cause infinite loop
        spi->sendRecv(0xff);
    } else {
        if(waitForCardReady()!=0xff) return 0xff;
    }
    // Send command
    unsigned char buf[6];
    buf[0]=cmd;                         // Start + Command index
    buf[1]=(unsigned char)(arg >> 24);  // Argument[31..24]
    buf[2]=(unsigned char)(arg >> 16);  // Argument[23..16]
    buf[3]=(unsigned char)(arg >> 8);   // Argument[15..8]
    buf[4]=(unsigned char)arg;          // Argument[7..0]
    n=0x01;                 // Dummy CRC + Stop
    if (cmd==CMD0) n=0x95;  // Valid CRC for CMD0(0)
    if (cmd==CMD8) n=0x87;  // Valid CRC for CMD8(0x1AA)
    buf[5]=n;
    spi->send(buf,6);
    // Receive response
    if (cmd==CMD12) spi->sendRecv(0xff); // Skip a stuff byte when stop reading
    n=10; // Wait response, try 10 times
    do
        res=spi->sendRecv(0xff);
    while ((res & 0x80) && --n);
    return res; // Return with the response value
}

template <class SPI>
bool SPISD<SPI>::readDataPacket(unsigned char *buf, unsigned int btr) noexcept
{
    unsigned char token;
    for(int i=0;i<0xffff;i++)
    {
        token=spi->sendRecv(0xff);
        if(token!=0xff) break;
    }
    if(token!=0xfe) return false; // If not valid data token, retutn error

    spi->recv(buf,btr);
    spi->sendRecv(0xff); // Discard CRC
    spi->sendRecv(0xff);

    return true; // Return success
}

template <class SPI>
bool SPISD<SPI>::sendDataPacket(const unsigned char *buf, unsigned char token) noexcept
{
    unsigned char resp;
    if(waitForCardReady()!=0xff) return false;

    spi->sendRecv(token); // Xmit data token
    if (token!=0xfd)
    {   // Is data token
        spi->send(buf,512);
        spi->sendRecv(0xff); // CRC (Dummy)
        spi->sendRecv(0xff);
        resp=spi->sendRecv(0xff); // Receive data response
        if((resp & 0x1f)!=0x05) // If not accepted, return error
        return false;
    }
    return true;
}

template <class SPI>
ssize_t SPISD<SPI>::readBlock(void* buffer, size_t size, off_t where)
{
    if(cardType==0) return -EIO;
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    unsigned char *buf=reinterpret_cast<unsigned char*>(buffer);
    Lock<KernelMutex> l(mutex);
    dbg("%s:%d: nSectors=%d\n",__FILE__,__LINE__,nSectors);
    if(!(cardType & 8)) lba*=512; // Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {   // Single block read
        result=sendCmd(CMD17,lba); // READ_SINGLE_BLOCK
        if(result!=0)
        {
            printErrorCode(result);
            cs.high();
            return -EBADF; // TODO: we always return EBADF, shouldn't it be EIO?
        }
        if(readDataPacket(buf,512)==false)
        {
            dbgerr("Block read error\n");
            cs.high();
            return -EBADF;
        }
    } else { // Multiple block read
        //dbgerr("Mbr\n");//debug only
        result=sendCmd(CMD18,lba); // READ_MULTIPLE_BLOCK
        if(result!=0)
        {
            printErrorCode(result);
            cs.high();
            return -EBADF;
        }
        do {
            if(!readDataPacket(buf,512)) break;
            buf+=512;
        } while(--nSectors);
        sendCmd(CMD12,0); // STOP_TRANSMISSION
        if(nSectors!=0)
        {
            dbgerr("Multiple block read error\n");
            cs.high();
            return -EBADF;
        }
    }
    cs.high();
    return size;
}

template <class SPI>
ssize_t SPISD<SPI>::writeBlock(const void* buffer, size_t size, off_t where)
{
    if(cardType==0) return -EIO;
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    const unsigned char *buf=reinterpret_cast<const unsigned char*>(buffer);
    Lock<KernelMutex> l(mutex);
    dbg("%s:%d: nSectors=%d\n",__FILE__,__LINE__,nSectors);
    if(!(cardType & 8)) lba*=512; // Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {   // Single block write
        result=sendCmd(CMD24,lba);         // WRITE_BLOCK
        if(result!=0)
        {
            printErrorCode(result);
            cs.high();
            return -EBADF;
        }
        if(sendDataPacket(buf,0xfe)==false)    // WRITE_BLOCK
        {
            dbgerr("Block write error\n");
            cs.high();
            return -EBADF;
        }
    } else { // Multiple block write
        //DBGERR("Mbw\n");//debug only
        if(cardType & 6) sendCmd(ACMD23,nSectors); //Only if it is SD card
        result=sendCmd(CMD25,lba); // WRITE_MULTIPLE_BLOCK
        if(result!=0)
        {
            printErrorCode(result);
            cs.high();
            return -EBADF;
        }
        do {
            if(!sendDataPacket(buf,0xfc)) break;
            buf+=512;
        } while(--nSectors);
        if(!sendDataPacket(0,0xfd)) // STOP_TRAN token
        {
            dbgerr("Multiple block write error\n");
            cs.high();
            return -EBADF;
        }
    }
    cs.high();
    return size;
}

template <class SPI>
int SPISD<SPI>::ioctl(int cmd, void* arg)
{
    if(cardType==0) return -EIO;
    dbg("%s\n",__PRETTY_FUNCTION__);
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<KernelMutex> l(mutex);
    cs.low();
    unsigned char result=waitForCardReady();
    cs.high();
    if(result==0xff) return 0;
    else return -EFAULT;
}

template <class SPI>
SPISD<SPI>::SPISD(std::unique_ptr<SPI> movedSpi, GpioPin cs)
    : Device(Device::BLOCK), spi(std::move(movedSpi)), cs(cs)
{
    cs.high();
    cs.mode(Mode::OUTPUT);

    spi->setBitrate(100*1000); // 100 kHz SPI speed

    // Send 160 clocks (should be at least 74) to exit from pre-init mode.
    // Newer cards seem not to care if this is done, but older cards do.
    for(int i=0;i<20;i++) spi->sendRecv(0xFF);

    unsigned char resp;
    int i;
    for(i=0;i<20;i++)
    {
        resp=sendCmd(CMD0,0);
        if(resp==1) break;
    }
    dbg("CMD0 required %d commands\n",i+1);
    
    if(resp!=1)
    {
        printErrorCode(resp);
        dbgerr("Init failed\n");
        cs.high();
        return; //Error
    }
    unsigned char n, cmd, ty=0, ocr[4];
    // Enter Idle state
    if(sendCmd(CMD8,0x1aa)==1)
    {   /* SDHC */
        for(n=0;n<4;n++) ocr[n]=spi->sendRecv(0xff); // Get return value of R7 resp
        if((ocr[2]==0x01)&&(ocr[3]==0xaa))
        {   // The card can work at vdd range of 2.7-3.6V
            for(i=0;i<200;i++)
            {
                resp=sendCmd(ACMD41, 1UL << 30);
                if(resp==0)
                {
                    if(sendCmd(CMD58,0)==0)
                    {   // Check CCS bit in the OCR
                        for(n=0;n<4;n++) ocr[n]=spi->sendRecv(0xff);
                        if(ocr[0] & 0x40)
                        {
                            ty=12;
                            dbg("SDHC, block addressing supported\n");
                        } else {
                            ty=4;
                            dbg("SDHC, no block addressing\n");
                        }
                    } else dbgerr("CMD58 failed\n");
                    break; //Exit from for
                } else {
                    printErrorCode(resp);
                    Thread::sleep(10);
                }
            }
            if(i==200) dbgerr("ACMD41 timeout\n");
            else dbg("ACMD41 required %d commands\n",i+1);
        } else dbgerr("CMD8 failed\n");
    } else { /* SDSC or MMC */
        if(sendCmd(ACMD41,0)<=1)
        {
            ty=2;
            cmd=ACMD41; /* SDSC */
            dbg("SD card\n");
        } else {
            ty=1;
            cmd=CMD1;   /* MMC */
            dbg("MMC card\n");
        }
        for(i=0;i<200;i++)
        {
            resp=sendCmd(cmd,0);
            if(resp==0)
            {
                if(sendCmd(CMD16,512)!=0)
                {
                    ty=0;
                    dbgerr("CMD16 failed\n");
                }
                break; //Exit from for
            } else {
                printErrorCode(resp);
                Thread::sleep(10);
            }
        }
        dbg("CMD required %d commands\n",i+1);
    }

    if(ty==0)
    {
        cs.high();
        return; //Error
    }
    cardType=ty;

    if(readSdStatus()<0)
    {
        dbgerr("Status error\n");
        cs.high();
        return; //Error
    }

    cs.high();
    //Configure the SPI interface to use at most 25MHz speed
    spi->setBitrate(25*1000*1000);

    dbg("Init done...\n");
}

} // namespace miosix
