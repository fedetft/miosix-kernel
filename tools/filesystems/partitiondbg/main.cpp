#include <cstdio>

#include "interfaces/bsp.h"
#include "miosix.h"
#include "drivers/sdmmc/stm32f2_f4_f7_sd.h"
#include "MBR/mbr.h"
#include "GPT/gpt.h"

using namespace miosix;

void tryMBR() {

    iprintf("Trying MBR\n");
    auto mbrReaderResult = MBR::MBRReader::readMBR(SDIODriver::instance());
    size_t retryCount = 0;
    while(!mbrReaderResult && retryCount < 5) {
        iprintf("Error while reading from device\n");
        mbrReaderResult = MBR::MBRReader::readMBR(SDIODriver::instance());
        retryCount++;
        Thread::sleep(1);
    }

    if (!mbrReaderResult) {
        iprintf("Excedeed the retry count while reading the device.\n");
        return;
    }

    auto mbrReader = *mbrReaderResult;

    if (!mbrReader.isValidMBR()) {
        iprintf("Invalid MBR. Expected: 0xAA55, Got: 0x%04X\n", mbrReader.mbrSignature());
        return;
    }
    
    mbrReader.printMBRInfo();
    iprintf("Finished MBR\n");
}

int main()
{
    iprintf("Starting partitiondbg\n");
    iprintf("Reading SDIO device size...\n");

    off_t cardSize = 0;
    SDIODriver::instance()->ioctl(IOCTL_GET_VOLUME_SIZE, &cardSize);

    iprintf("SDIO device size: %llu bytes\n", cardSize);
    tryMBR();
    iprintf("Trying GPT\n");
    auto gptReaderResult = GPT::GPTReader::readGPT(SDIODriver::instance());
    // auto gptReader = std::move(gptReaderResult.second);
    // if (gptReaderResult.first != GPT::ReaderResult::Ok) {
    //     printf("Error reading gpt partition, reasonID: %d", static_cast<int>(gptReaderResult.first));
    // }
    // TODO: here we can try to try-loop mount the partitions coming from the mbr header
    // We can also create virtual devices showing the partitions in devfs
    // linux does that, so we can have read/write ops on the partition directly
    // This is also helpful from abstracting away the partition offset for fatfs
    // or in general for all filesystems
    // for the automount the partition reader should also create a file like fstab in the 
    // root directory or some virtual file somewhere


    for(;;) {
        ledOn();
        Thread::sleep(500);
        ledOff();
        Thread::sleep(500);
    }
}
