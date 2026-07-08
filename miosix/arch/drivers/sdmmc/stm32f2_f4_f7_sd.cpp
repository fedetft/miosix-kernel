/***************************************************************************
 *   Copyright (C) 2010, 2011, 2012, 2013, 2014 by Terraneo Federico       *
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

#include "stm32f2_f4_f7_sd.h"
#include "interfaces/bsp.h"
#include "interfaces/arch_registers.h"
#include "interfaces/interrupts.h"
#include "interfaces/cache.h"
#include "interfaces/delays.h"
#include "kernel/thread.h"
#include "board_settings.h" //For sdVoltage and SD_ONE_BIT_DATABUS definitions
#include <cstdio>
#include <cstring>
#include <errno.h>

//Note: enabling debugging might cause deadlock when using sleep() or reboot()
//The bug won't be fixed because debugging is only useful for driver development
///\internal Debug macro, for normal conditions
//#define DBG iprintf
#define DBG(x,...) do {} while(0)
///\internal Debug macro, for errors only
//#define DBGERR iprintf
#define DBGERR(x,...) do {} while(0)

/*
 * The SDMMC1 peripheral in the STM32F7 is basically the old SDIO with the
 * registers renamed and a few bits changed. Let's map the old names in the new
 */
#if defined(_CHIP_STM32F7)

#if SD_SDMMC==1
#define SDIO                 SDMMC1
#define RCC_APB2ENR_SDIOEN   RCC_APB2ENR_SDMMC1EN
#define SDIO_IRQn            SDMMC1_IRQn
#elif SD_SDMMC==2
#define SDIO                 SDMMC2
#define RCC_APB2ENR_SDIOEN   RCC_APB2ENR_SDMMC2EN
#define SDIO_IRQn            SDMMC2_IRQn
#else
#error SD_SDMMC undefined or not in range
#endif

#define SDIO_STA_STBITERR    0 //This bit has been removed
#define SDIO_STA_RXOVERR     SDMMC_STA_RXOVERR
#define SDIO_STA_TXUNDERR    SDMMC_STA_TXUNDERR
#define SDIO_STA_DTIMEOUT    SDMMC_STA_DTIMEOUT
#define SDIO_STA_DCRCFAIL    SDMMC_STA_DCRCFAIL
#define SDIO_STA_CMDSENT     SDMMC_STA_CMDSENT
#define SDIO_STA_CMDREND     SDMMC_STA_CMDREND
#define SDIO_STA_CCRCFAIL    SDMMC_STA_CCRCFAIL
#define SDIO_STA_CTIMEOUT    SDMMC_STA_CTIMEOUT
#define SDIO_STA_DATAEND     SDMMC_STA_DATAEND

#define SDIO_CMD_CPSMEN      SDMMC_CMD_CPSMEN
#define SDIO_CMD_WAITRESP_0  SDMMC_CMD_WAITRESP_0
#define SDIO_CMD_WAITRESP_1  SDMMC_CMD_WAITRESP_1

#define SDIO_ICR_CTIMEOUTC   SDMMC_ICR_CTIMEOUTC
#define SDIO_ICR_CCRCFAILC   SDMMC_ICR_CCRCFAILC

#define SDIO_CLKCR_CLKEN     SDMMC_CLKCR_CLKEN
#define SDIO_CLKCR_PWRSAV    SDMMC_CLKCR_PWRSAV
#define SDIO_CLKCR_WIDBUS_0  SDMMC_CLKCR_WIDBUS_0
#define SDIO_CLKCR_HWFC_EN   SDMMC_CLKCR_HWFC_EN

#define SDIO_MASK_STBITERRIE 0 //This bit has been removed
#define SDIO_MASK_RXOVERRIE  SDMMC_MASK_RXOVERRIE
#define SDIO_MASK_TXUNDERRIE SDMMC_MASK_TXUNDERRIE
#define SDIO_MASK_DCRCFAILIE SDMMC_MASK_DCRCFAILIE
#define SDIO_MASK_DTIMEOUTIE SDMMC_MASK_DTIMEOUTIE
#define SDIO_MASK_DATAENDIE  SDMMC_MASK_DATAENDIE

#define SDIO_DCTRL_DMAEN     SDMMC_DCTRL_DMAEN
#define SDIO_DCTRL_DTDIR     SDMMC_DCTRL_DTDIR
#define SDIO_DCTRL_DTEN      SDMMC_DCTRL_DTEN

#define SDIO_POWER_PWRCTRL_1 SDMMC_POWER_PWRCTRL_1
#define SDIO_POWER_PWRCTRL_0 SDMMC_POWER_PWRCTRL_0

constexpr int ICR_FLAGS_CLR=0x5ff;

#else  //defined(_CHIP_STM32F7)

constexpr int ICR_FLAGS_CLR=0x7ff;

#endif //defined(_CHIP_STM32F7)

#if defined(_CHIP_STM32F7)&& SD_SDMMC==2
#define DMA_Stream           DMA2_Stream0
#else
#define DMA_Stream           DMA2_Stream3
#endif

namespace miosix {

static volatile bool driverError;       ///< \internal Errors caused by OS issues (premature wakeup)
static volatile bool dmaTransferError;  ///< \internal DMA transfer error
static volatile bool sdioTransferError; ///< \internal SDIO transfer error
static volatile bool dmaDone;           ///< \internal DMA TC received (no error) — read path
static volatile bool sdioDone;          ///< \internal SDIO DATAEND received (no error)
static volatile bool waitForDma;        ///< \internal Read path: require both DMA TC and SDIO DATAEND
static Thread *waiting;                 ///< \internal Thread waiting for transfer
static unsigned int dmaFlags;           ///< \internal DMA status flags
static unsigned int sdioFlags;          ///< \internal SDIO status flags

/**
 * \internal
 * Called from ISR context only. Wakes the waiting thread when the transfer is
 * fully complete (or on any error). For reads (waitForDma==true), both DMA TC
 * and SDIO DATAEND must have been observed; for writes, SDIO DATAEND alone suffices.
 */
inline static void maybeWakeWaitingThread()
{
    if(!waiting) return;
    bool ready = waitForDma ? (dmaDone && sdioDone) : sdioDone;
    if(ready || dmaTransferError || sdioTransferError)
    {
        waiting->IRQwakeup();
        waiting=nullptr;
    }
}

/**
 * \internal
 * DMA2 Stream3 interrupt handler actual implementation
 */
void SDDMAirqImpl()
{
    dmaFlags=DMA2->LISR;
    #if defined(_CHIP_STM32F7) && SD_SDMMC==2
    if(dmaFlags & (DMA_LISR_TEIF0 | DMA_LISR_DMEIF0 | DMA_LISR_FEIF0))
        dmaTransferError=true;
    else
        dmaDone=true;

    DMA2->LIFCR = DMA_LIFCR_CTCIF0
                | DMA_LIFCR_CTEIF0
                | DMA_LIFCR_CDMEIF0
                | DMA_LIFCR_CFEIF0;
    #else
    if(dmaFlags & (DMA_LISR_TEIF3 | DMA_LISR_DMEIF3 | DMA_LISR_FEIF3))
        dmaTransferError=true;
    else
        dmaDone=true;

    DMA2->LIFCR = DMA_LIFCR_CTCIF3
                | DMA_LIFCR_CTEIF3
                | DMA_LIFCR_CDMEIF3
                | DMA_LIFCR_CFEIF3;
    #endif

    maybeWakeWaitingThread();
}

/**
 * \internal
 * SDIO device interrupt handler actual implementation
 */
void SDirqImpl()
{
    sdioFlags=SDIO->STA;

    #ifdef SDIO_STA_STBITERR
    //Some STM32 chips leave this flag reserved, in that case it's left
    //undefined in the CMSIS headers
    if(sdioFlags & SDIO_STA_STBITERR)
        sdioTransferError=true;
    #endif
    if(sdioFlags & (SDIO_STA_RXOVERR  | SDIO_STA_TXUNDERR | 
                    SDIO_STA_DTIMEOUT | SDIO_STA_DCRCFAIL))
        sdioTransferError=true;
    // Latch DATAEND separately; set only when no error flag is present so that
    // maybeWakeWaitingThread() can distinguish success from failure cleanly.
    if(!sdioTransferError && (sdioFlags & SDIO_STA_DATAEND))
        sdioDone=true;

    SDIO->ICR=ICR_FLAGS_CLR; //Clear flags

    maybeWakeWaitingThread();
}

/*
 * Operating voltage of device. It is sent to the SD card to check if it can
 * work at this voltage. Range *must* be within 28..36
 * Example 33=3.3v
 */
//static const unsigned char sdVoltage=33; //Is defined in board_settings.h
static const unsigned int sdVoltageMask=1<<(sdVoltage-13); //See OCR reg in SD spec

/**
 * \internal
 * Possible state of the cardType variable.
 */
enum CardType
{
    Invalid=0, ///<\internal Invalid card type
    MMC=1<<0,  ///<\internal if(cardType==MMC) card is an MMC
    SDv1=1<<1, ///<\internal if(cardType==SDv1) card is an SDv1
    SDv2=1<<2, ///<\internal if(cardType==SDv2) card is an SDv2
    SDHC=1<<3  ///<\internal if(cardType==SDHC) card is an SDHC
};

///\internal Type of card.
static CardType cardType=Invalid;

//SD card GPIOs
//TODO: expose gpio selection to the BSPs...
#if defined(_CHIP_STM32F7) && SD_SDMMC==2
typedef Gpio<PG,9>  sdD0;
typedef Gpio<PG,10> sdD1;
typedef Gpio<PB,3>  sdD2;
typedef Gpio<PB,4>  sdD3;
typedef Gpio<PD,6>  sdCLK;
typedef Gpio<PD,7>  sdCMD;
#elif defined(_BOARD_STM32F411CE_BLACKPILL)
typedef Gpio<PB,4>  sdD0;
typedef Gpio<PC,9>  sdD1;
typedef Gpio<PC,10> sdD2;
typedef Gpio<PC,11> sdD3;
typedef Gpio<PB,15> sdCLK;
typedef Gpio<PA,6>  sdCMD;
#else
typedef Gpio<PC,8>  sdD0;
typedef Gpio<PC,9>  sdD1;
typedef Gpio<PC,10> sdD2;
typedef Gpio<PC,11> sdD3;
typedef Gpio<PC,12> sdCLK;
typedef Gpio<PD,2>  sdCMD;
#endif


//
// Class BufferConverter
//

/**
 * \internal
 * After fixing the FSMC bug in the stm32f1, ST decided to willingly introduce
 * another quirk in the stm32f4. They introduced a core coupled memory that is
 * not accessible by the DMA. While from an hardware perspective it may make
 * sense, it is a bad design decision when viewed from the software side.
 * This is because if application code allocates a buffer in the core coupled
 * memory and passes that to an fread() or fwrite() call, that buffer is
 * forwarded here, and this driver is DMA-based... Now, in an OS such as Miosix
 * that tries to shield the application developer from such quirks, it is
 * unacceptable to fail to work in such an use case, so this class exists to
 * try and work around this.
 * In essence, the first "bad buffer" that is passed to a readBlock() or
 * writeBlock() causes the allocation on the heap (which Miosix guarantees
 * is not allocated in the core coupled memory) of a 512 byte buffer which is
 * then never deallocated and always reused to deal with these bad buffers.
 * While this works, performance suffers for two reasons: first, when dealing
 * with those bad buffers, the filesystem code is no longer zero copy, and
 * second because multiple block read/writes between bad buffers and the SD
 * card are implemented as a sequence of single block read/writes.
 * If you're an application developer and care about speed, try to allocate
 * your buffers in the heap if you're coding for the STM32F4.
 */
class BufferConverter
{
public:
    /**
     * \internal
     * The buffer will be of this size only.
     */
    static const int BUFFER_SIZE=512;

    /**
     * \internal
     * \return true if the pointer is not inside the CCM
     */
    static bool isGoodBuffer(const void *x)
    {
        unsigned int ptr=reinterpret_cast<const unsigned int>(x);
        return (ptr<0x10000000) || (ptr>=(0x10000000+64*1024));
    }

    /**
     * \internal
     * Convert from a constunsigned char* buffer of size BUFFER_SIZE to a
     * const unsigned int* word aligned buffer.
     * If the original buffer is already word aligned it only does a cast,
     * otherwise it copies the data on the original buffer to a word aligned
     * buffer. Useful if subseqent code will read from the buffer.
     * \param a buffer of size BUFFER_SIZE. Can be word aligned or not.
     * \return a word aligned buffer with the same data of the given buffer
     */
    static const unsigned char *toWordAligned(const unsigned char *buffer);

    /**
     * \internal
     * Convert from an unsigned char* buffer of size BUFFER_SIZE to an
     * unsigned int* word aligned buffer.
     * If the original buffer is already word aligned it only does a cast,
     * otherwise it returns a new buffer which *does not* contain the data
     * on the original buffer. Useful if subseqent code will write to the
     * buffer. To move the written data to the original buffer, use
     * toOriginalBuffer()
     * \param a buffer of size BUFFER_SIZE. Can be word aligned or not.
     * \return a word aligned buffer with undefined content.
     */
    static unsigned char *toWordAlignedWithoutCopy(unsigned char *buffer);

    /**
     * \internal
     * Convert the buffer got through toWordAlignedWithoutCopy() to the
     * original buffer. If the original buffer was word aligned, nothing
     * happens, otherwise a memcpy is done.
     * Note that this function does not work on buffers got through
     * toWordAligned().
     */
    static void toOriginalBuffer();

    /**
     * \internal
     * Can be called to deallocate the buffer
     */
    static void deallocateBuffer();

private:
    static unsigned char *originalBuffer;
    static unsigned char *wordAlignedBuffer;
};

const unsigned char *BufferConverter::toWordAligned(const unsigned char *buffer)
{
    originalBuffer=0; //Tell toOriginalBuffer() that there's nothing to do
    if(isGoodBuffer(buffer))
    {
        return buffer;
    } else {
        if(wordAlignedBuffer==0)
            wordAlignedBuffer=new unsigned char[BUFFER_SIZE];
        std::memcpy(wordAlignedBuffer,buffer,BUFFER_SIZE);
        return wordAlignedBuffer;
    }
}

unsigned char *BufferConverter::toWordAlignedWithoutCopy(
    unsigned char *buffer)
{
    if(isGoodBuffer(buffer))
    {
        originalBuffer=0; //Tell toOriginalBuffer() that there's nothing to do
        return buffer;
    } else {
        originalBuffer=buffer; //Save original pointer for toOriginalBuffer()
        if(wordAlignedBuffer==0)
            wordAlignedBuffer=new unsigned char[BUFFER_SIZE];
        return wordAlignedBuffer;
    }
}

void BufferConverter::toOriginalBuffer()
{
    if(originalBuffer==0) return;
    std::memcpy(originalBuffer,wordAlignedBuffer,BUFFER_SIZE);
    originalBuffer=0;
}

void BufferConverter::deallocateBuffer()
{
    originalBuffer=0; //Invalidate also original buffer
    if(wordAlignedBuffer!=0)
    {
        delete[] wordAlignedBuffer;
        wordAlignedBuffer=0;
    }
}

unsigned char *BufferConverter::originalBuffer=0;
unsigned char *BufferConverter::wordAlignedBuffer=0;


// 
// enum CommandType
// 

/**
 * \internal
 * SD/MMC commands
 * - bit #7 is @ 1 if a command is an ACMDxx. send() will send the
 *   sequence CMD55, CMDxx
 * - bit from #0 to #5 indicate command index (CMD0..CMD63)
 * - bit #6 is don't care
 */
enum class CommandType
{
    CMD0=0,           //GO_IDLE_STATE
    CMD2=2,           //ALL_SEND_CID
    CMD3=3,           //SEND_RELATIVE_ADDR
    ACMD6=0x80 | 6,   //SET_BUS_WIDTH
    CMD7=7,           //SELECT_DESELECT_CARD
    ACMD41=0x80 | 41, //SEND_OP_COND (SD)
    CMD8=8,           //SEND_IF_COND
    CMD9=9,           //SEND_CSD
    CMD12=12,         //STOP_TRANSMISSION
    CMD13=13,         //SEND_STATUS
    CMD16=16,         //SET_BLOCKLEN
    CMD17=17,         //READ_SINGLE_BLOCK
    CMD18=18,         //READ_MULTIPLE_BLOCK
    ACMD23=0x80 | 23, //SET_WR_BLK_ERASE_COUNT (SD)
    CMD24=24,         //WRITE_BLOCK
    CMD25=25,         //WRITE_MULTIPLE_BLOCK
    CMD55=55          //APP_CMD
};

//
// Class CmdResult
//

/**
 * \internal
 * Contains the result of an SD/MMC command
 */
class CmdResult
{
public:

    /**
     * \internal
     * Possible outcomes of sending a command
     */
    enum Error
    {
        Ok=0,        /// No errors
        Timeout,     /// Timeout while waiting command reply
        CRCFail,     /// CRC check failed in command reply
        RespNotMatch,/// Response index does not match command index
        ACMDFail     /// Sending CMD55 failed
    };

    /**
     * \internal
     * Default constructor
     */
    CmdResult(): cmd(CommandType::CMD0), error(Ok), response{0, 0, 0, 0} {}

    /**
     * \internal
     * Constructor,  set the response data
     * \param cmd command index of command that was sent
     * \param result result of command
     */
    CmdResult(CommandType cmd, Error error): cmd(cmd), error(error)
    {
        if(cmd==CommandType::CMD9) 
        {
            response[0]=SDIO->RESP4;
            response[1]=SDIO->RESP3;
            response[2]=SDIO->RESP2;
            response[3]=SDIO->RESP1;
        } else {
            response[0]=SDIO->RESP1;           
        }
    }

    /**
     * \internal
     * \return the 32 bit of the response.
     * May not be valid if getError()!=Ok or the command does not send a
     * response, such as CMD0
     */
    unsigned int getShortResponse() { return response[0]; }

    /**
     * \internal
     * \param part one of the four 32 bit parts of the 128 bit response
     * \return the part selected with the part parameter or 0 if out of bounds
     */
    unsigned int getLongResponsePart(size_t part)
    {
        if(part>=4) return 0;
        return response[part];
    }

    /**
     * \internal
     * \return the error flags of the response
     */
    Error getError() { return error; }

    /**
     * \internal
     * Checks if errors occurred while sending the command.
     * \return true if no errors, false otherwise
     */
    bool validateError();

    /**
     * \internal
     * interprets this->getShortResponse() as an R1 response, and checks if there are
     * errors, or everything is ok
     * \return true on success, false on failure
     */
    bool validateR1Response();

    /**
     * \internal
     * Same as validateR1Response, but can be called with interrupts disabled.
     * \return true on success, false on failure
     */
    bool IRQvalidateR1Response();

    /**
     * \internal checks if there was an error during long response exchange.
     * R2 responses can never generate RespNotMatch error.
     * \return true on success, false on any error
     */
    bool validateR2Response();

    /**
     * \internal
     * interprets this->getShortResponse() as an R6 response, and checks if there are
     * errors, or everything is ok
     * \return true on success, false on failure
     */
    bool validateR6Response();

    /**
     * \internal
     * \return the card state from an R1 or R6 resonse
     */
    unsigned char getState();

private:
    CommandType cmd; ///<\internal Command index that was sent
    Error error; ///<\internal possible error that occurred
    unsigned int response[4]; ///<\internal 32/128bit response
};

bool CmdResult::validateError()
{
    switch(error)
    {
        case Ok:
            return true;
        case Timeout:
            DBGERR("CMD%u: Timeout\n",static_cast<unsigned char>(cmd));
            break;
        case CRCFail:
            DBGERR("CMD%u: CRC Fail\n",static_cast<unsigned char>(cmd));
            break;
        case RespNotMatch:
            DBGERR("CMD%u: Response does not match\n",static_cast<unsigned char>(cmd));
            break;
        case ACMDFail:
            DBGERR("CMD%u: ACMD Fail\n",static_cast<unsigned char>(cmd));
            break;
    }
    return false;
}

bool CmdResult::validateR1Response()
{
    if(error!=Ok) return validateError();
    //Note: this number is obtained with all the flags of R1 which are errors
    //(flagged as E in the SD specification), plus CARD_IS_LOCKED because
    //locked card are not supported by this software driver
    if((response[0] & 0xfff98008)==0) return true;
    DBGERR("CMD%u: R1 response error(s):\n",static_cast<unsigned char>(cmd));
    if(response[0] & (1<<31)) DBGERR("Out of range\n");
    if(response[0] & (1<<30)) DBGERR("ADDR error\n");
    if(response[0] & (1<<29)) DBGERR("BLOCKLEN error\n");
    if(response[0] & (1<<28)) DBGERR("ERASE SEQ error\n");
    if(response[0] & (1<<27)) DBGERR("ERASE param\n");
    if(response[0] & (1<<26)) DBGERR("WP violation\n");
    if(response[0] & (1<<25)) DBGERR("card locked\n");
    if(response[0] & (1<<24)) DBGERR("LOCK_UNLOCK failed\n");
    if(response[0] & (1<<23)) DBGERR("command CRC failed\n");
    if(response[0] & (1<<22)) DBGERR("illegal command\n");
    if(response[0] & (1<<21)) DBGERR("ECC fail\n");
    if(response[0] & (1<<20)) DBGERR("card controller error\n");
    if(response[0] & (1<<19)) DBGERR("unknown error\n");
    if(response[0] & (1<<16)) DBGERR("CSD overwrite\n");
    if(response[0] & (1<<15)) DBGERR("WP ERASE skip\n");
    if(response[0] & (1<<3)) DBGERR("AKE_SEQ error\n");
    return false;
}

bool CmdResult::IRQvalidateR1Response()
{
    if(error!=Ok) return false;
    if(response[0] & 0xfff98008) return false;
    return true;
}

bool CmdResult::validateR2Response() {
    // R2 responses do not contain cmd index so it is always wrong
    if(error!=Ok) return validateError();

    return true;
}

bool CmdResult::validateR6Response()
{
    if(error!=Ok) return validateError();
    if((response[0] & 0xe008)==0) return true;
    DBGERR("CMD%u: R6 response error(s):\n",static_cast<unsigned char>(cmd));
    if(response[0] & (1<<15)) DBGERR("command CRC failed\n");
    if(response[0] & (1<<14)) DBGERR("illegal command\n");
    if(response[0] & (1<<13)) DBGERR("unknown error\n");
    if(response[0] & (1<<3)) DBGERR("AKE_SEQ error\n");
    return false;
}

unsigned char CmdResult::getState()
{
    unsigned char result=(response[0]>>9) & 0xf;
    DBG("CMD%u: State: ",static_cast<unsigned char>(cmd));
    switch(result)
    {
        case 0:  DBG("Idle\n");  break;
        case 1:  DBG("Ready\n"); break;
        case 2:  DBG("Ident\n"); break;
        case 3:  DBG("Stby\n"); break;
        case 4:  DBG("Tran\n"); break;
        case 5:  DBG("Data\n"); break;
        case 6:  DBG("Rcv\n"); break;
        case 7:  DBG("Prg\n"); break;
        case 8:  DBG("Dis\n"); break;
        case 9:  DBG("Btst\n"); break;
        default: DBG("Unknown\n"); break;
    }
    return result;
}

//
// Class Command
//

/**
 * \internal
 * This class allows sending commands to an SD or MMC
 */
class Command
{
public:

    /**
     * \internal
     * Send a command.
     * \param cmd command index (CMD0..CMD63) or ACMDxx command
     * \param arg the 32 bit argument to the command
     * \return a CmdResult object
     */
    static CmdResult send(CommandType cmd, unsigned int arg);

    /**
     * \internal
     * Set the relative card address, obtained during initialization.
     * \param r the card's rca
     */
    static void setRca(unsigned short r) { rca=r; }

    /**
     * \internal
     * \return the card's rca, as set by setRca
     */
    static unsigned int getRca() { return static_cast<unsigned int>(rca); }

private:
    static unsigned short rca;///<\internal Card's relative address
};

CmdResult Command::send(CommandType cmd, unsigned int arg)
{
    unsigned char cc=static_cast<unsigned char>(cmd);
    //Handle ACMDxx as CMD55, CMDxx
    if(cc & 0x80)
    {
        cc &= 0x3f;
        cmd=static_cast<CommandType>(cc);
        DBG("ACMD%u\n",cc);
        CmdResult r=send(CommandType::CMD55,(static_cast<unsigned int>(rca))<<16);
        if(r.validateR1Response()==false)
            return CmdResult(cmd,CmdResult::ACMDFail);
        //Bit 5 @ 1 = next command will be interpreted as ACMD
        if((r.getShortResponse() & (1<<5))==0)
            return CmdResult(cmd,CmdResult::ACMDFail);
    } else DBG("CMD%u\n",cc);

    //Send command
    unsigned int command=SDIO_CMD_CPSMEN | static_cast<unsigned int>(cc);
    if(cmd!=CommandType::CMD0) command |= SDIO_CMD_WAITRESP_0; //CMD0 has no response
    if(cmd==CommandType::CMD2) command |= SDIO_CMD_WAITRESP_1; //CMD2 has long response
    if(cmd==CommandType::CMD9) command |= SDIO_CMD_WAITRESP_1; //CMD9 has long response
    SDIO->ARG=arg;
    SDIO->CMD=command;

    //CMD0 has no response, so wait until it is sent
    if(cmd==CommandType::CMD0)
    {
        for(int i=0;i<500;i++)
        {
            if(SDIO->STA & SDIO_STA_CMDSENT)
            {
                SDIO->ICR=ICR_FLAGS_CLR;//Clear flags
                return CmdResult(cmd,CmdResult::Ok);
            }
            delayUs(1);
        }
        SDIO->ICR=ICR_FLAGS_CLR;//Clear flags
        return CmdResult(cmd,CmdResult::Timeout);
    }

    //Command is not CMD0, so wait a reply
    for(int i=0;i<500;i++)
    {
        unsigned int status=SDIO->STA;
        if(status & SDIO_STA_CMDREND)
        {
            SDIO->ICR=ICR_FLAGS_CLR;//Clear flags
            auto responseCmd = SDIO->RESPCMD;
            //NOTE: CMD2 and CMD9 return long response and don't set RESPCMD
            if(responseCmd==cc || cmd==CommandType::CMD2 || cmd==CommandType::CMD9)
            {
                return CmdResult(cmd,CmdResult::Ok);
            } else {
                DBGERR("CMD%u: Response index does not match. Got %lu\n",cc,responseCmd);
                return CmdResult(cmd,CmdResult::RespNotMatch);
            }
        }
        if(status & SDIO_STA_CCRCFAIL)
        {
            SDIO->ICR=SDIO_ICR_CCRCFAILC;
            return CmdResult(cmd,CmdResult::CRCFail);
        }
        if(status & SDIO_STA_CTIMEOUT) break;
        delayUs(1);
    }
    SDIO->ICR=SDIO_ICR_CTIMEOUTC;
    return CmdResult(cmd,CmdResult::Timeout);
}

unsigned short Command::rca=0;

//
// Class ClockController
//

/**
 * \internal
 * This class controls the clock speed of the SDIO peripheral. It originated
 * from a previous version of this driver, where the SDIO was used in polled
 * mode instead of DMA mode, but has been retained to improve the robustness
 * of the driver.
 */
class ClockController
{
public:

    /**
     * \internal. Set a low clock speed of 400KHz or less, used for
     * detecting SD/MMC cards. This function as a side effect enables 1bit bus
     * width, and disables clock powersave, since it is not allowed by SD spec.
     */
    static void setLowSpeedClock()
    {
        clockReductionAvailable=0;
        // No hardware flow control, SDIO_CK generated on rising edge, 1bit bus
        // width, no clock bypass, no powersave.
        // Set low clock speed 400KHz
        SDIO->CLKCR=CLOCK_400KHz | SDIO_CLKCR_CLKEN;
        SDIO->DTIMER=240000; //Timeout 600ms expressed in SD_CK cycles
    }

    /**
     * \internal
     * Automatically select the data speed. This routine selects the highest
     * sustainable data transfer speed by binary search and accepts a candidate
     * speed only if repeated block reads succeed and return the expected
     * payload.
     *
     * As a side effect, this function enables 4bit bus width and clock
     * powersave.
     */
    static bool calibrateClockSpeed(SDIODriver *sdio);

    /**
     * \internal
     * Reduce the SDIO clock slightly after calibration.
     * This helper is only used later, during normal operation, if a transfer
     * error suggests that the selected speed is marginally too high.
     * Since clock speed is chosen dynamically at runtime, a corner case might
     * be that of a speed which works most of the time but occasionally fails.
     * For maximum robustness, this function is provided to reduce the clock
     * speed slightly in case a data transfer should fail after clock
     * calibration. To avoid inadvertently considering other kind of issues as
     * clock issues, this function can be called only MAX_ALLOWED_REDUCTIONS
     * times after clock calibration, subsequent calls will fail. This will
     * avoid other issues causing an ever decreasing clock speed.
     * \return true on success, false on failure
     */
    static bool reduceClockSpeed();

    /**
     * \internal
     * Read and write operations retry during normal use for robustness. During
     * speed probing each read uses a single attempt, so calibration does not
     * accept a marginal clock just because a later retry happened to succeed.
     */
    static unsigned char getRetryCount() { return retries; }

private:
    /**
     * Set SDIO clock speed
     * \param clkdiv speed is SDIOCLK/(clkdiv+2) 
     */
    static void setClockSpeed(unsigned int clkdiv);
    
    static const unsigned int SDIOCLK=48000000; //On stm32f2 SDIOCLK is always 48MHz
    static const unsigned int CLOCK_400KHz=118; //48MHz/(118+2)=400KHz
    #ifdef OVERRIDE_SD_CLOCK_DIVIDER_MAX
    //Some boards using SDRAM cause SDIO TX Underrun occasionally
    static const unsigned int CLOCK_MAX=OVERRIDE_SD_CLOCK_DIVIDER_MAX;
    #else //OVERRIDE_SD_CLOCK_DIVIDER_MAX
    static const unsigned int CLOCK_MAX=0;      //48MHz/(0+2)  =24MHz
    #endif //OVERRIDE_SD_CLOCK_DIVIDER_MAX

    #ifdef SD_ONE_BIT_DATABUS
    ///\internal Clock enabled, bus width 1bit, clock powersave enabled.
    static const unsigned int CLKCR_FLAGS=SDIO_CLKCR_CLKEN | SDIO_CLKCR_PWRSAV;
    #else //SD_ONE_BIT_DATABUS
    ///\internal Clock enabled, bus width 4bit, clock powersave enabled.
    static const unsigned int CLKCR_FLAGS=SDIO_CLKCR_CLKEN |
        SDIO_CLKCR_WIDBUS_0 | SDIO_CLKCR_PWRSAV;
    #endif //SD_ONE_BIT_DATABUS

    ///\internal Maximum number of calls to IRQreduceClockSpeed() allowed
    static const unsigned char MAX_ALLOWED_REDUCTIONS=1;

    ///\internal value returned by getRetryCount() while *not* calibrating clock.
    static const unsigned char MAX_RETRY=10;

    ///\internal Used to allow only one call to reduceClockSpeed()
    static unsigned char clockReductionAvailable;

    ///\internal value returned by getRetryCount()
    static unsigned char retries;
};

bool ClockController::calibrateClockSpeed(SDIODriver *sdio)
{
    // During calibration we call readBlock(), which can request clock
    // reduction after transfer errors. Keep it disabled while searching or the
    // calibration result would become self-invalidating.
    clockReductionAvailable=0;

    DBG("Automatic speed calibration\n");
    unsigned int reference[512/sizeof(unsigned int)];
    unsigned int probe[512/sizeof(unsigned int)];
    unsigned int minFreq=CLOCK_400KHz;
    unsigned int maxFreq=CLOCK_MAX;
    unsigned int selected;
    bool success=false;
    const unsigned char calibrationProbeReads=2;

    // First capture a known-good reference block at 400kHz. Some unstable
    // speeds can complete a transfer without protocol errors while still
    // returning corrupted payload, so calibration must validate data too.
    // This low-speed reference read may use normal retries because it is
    // not testing the upper bus-speed limit.
    setClockSpeed(minFreq);
    retries=MAX_RETRY;
    if(sdio->readBlock(reinterpret_cast<unsigned char*>(reference),512,0)!=512)
    {
        clockReductionAvailable=MAX_ALLOWED_REDUCTIONS;
        return false;
    }

    // Candidate reads must be single-shot and repeated explicitly: accepting a
    // speed because one retry succeeded would select a marginal clock, while a
    // single successful read would not prove that the selected clock is stable.
    retries=1;
    auto clockCandidateIsStable=[&]() {
        for(unsigned int i=0;i<calibrationProbeReads;i++)
        {
            if(sdio->readBlock(reinterpret_cast<unsigned char*>(probe),512,0)!=512
            || std::memcmp(probe, reference, sizeof(reference))!=0)
                return false;
        }
        return true;
    };

    while(minFreq-maxFreq>1)
    {
        selected=(minFreq+maxFreq)/2;
        DBG("Trying CLKCR=%d\n",selected);
        setClockSpeed(selected);
        if(clockCandidateIsStable()) minFreq=selected;
        else maxFreq=selected;
    }
    //Last round of algorithm
    setClockSpeed(maxFreq);
    success=clockCandidateIsStable();
    if(success)
    {
        DBG("Optimal CLKCR=%d\n",maxFreq);
    } else {
        setClockSpeed(minFreq);
        success=clockCandidateIsStable();
        if(success) DBG("Optimal CLKCR=%d\n",minFreq);
        else DBGERR("Automatic speed calibration failed\n");
    }

    clockReductionAvailable=MAX_ALLOWED_REDUCTIONS;
    retries=MAX_RETRY;
    return success;
}

bool ClockController::reduceClockSpeed()
{
    DBGERR("clock speed reduction requested\n");
    //Ensure this function can be called only a few times
    if(clockReductionAvailable==0) return false;
    clockReductionAvailable--;

    unsigned int currentClkcr=SDIO->CLKCR & 0xff;
    if(currentClkcr==CLOCK_400KHz) return false; //No lower than this value

    //If the value of clockcr is low, increasing it by one is enough since
    //frequency changes a lot, otherwise increase by 2.
    if(currentClkcr<10) currentClkcr++;
    else currentClkcr+=2;

    setClockSpeed(currentClkcr);
    return true;
}

void ClockController::setClockSpeed(unsigned int clkdiv)
{
    #ifndef SD_KEEP_CARD_SELECTED
    SDIO->CLKCR=clkdiv | CLKCR_FLAGS;
    #else //SD_KEEP_CARD_SELECTED
    SDIO->CLKCR=clkdiv | CLKCR_FLAGS | SDIO_CLKCR_HWFC_EN;
    #endif //SD_KEEP_CARD_SELECTED
    //Timeout 600ms expressed in SD_CK cycles
    SDIO->DTIMER=(6*SDIOCLK)/((clkdiv+2)*10);
}

unsigned char ClockController::clockReductionAvailable=false;
unsigned char ClockController::retries=ClockController::MAX_RETRY;

//
// Data send/receive functions
//

/**
 * \internal
 * Wait until the card is ready for data transfer.
 * Can be called independently of the card being selected.
 * \return true on success, false on failure
 */
static bool waitForCardReady()
{
    //The card may remain busy for up to 500ms and there appears to be no way
    //to set an interrupt to wait until it becomes ready again. We can't just
    //poll for that long as if a high priority thread is stuck polling all lower
    //priority threads block, so we are forced to do a sleep. The initial value
    //of 2ms was found to be impacting performance excessively, so we take
    //advantage of high resolution timers by sleeping for 200us, and fallback to
    //the previous value only for slow configurations
    #if !defined(__CODE_IN_XRAM)
    const long long sleepTime=200000;
    #else
    const long long sleepTime=2000000;
    #endif
    long long timeout=getTime()+1500000000; //Timeout 1.5 second
    do {
        CmdResult cr=Command::send(CommandType::CMD13,Command::getRca()<<16);
        if(cr.validateR1Response()==false) return false;
        //Bit 8 in R1 response means ready for data.
        if(cr.getShortResponse() & (1<<8)) return true;
        Thread::nanoSleep(sleepTime);
    } while(getTime()<timeout);
    DBGERR("Timeout waiting card ready\n");
    return false;
}

/**
 * \internal
 * Prints the errors that may occur during a DMA transfer
 */
static void displayBlockTransferError()
{
    DBGERR("Block transfer error\n");
    if(dmaFlags & DMA_LISR_TEIF3)     DBGERR("* DMA Transfer error\n");
    if(dmaFlags & DMA_LISR_DMEIF3)    DBGERR("* DMA Direct mode error\n");
    if(dmaFlags & DMA_LISR_FEIF3)     DBGERR("* DMA Fifo error\n");
    #ifdef SDIO_STA_STBITERR
    if(sdioFlags & SDIO_STA_STBITERR) DBGERR("* SDIO Start bit error\n");
    #endif
    if(sdioFlags & SDIO_STA_RXOVERR)  DBGERR("* SDIO RX Overrun\n");
    if(sdioFlags & SDIO_STA_TXUNDERR) DBGERR("* SDIO TX Underrun error\n");
    if(sdioFlags & SDIO_STA_DCRCFAIL) DBGERR("* SDIO Data CRC fail\n");
    if(sdioFlags & SDIO_STA_DTIMEOUT) DBGERR("* SDIO Data timeout\n");
}

/**
 * \internal
 * Contains initial common code between multipleBlockRead and multipleBlockWrite
 * to clear interrupt and error flags, set the waiting thread and compute the
 * memory transfer size based on buffer alignment
 * \return the best DMA transfer size for a given buffer alignment 
 */
static unsigned int dmaTransferCommonSetup(const unsigned char *buffer, bool readMode=false)
{
    //Clear both SDIO and DMA interrupt flags
    SDIO->ICR=ICR_FLAGS_CLR;
    #if defined(_CHIP_STM32F7) && SD_SDMMC==2
    DMA2->LIFCR = DMA_LIFCR_CTCIF0
                | DMA_LIFCR_CTEIF0
                | DMA_LIFCR_CDMEIF0
                | DMA_LIFCR_CFEIF0;
    #else
    DMA2->LIFCR = DMA_LIFCR_CTCIF3
                | DMA_LIFCR_CTEIF3
                | DMA_LIFCR_CDMEIF3
                | DMA_LIFCR_CFEIF3;
    #endif

    driverError=false;
    dmaTransferError=false;
    sdioTransferError=false;
    dmaDone=false;
    sdioDone=false;
    waitForDma=readMode;
    dmaFlags=sdioFlags=0;
    waiting=Thread::getCurrentThread();
    
    //Select DMA transfer size based on buffer alignment. Best performance
    //is achieved when the buffer is aligned on a 4 byte boundary
    switch(reinterpret_cast<unsigned int>(buffer) & 0x3)
    {
        case 0:  return DMA_SxCR_MSIZE_1; //DMA reads 32bit at a time
        case 2:  return DMA_SxCR_MSIZE_0; //DMA reads 16bit at a time
        default: return 0;                //DMA reads  8bit at a time
    }
}

/**
 * \internal
 * Read a given number of contiguous 512 byte blocks from an SD/MMC card.
 * Card must be selected prior to calling this function.
 * \param buffer, a buffer whose size is 512*nblk bytes
 * \param nblk number of blocks to read.
 * \param lba logical block address of the first block to read.
 */
static bool multipleBlockRead(unsigned char *buffer, unsigned int nblk,
    unsigned int lba)
{
    if(nblk==0) return true;
    while(nblk>32767)
    {
        if(multipleBlockRead(buffer,32767,lba)==false) return false;
        buffer+=32767*512;
        nblk-=32767;
        lba+=32767;
    }
    if(waitForCardReady()==false) return false;
    
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC
    
    unsigned int memoryTransferSize=dmaTransferCommonSetup(buffer, true /*read mode*/);

    // Thread is woken only when BOTH DMA TC (dmaDone) AND SDIO DATAEND (sdioDone)
    // have been observed, or on any error. This eliminates the race where DMA TC
    // fires before the SDIO CRC16 check has completed.
    int32_t t=SDIO_MASK_DATAENDIE  | //Interrupt on data end (CRC passed)
              SDIO_MASK_RXOVERRIE  | //Interrupt on rx overrun
              SDIO_MASK_TXUNDERRIE | //Interrupt on tx underrun
              SDIO_MASK_DCRCFAILIE | //Interrupt on data CRC fail
              SDIO_MASK_DTIMEOUTIE;  //Interrupt on data timeout
    #ifdef SDIO_MASK_STBITERRIE
    t|=SDIO_MASK_STBITERRIE; //Interrupt on start bit error
    #endif
    SDIO->MASK=t;
    DMA_Stream->PAR=reinterpret_cast<unsigned int>(&SDIO->FIFO);
    DMA_Stream->M0AR=reinterpret_cast<unsigned int>(buffer);
    //Note: DMA_Stream->NDTR is don't care in peripheral flow control mode
    DMA_Stream->FCR = DMA_SxFCR_FEIE   //Interrupt on fifo error
                    | DMA_SxFCR_DMDIS  //Fifo enabled
                    | DMA_SxFCR_FTH_0; //Take action if fifo half full
    #if defined(_CHIP_STM32F7) && SD_SDMMC==2
    DMA_Stream->CR = (11 << DMA_SxCR_CHSEL_Pos) //Channel 4 (SDIO)
    #else
    DMA_Stream->CR = DMA_SxCR_CHSEL_2   //Channel 4 (SDIO)
    #endif
                   | DMA_SxCR_PBURST_0  //4-beat bursts read from SDIO
                   | DMA_SxCR_PL_0      //Medium priority DMA stream
                   | memoryTransferSize //RAM data size depends on alignment
                   | DMA_SxCR_PSIZE_1   //Read 32bit at a time from SDIO
                   | DMA_SxCR_MINC      //Increment RAM pointer
                   | 0                  //Peripheral to memory direction
                   | DMA_SxCR_PFCTRL    //Peripheral is flow controller
                   | DMA_SxCR_TCIE      //Interrupt on transfer complete
                   | DMA_SxCR_TEIE      //Interrupt on transfer error
                   | DMA_SxCR_DMEIE     //Interrupt on direct mode error
                   | DMA_SxCR_EN;       //Start the DMA
    
    SDIO->DLEN=nblk*512;
    if(waiting==0)
    {
        DBGERR("Premature wakeup\n");
        driverError=true;
    }
    CmdResult cr=Command::send(nblk>1 ? CommandType::CMD18 : CommandType::CMD17,lba);
    if(cr.validateR1Response())
    {
        //Block size 512 bytes, block data xfer, from card to controller
        SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTDIR | SDIO_DCTRL_DTEN;
        FastGlobalIrqLock dLock;
        while(waiting) Thread::IRQglobalIrqUnlockAndWait(dLock);
    } else sdioTransferError=true;
    DMA_Stream->CR=0;
    while(DMA_Stream->CR & DMA_SxCR_EN) ; //DMA may take time to stop
    SDIO->DCTRL=0; //Disable data path state machine
    SDIO->MASK=0;

    // CMD12 is sent to end CMD18 (multiple block read), or to abort an
    // unfinished read in case of errors
    if(nblk>1 || driverError || sdioTransferError || dmaTransferError)
    {
        cr=Command::send(CommandType::CMD12,0);
        if(driverError || dmaTransferError || sdioTransferError)
        {
            // CMD13 is sent to check the real status of the sdio after CMD12
            // and to reset the card in case if it gets stuck in a illegal state
            cr=Command::send(CommandType::CMD13, Command::getRca()<<16);
        }
    }
    if(sdioTransferError || dmaTransferError || cr.validateR1Response()==false)
    {
        displayBlockTransferError();
        ClockController::reduceClockSpeed();
        return false;
    }
    
    //Read ok, deal with cache coherence
    markBufferAfterDmaRead(buffer,nblk*512);
    return true;
}

/**
 * \internal
 * Write a given number of contiguous 512 byte blocks to an SD/MMC card.
 * Card must be selected prior to calling this function.
 * \param buffer, a buffer whose size is 512*nblk bytes
 * \param nblk number of blocks to write.
 * \param lba logical block address of the first block to write.
 */
static bool multipleBlockWrite(const unsigned char *buffer, unsigned int nblk,
    unsigned int lba)
{
    if(nblk==0) return true;
    while(nblk>32767)
    {
        if(multipleBlockWrite(buffer,32767,lba)==false) return false;
        buffer+=32767*512;
        nblk-=32767;
        lba+=32767;
    }
    
    //Deal with cache coherence
    markBufferBeforeDmaWrite(buffer,nblk*512);
    
    if(waitForCardReady()==false) return false;
    
    if(cardType!=SDHC) lba*=512; // Convert to byte address if not SDHC
    if(nblk>1)
    {
        CmdResult cr=Command::send(CommandType::ACMD23,nblk);
        if(cr.validateR1Response()==false) return false;
    }
    
    unsigned int memoryTransferSize=dmaTransferCommonSetup(buffer);
    
    //Data transfer is considered complete once the SDIO transfer complete
    //interrupt occurs, that happens when the last data was written to the SDIO
    //Both SDIO and DMA error interrupts are active to catch errors
    uint32_t t=SDIO_MASK_DATAENDIE  | //Interrupt on data end
               SDIO_MASK_RXOVERRIE  | //Interrupt on rx underrun
               SDIO_MASK_TXUNDERRIE | //Interrupt on tx underrun
               SDIO_MASK_DCRCFAILIE | //Interrupt on data CRC fail
               SDIO_MASK_DTIMEOUTIE;  //Interrupt on data timeout
    #ifdef SDIO_MASK_STBITERRIE
    t|=SDIO_MASK_STBITERRIE; //Interrupt on start bit error
    #endif
    SDIO->MASK=t;
    DMA_Stream->PAR=reinterpret_cast<unsigned int>(&SDIO->FIFO);
    DMA_Stream->M0AR=reinterpret_cast<unsigned int>(buffer);
    //Note: DMA_Stream->NDTR is don't care in peripheral flow control mode
    //Quirk: not enabling DMA_SxFCR_FEIE because the SDIO seems to generate
    //a spurious fifo error. The code was tested and the transfer completes
    //successfully even in the presence of this fifo error
    DMA_Stream->FCR = DMA_SxFCR_DMDIS  //Fifo enabled
                    | DMA_SxFCR_FTH_1  //Take action if fifo full
                    | DMA_SxFCR_FTH_0;
#if defined(_CHIP_STM32F7) && SD_SDMMC==2
    DMA_Stream->CR = (11 << DMA_SxCR_CHSEL_Pos) // Channel 4 (SDIO)
#else
    DMA_Stream->CR = DMA_SxCR_CHSEL_2     // Channel 4 (SDIO)
#endif
                   | DMA_SxCR_PBURST_0  //4-beat bursts write to SDIO
                   | DMA_SxCR_PL_0      //Medium priority DMA stream
                   | memoryTransferSize //RAM data size depends on alignment
                   | DMA_SxCR_PSIZE_1   //Write 32bit at a time to SDIO
                   | DMA_SxCR_MINC      //Increment RAM pointer
                   | DMA_SxCR_DIR_0     //Memory to peripheral direction
                   | DMA_SxCR_PFCTRL    //Peripheral is flow controller
                   | DMA_SxCR_TEIE      //Interrupt on transfer error
                   | DMA_SxCR_DMEIE     //Interrupt on direct mode error
                   | DMA_SxCR_EN;       //Start the DMA
    
    SDIO->DLEN=nblk*512;
    if(waiting==0)
    {
        DBGERR("Premature wakeup\n");
        driverError=true;
    }
    CmdResult cr=Command::send(nblk>1 ? CommandType::CMD25 : CommandType::CMD24,lba);
    if(cr.validateR1Response())
    {
        //Block size 512 bytes, block data xfer, from card to controller
        SDIO->DCTRL=(9<<4) | SDIO_DCTRL_DMAEN | SDIO_DCTRL_DTEN;
        FastGlobalIrqLock dLock;
        while(waiting) Thread::IRQglobalIrqUnlockAndWait(dLock);
    } else sdioTransferError=true;
    DMA_Stream->CR=0;
    while(DMA_Stream->CR & DMA_SxCR_EN) ; //DMA may take time to stop
    SDIO->DCTRL=0; //Disable data path state machine
    SDIO->MASK=0;

    // CMD12 is sent to end CMD25 (multiple block write), or to abort an
    // unfinished write in case of errors
    if(nblk>1 || driverError || sdioTransferError || dmaTransferError)
    {
        cr=Command::send(CommandType::CMD12,0);
        if(driverError || dmaTransferError || sdioTransferError)
        {
            // CMD13 is sent to check the real status of the sdio after CMD12
            // and to reset the card in case if it gets stuck in a illegal state
            cr=Command::send(CommandType::CMD13,Command::getRca()<<16);
        }
    }
    if(sdioTransferError || dmaTransferError || cr.validateR1Response()==false)
    {
        displayBlockTransferError();
        ClockController::reduceClockSpeed();
        return false;
    }
    return true;
}

//
// Class CardSelector
//

#ifndef SD_KEEP_CARD_SELECTED
/**
 * \internal
 * Simple RAII class for selecting an SD/MMC card an automatically deselect it
 * at the end of the scope.
 */
class CardSelector
{
public:
    /**
     * \internal
     * Constructor. Selects the card.
     * The result of the select operation is available through its succeded()
     * member function
     */
    explicit CardSelector()
    {
        success=Command::send(
                CommandType::CMD7,Command::getRca()<<16).validateR1Response();
    }

    /**
     * \internal
     * \return true if the card was selected, false on error
     */
    bool succeded() { return success; }

    /**
     * \internal
     * Destructor, ensures that the card is deselected
     */
    ~CardSelector()
    {
        Command::send(CommandType::CMD7,0); //Deselect card. This will timeout
    }

private:
    bool success;
};
#endif //SD_KEEP_CARD_SELECTED

//
// Initialization helper functions
//

/**
 * \internal
 * One-time SDIO setup: clocks, GPIO alternate functions and IRQ registration.
 */
static void initSDIOPeripheralOnce()
{
    //Doing read-modify-write on RCC->APBENR2 and gpios, better be safe
    GlobalIrqLock lock;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN
                  | RCC_AHB1ENR_GPIODEN
                  | RCC_AHB1ENR_DMA2EN;
    RCC_SYNC();
    RCC->APB2ENR |= RCC_APB2ENR_SDIOEN;
    RCC_SYNC();
    #if defined(_CHIP_STM32F7) && SD_SDMMC==2
    sdD0::mode(Mode::ALTERNATE);
    sdD0::alternateFunction(11);
    #ifndef SD_ONE_BIT_DATABUS
    sdD1::mode(Mode::ALTERNATE);
    sdD1::alternateFunction(11);
    sdD2::mode(Mode::ALTERNATE);
    sdD2::alternateFunction(10);
    sdD3::mode(Mode::ALTERNATE);
    sdD3::alternateFunction(10);
    #endif // SD_ONE_BIT_DATABUS
    sdCLK::mode(Mode::ALTERNATE);
    sdCLK::alternateFunction(11);
    sdCMD::mode(Mode::ALTERNATE);
    sdCMD::alternateFunction(11);
    #else
    sdD0::mode(Mode::ALTERNATE);
    sdD0::alternateFunction(12);
    #ifndef SD_ONE_BIT_DATABUS
    sdD1::mode(Mode::ALTERNATE);
    sdD1::alternateFunction(12);
    sdD2::mode(Mode::ALTERNATE);
    sdD2::alternateFunction(12);
    sdD3::mode(Mode::ALTERNATE);
    sdD3::alternateFunction(12);
    #endif // SD_ONE_BIT_DATABUS
    sdCLK::mode(Mode::ALTERNATE);
    sdCLK::alternateFunction(12);
    sdCMD::mode(Mode::ALTERNATE);
    sdCMD::alternateFunction(12);
    #endif

    #if defined(_CHIP_STM32F7) && SD_SDMMC==2
    IRQregisterIrq(lock,DMA2_Stream0_IRQn,SDDMAirqImpl);
    #else
    IRQregisterIrq(lock,DMA2_Stream3_IRQn,SDDMAirqImpl);
    #endif
    IRQregisterIrq(lock,SDIO_IRQn,SDirqImpl);
}

/**
 * \internal
 * Reset and restart the SDIO peripheral state machine.
 */
static void initSDIOPeripheral()
{
    SDIO->POWER=0; //Power off state
    delayUs(1);
    SDIO->CLKCR=0;
    SDIO->CMD=0;
    SDIO->DCTRL=0;
    #if defined(_CHIP_STM32F7)
    SDIO->ICR=0x4005ff;
    #else
    SDIO->ICR=0xc007ff;
    #endif
    SDIO->POWER=SDIO_POWER_PWRCTRL_1 | SDIO_POWER_PWRCTRL_0; //Power on state
    //This delay is particularly important: when setting the POWER register a
    //glitch on the CMD pin happens. This glitch has a fast fall time and a slow
    //rise time resembling an RC charge with a ~6us rise time. If the clock is
    //started too soon, the card sees a clock pulse while CMD is low, and
    //interprets it as a start bit. No, setting POWER to powerup does not
    //eliminate the glitch.
    delayUs(10);
    ClockController::setLowSpeedClock();
    //Wait at least 74 clock cycles before first command
    Thread::nanoSleep(250000);
}

/**
 * \internal
 * Detect if the card is an SDHC, SDv2, SDv1, MMC
 * \return Type of card: (1<<0)=MMC (1<<1)=SDv1 (1<<2)=SDv2 (1<<2)|(1<<3)=SDHC
 * or Invalid if card detect failed.
 */
static CardType detectCardType()
{
    const int INIT_TIMEOUT=200; //200*10ms= 2 seconds
    CmdResult r=Command::send(CommandType::CMD8,0x1aa);
    if(r.validateError())
    {
        //We have an SDv2 card connected
        if(r.getShortResponse()!=0x1aa)
        {
            DBGERR("CMD8 validation: voltage range fail\n");
            return Invalid;
        }
        for(int i=0;i<INIT_TIMEOUT;i++)
        {
            //Bit 30 @ 1 = tell the card we like SDHCs
            r=Command::send(CommandType::ACMD41,(1<<30) | sdVoltageMask);
            //ACMD41 sends R3 as response, whose CRC is wrong.
            if(r.getError()!=CmdResult::Ok && r.getError()!=CmdResult::CRCFail)
            {
                r.validateError();
                return Invalid;
            }
            if((r.getShortResponse() & (1<<31))==0) //Busy bit
            {
                Thread::sleep(10);
                continue;
            }
            if((r.getShortResponse() & sdVoltageMask)==0)
            {
                DBGERR("ACMD41 validation: voltage range fail\n");
                return Invalid;
            }
            DBG("ACMD41 validation: looped %d times\n",i);
            if(r.getShortResponse() & (1<<30))
            {
                DBG("SDHC\n");
                return SDHC;
            } else {
                DBG("SDv2\n");
                return SDv2;
            }
        }
        DBGERR("ACMD41 validation: looped until timeout\n");
        return Invalid;
    } else {
        //We have an SDv1 or MMC
        r=Command::send(CommandType::ACMD41,sdVoltageMask);
        //ACMD41 sends R3 as response, whose CRC is wrong.
        if(r.getError()!=CmdResult::Ok && r.getError()!=CmdResult::CRCFail)
        {
            //MMC card
            DBG("MMC card\n");
            return MMC;
        } else {
            //SDv1 card
            for(int i=0;i<INIT_TIMEOUT;i++)
            {
                //ACMD41 sends R3 as response, whose CRC is wrong.
                if(r.getError()!=CmdResult::Ok &&
                        r.getError()!=CmdResult::CRCFail)
                {
                    r.validateError();
                    return Invalid;
                }
                if((r.getShortResponse() & (1<<31))==0) //Busy bit
                {
                    Thread::sleep(10);
                    //Send again command
                    r=Command::send(CommandType::ACMD41,sdVoltageMask);
                    continue;
                }
                if((r.getShortResponse() & sdVoltageMask)==0)
                {
                    DBGERR("ACMD41 validation: voltage range fail\n");
                    return Invalid;
                }
                DBG("ACMD41 validation: looped %d times\nSDv1\n",i);
                return SDv1;
            }
            DBGERR("ACMD41 validation: looped until timeout\n");
            return Invalid;
        }
    }
}

static off_t getCardSize() {

    CmdResult r=Command::send(CommandType::CMD9,Command::getRca()<<16);    //Get CSD 
    if(r.validateR2Response()==false) return 0;
    auto csdStructure = (r.getLongResponsePart(3) & 0xC0000000) >> 30;
    switch (csdStructure) {
        case 0: {
            DBG("CSD structure version 1.0\n");
            auto cSize  = ((r.getLongResponsePart(2) & 0x000003ff)) << 2 | ((r.getLongResponsePart(1) & 0xC0000000) >> 30);
            DBG("C_SIZE=%8X\n", cSize);
            auto cSizeMult = (r.getLongResponsePart(1) & 0x00038000) >> 15;
            DBG("C_SIZE_MULT=%8X\n", cSizeMult);
            auto readBlLen = (r.getLongResponsePart(2) & 0x000F0000) >> 16;
            DBG("READ_BL_LEN=%8X\n", readBlLen);
            return (cSize + 1) * (1 << (cSizeMult + 2)) * (1 << readBlLen); // (C_SIZE + 1) * 2^(C_SIZE_MULT + 2) * 2^READ_BL_LEN
        }
        case 1: {
            DBG("CSD structure version 2.0\n");
            off_t cSize = (r.getLongResponsePart(1) & 0xffff0000) >> 16 | ((r.getLongResponsePart(2) & 0x0000001f) << 16);
            DBG("C_SIZE=%16llX\n", cSize);
            return (cSize + 1) * (512 * 1024); // (C_SIZE + 1) * 512KB, since C_SIZE is in units of 512KB for CSD version 2.0
        }
        case 2: {
            DBG("CSD structure version 3.0\n");
            off_t cSize = (r.getLongResponsePart(1) & 0xffff0000) >> 16 | ((r.getLongResponsePart(2) & 0x000007ff) << 16);
            DBG("C_SIZE=%16llX\n", cSize);
            return (cSize + 1) * (512 * 1024); // Same formula as CSD version 2.0, but with a larger C_SIZE field
        }
        default:
            DBGERR("Unsupported CSD structure version: %d\n", csdStructure);
            return 0;
    }

}

//
// class SDIODriver
//

intrusive_ref_ptr<SDIODriver> SDIODriver::instance()
{
    static KernelMutex m;
    static intrusive_ref_ptr<SDIODriver> instance;
    Lock<KernelMutex> l(m);
    if(!instance) instance=new SDIODriver();
    return instance;
}

ssize_t SDIODriver::readBlock(void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    Lock<KernelMutex> l(mutex);
    DBG("SDIODriver::readBlock(): nSectors=%d\n",nSectors);
    bool goodBuffer=BufferConverter::isGoodBuffer(buffer);
    if(goodBuffer==false) DBG("Buffer inside CCM\n");
    
    for(int i=0;i<ClockController::getRetryCount();i++)
    {
        #ifndef SD_KEEP_CARD_SELECTED
        CardSelector selector;
        if(selector.succeded()==false) continue;
        #endif //SD_KEEP_CARD_SELECTED
        bool error=false;
        
        if(goodBuffer)
        {
            if(multipleBlockRead(reinterpret_cast<unsigned char*>(buffer),
                nSectors,lba)==false) error=true;
        } else {
            //Fallback code to work around CCM
            unsigned char *tempBuffer=reinterpret_cast<unsigned char*>(buffer);
            unsigned int tempLba=lba;
            for(unsigned int j=0;j<nSectors;j++)
            {
                unsigned char* b=BufferConverter::toWordAlignedWithoutCopy(tempBuffer);
                if(multipleBlockRead(b,1,tempLba)==false)
                {
                    error=true;
                    break;
                }
                BufferConverter::toOriginalBuffer();
                tempBuffer+=512;
                tempLba++;
            }
        }
        
        if(error==false)
        {
            if(i>0) DBGERR("Read: required %d retries\n",i);
            return size;
        }
    }
    return -EBADF;
}

ssize_t SDIODriver::writeBlock(const void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    Lock<KernelMutex> l(mutex);
    DBG("SDIODriver::writeBlock(): nSectors=%d\n",nSectors);
    bool goodBuffer=BufferConverter::isGoodBuffer(buffer);
    if(goodBuffer==false) DBG("Buffer inside CCM\n");
    
    for(int i=0;i<ClockController::getRetryCount();i++)
    {
        #ifndef SD_KEEP_CARD_SELECTED
        CardSelector selector;
        if(selector.succeded()==false) continue;
        #endif //SD_KEEP_CARD_SELECTED
        bool error=false;
        
        if(goodBuffer)
        {
            if(multipleBlockWrite(reinterpret_cast<const unsigned char*>(buffer),
                nSectors,lba)==false) error=true;
        } else {
            //Fallback code to work around CCM
            const unsigned char *tempBuffer=
                reinterpret_cast<const unsigned char*>(buffer);
            unsigned int tempLba=lba;
            for(unsigned int j=0;j<nSectors;j++)
            {
                const unsigned char* b=BufferConverter::toWordAligned(tempBuffer);
                if(multipleBlockWrite(b,1,tempLba)==false)
                {
                    error=true;
                    break;
                }
                tempBuffer+=512;
                tempLba++;
            }
        }
        
        if(error==false)
        {
            if(i>0) DBGERR("Write: required %d retries\n",i);
            return size;
        }
    }
    return -EBADF;
}

int SDIODriver::ioctl(int cmd, void* arg)
{
    DBG("SDIODriver::ioctl()\n");
    switch (cmd) {
    case IOCTL_REINIT: {
        DBG("IOCTL_REINIT\n");
        return reinitialize(true) ? 0 : -EFAULT;
    }
    case IOCTL_GET_VOLUME_SIZE: {
        DBG("IOCTL_GET_VOLUME_SIZE\n");
        off_t* sizePtr=static_cast<off_t*>(arg);
        if (sizePtr==nullptr) {
            return -EINVAL;
        }
        Lock<KernelMutex> l(mutex);
        *sizePtr=cardSize;
        return 0;
    }
    case IOCTL_SYNC: {
        DBG("IOCTL_SYNC\n");
        Lock<KernelMutex> l(mutex);
        // Note: no need to select card, since status can be queried even with card
        // not selected.
        return waitForCardReady() ? 0 : -EFAULT;
    }
    default:
        return -ENOTTY;
    }
}

bool SDIODriver::sdioReinitLocked()
{
    initSDIOPeripheral();

    // This is more important than it seems, since CMD55 requires the card's RCA
    // as argument. During initalization, after CMD0 the card has an RCA of zero
    // so without this line ACMD41 will fail and the card won't be initialized.
    Command::setRca(0);

    //Send card reset command
    CmdResult r=Command::send(CommandType::CMD0,0);
    if(r.validateError()==false) return false;

    cardType=detectCardType();
    if(cardType==Invalid) return false; //Card detect failed
    if(cardType==MMC) return false; //MMC cards currently unsupported

    // Now give an RCA to the card. In theory we should loop and enumerate all
    // the cards but this driver supports only one card.
    r=Command::send(CommandType::CMD2,0);
    if(r.validateR2Response()==false) return false;

    r=Command::send(CommandType::CMD3,0);
    if(r.validateR6Response()==false) return false;
    Command::setRca(r.getShortResponse()>>16);
    DBG("Got RCA=%u\n",Command::getRca());
    if(Command::getRca()==0)
    {
        //RCA=0 can't be accepted, since it is used to deselect cards
        DBGERR("RCA=0 is invalid\n");
        return false;
    }

    // Here we should be able to get the card's CSD
    cardSize = getCardSize();
    if (cardSize == 0)
    {
        DBGERR("Failed to get card size or card is empty\n");
    } else {
        DBG("Card size: %llu bytes\n", cardSize);
    }

    //Lastly, try selecting the card and configure the latest bits
    {
        #ifndef SD_KEEP_CARD_SELECTED
        CardSelector selector;
        if(selector.succeded()==false) return false;
        #else //SD_KEEP_CARD_SELECTED
        //Select card here, and keep it selected indefinitely
        r=Command::send(CommandType::CMD7,Command::getRca()<<16);
        if(r.validateR1Response()==false) return false;
        #endif //SD_KEEP_CARD_SELECTED

        r=Command::send(CommandType::CMD13,Command::getRca()<<16);//Get status
        if(r.validateR1Response()==false) return false;
        if(r.getState()!=4) //4=Tran state
        {
            DBGERR("CMD7 was not able to select card\n");
            return false;
        }

        #ifndef SD_ONE_BIT_DATABUS
        r=Command::send(CommandType::ACMD6,2);   //Set 4 bit bus width
        if(r.validateR1Response()==false) return false;
        #endif //SD_ONE_BIT_DATABUS
        
        if(cardType!=SDHC)
        {
            r=Command::send(CommandType::CMD16,512); //Set 512Byte block length
            if(r.validateR1Response()==false) return false;
        }
    }

    // Reinitialization restores a defined card state. Clock calibration is
    // left to the caller so it can be requested only when needed.
    return true;
}

bool SDIODriver::reinitialize(bool calibrate)
{
    Lock<KernelMutex> l(mutex);
    if(sdioReinitLocked()==false) return false;
    if(calibrate==false) return true;
    return ClockController::calibrateClockSpeed(this);
}

SDIODriver::SDIODriver() : Device(Device::BLOCK)
{
    initSDIOPeripheralOnce();
    if(reinitialize(true)) DBG("SDIO init: Success\n");
}

} //namespace miosix
