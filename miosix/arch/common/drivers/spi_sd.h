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
    SPISD(SPI& spi);
    
    virtual ssize_t readBlock(void *buffer, size_t size, off_t where);
    
    virtual ssize_t writeBlock(const void *buffer, size_t size, off_t where);
    
    virtual int ioctl(int cmd, void *arg);
private:
    //Note: enabling debugging might cause deadlock when using sleep() or reboot()
    //The bug won't be fixed because debugging is only useful for driver development
    ///\internal When set to 1, enables error messages.
    ///When set to 2, enables debug messages as well.
    static const int dbgLevel=0;
    ///\internal Debug macro, for normal conditions
    static inline void dbg(const char *fmt, ...)
    {
        if(dbgLevel<2) return;
        va_list arg;
        va_start(arg,fmt);
        viprintf(fmt,arg);
        va_end(arg);
    }
    ///\internal Debug macro, for errors only
    static inline void dbgerr(const char *fmt, ...)
    {
        if(dbgLevel<1) return;
        va_list arg;
        va_start(arg,fmt);
        viprintf(fmt,arg);
        va_end(arg);
    }

    static void print_error_code(unsigned char value);

    void CS_HIGH() {}
    void CS_LOW() {}
    
    unsigned char spi_1_send(unsigned char outgoing)
    {
        return spi.sendRecv(outgoing);
    }

    char sd_status();
    unsigned char wait_ready();
    unsigned char send_cmd(unsigned char cmd, unsigned int arg);
    bool rx_datablock(unsigned char *buf, unsigned int btr);
    bool tx_datablock(const unsigned char *buf, unsigned char token);

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
    * - bit #7 @ 1 indicates that it is an ACMD. send_cmd() will send CMD55, then
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

    SPI &spi;
    KernelMutex mutex;
    ///\internal Type of card (1<<0)=MMC (1<<1)=SDv1 (1<<2)=SDv2 (1<<2)|(1<<3)=SDHC
    unsigned char cardType;
};

/**
 * \internal
 * Used for debugging, print 8 bit error code from SD card
 */
template <class SPI>
void SPISD<SPI>::print_error_code(unsigned char value)
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

/**
 * \internal
 * Return 1 if card is OK, otherwise print 16 bit error code from SD card
 */
template <class SPI>
char SPISD<SPI>::sd_status()
{
    short value=send_cmd(CMD13,0);
    value<<=8;
    value|=spi_1_send(0xff);

    switch(value)
    {
        case 0x0000:
            return 1;
        case 0x0001:
            dbgerr("Card is Locked\n");
            /*//Try to fix the problem by erasing all the SD card.
            char e=send_cmd(CMD16,1);
            if(e!=0) print_error_code(e);
            e=send_cmd(CMD42,0);
            if(e!=0) print_error_code(e);
            spi_1_send(0xfe); // Start block
            spi_1_send(1<<3); //Erase all disk command
            spi_1_send(0xff); // Checksum part 1
            spi_1_send(0xff); // Checksum part 2
            e=spi_1_send(0xff);
            iprintf("Reached here 0x%x\n",e);//Should return 0xe5
            while(spi_1_send(0xff)!=0xff);*/
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
                print_error_code((unsigned char)(value>>8));
            else
                dbgerr("Unknown error 0x%x\n",value);
            break;
    }
    return -1;
}

/**
 * \internal
 * Wait until card is ready
 */
template <class SPI>
unsigned char SPISD<SPI>::wait_ready()
{
    unsigned char result;
    // Backoff of 100us; do not use Thread::sleep, too few cycles
    for(int i=0;i<10;i++)
    {
        result=spi_1_send(0xff);
        if(result==0xff) return 0xff;
        delayUs(100);
    }
    unsigned int t=0;
    while(t<=500) // Timeout ~500ms
    {
        result=spi_1_send(0xff);
        if(result==0xff) return 0xff;
        // exponential backoff
        if(t<10) { Thread::sleep(1); t+=1; }
        else if(t<100) { Thread::sleep(10); t+=10; }
        else { Thread::sleep(100); t+=100; }
    }
    dbgerr("Error: wait_ready() (result=%x)\n",result);
    return result;
}

/**
 * \internal
 * Send a command to the SD card
 * \param cmd one among the #define'd commands
 * \param arg command's argument
 * \return command's r1 response. If command returns a longer response, the user
 * can continue reading the response with spi_1_send(0xff)
 */
template <class SPI>
unsigned char SPISD<SPI>::send_cmd(unsigned char cmd, unsigned int arg)
{
    unsigned char n, res;
    if(cmd & 0x80)
    {	// ACMD<n> is the command sequence of CMD55-CMD<n>
        cmd&=0x7f;
        res=send_cmd(CMD55,0);
        if(res>1) return res;
    }

    // Select the card and wait for ready
    CS_HIGH();
    CS_LOW();

    if(cmd==CMD0)
    {
        //wait_ready on CMD0 seems to cause infinite loop
        spi_1_send(0xff);
    } else {
        if(wait_ready()!=0xff) return 0xff;
    }
    // Send command
    spi_1_send(cmd);			            // Start + Command index
    spi_1_send((unsigned char)(arg >> 24));	// Argument[31..24]
    spi_1_send((unsigned char)(arg >> 16));	// Argument[23..16]
    spi_1_send((unsigned char)(arg >> 8));	// Argument[15..8]
    spi_1_send((unsigned char)arg);		    // Argument[7..0]
    n=0x01;                // Dummy CRC + Stop
    if (cmd==CMD0) n=0x95; // Valid CRC for CMD0(0)
    if (cmd==CMD8) n=0x87; // Valid CRC for CMD8(0x1AA)
    spi_1_send(n);
    // Receive response
    if (cmd==CMD12) spi_1_send(0xff);   // Skip a stuff byte when stop reading
    n=10; // Wait response, try 10 times
    do
        res=spi_1_send(0xff);
    while ((res & 0x80) && --n);
    return res; // Return with the response value
}

/**
 * \internal
 * Receive a data packet from the SD card
 * \param buf data buffer to store received data
 * \param byte count (must be multiple of 4)
 * \return true on success, false on failure
 */
template <class SPI>
bool SPISD<SPI>::rx_datablock(unsigned char *buf, unsigned int btr)
{
    unsigned char token;
    for(int i=0;i<0xffff;i++)
    {
        token=spi_1_send(0xff);
        if(token!=0xff) break;
    }
    if(token!=0xfe) return false; // If not valid data token, retutn error

    spi.recv(buf,btr);
    spi_1_send(0xff); // Discard CRC
    spi_1_send(0xff);

    return true; // Return success
}

/**
 * \internal
 * Send a data packet to the SD card
 * \param buf 512 byte data block to be transmitted
 * \param token data start/stop token
 * \return true on success, false on failure
 */
template <class SPI>
bool SPISD<SPI>::tx_datablock(const unsigned char *buf, unsigned char token)
{
    unsigned char resp;
    if(wait_ready()!=0xff) return false;

    spi_1_send(token);      // Xmit data token
    if (token!=0xfd)
    {                       // Is data token
        spi.send(buf,512);
        spi_1_send(0xff);		// CRC (Dummy)
        spi_1_send(0xff);
        resp=spi_1_send(0xff);          // Receive data response
        if((resp & 0x1f)!=0x05) 	// If not accepted, return error
        return false;
    }
    return true;
}

template <class SPI>
ssize_t SPISD<SPI>::readBlock(void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    unsigned char *buf=reinterpret_cast<unsigned char*>(buffer);
    Lock<KernelMutex> l(mutex);
    dbg("%s:%d: nSectors=%d\n",__FILE__,__LINE__,nSectors);
    if(!(cardType & 8)) lba*=512;	// Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {           // Single block read
        result=send_cmd(CMD17,lba);	// READ_SINGLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        if(rx_datablock(buf,512)==false)
        {
            dbgerr("Block read error\n");
            CS_HIGH();
            return -EBADF;
        }
    } else {	// Multiple block read
        //dbgerr("Mbr\n");//debug only
        result=send_cmd(CMD18,lba);   // READ_MULTIPLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        do {
            if(!rx_datablock(buf,512)) break;
            buf+=512;
        } while(--nSectors);
        send_cmd(CMD12,0);			// STOP_TRANSMISSION
        if(nSectors!=0)
        {
            dbgerr("Multiple block read error\n");
            CS_HIGH();
            return -EBADF;
        }
    }
    CS_HIGH();
    return size;
}

template <class SPI>
ssize_t SPISD<SPI>::writeBlock(const void* buffer, size_t size, off_t where)
{
    if(where % 512 || size % 512) return -EFAULT;
    unsigned int lba=where/512;
    unsigned int nSectors=size/512;
    const unsigned char *buf=reinterpret_cast<const unsigned char*>(buffer);
    Lock<KernelMutex> l(mutex);
    dbg("%s:%d: nSectors=%d\n",__FILE__,__LINE__,nSectors);
    if(!(cardType & 8)) lba*=512;	// Convert to byte address if needed
    unsigned char result;
    if(nSectors==1)
    {           // Single block write
        result=send_cmd(CMD24,lba);         // WRITE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        if(tx_datablock(buf,0xfe)==false)    // WRITE_BLOCK
        {
            dbgerr("Block write error\n");
            CS_HIGH();
            return -EBADF;
        }
    } else {	// Multiple block write
        //DBGERR("Mbw\n");//debug only
        if(cardType & 6) send_cmd(ACMD23,nSectors);//Only if it is SD card
        result=send_cmd(CMD25,lba);          // WRITE_MULTIPLE_BLOCK
        if(result!=0)
        {
            print_error_code(result);
            CS_HIGH();
            return -EBADF;
        }
        do {
            if(!tx_datablock(buf,0xfc)) break;
            buf+=512;
        } while(--nSectors);
        if(!tx_datablock(0,0xfd))    // STOP_TRAN token
        {
            dbgerr("Multiple block write error\n");
            CS_HIGH();
            return -EBADF;
        }
    }
    CS_HIGH();
    return size;
}

template <class SPI>
int SPISD<SPI>::ioctl(int cmd, void* arg)
{
    dbg("%s\n",__PRETTY_FUNCTION__);
    if(cmd!=IOCTL_SYNC) return -ENOTTY;
    Lock<KernelMutex> l(mutex);
    CS_LOW();
    unsigned char result=wait_ready();
    CS_HIGH();
    if(result==0xff) return 0;
    else return -EFAULT;
}

template <class SPI>
SPISD<SPI>::SPISD(SPI& spi) : Device(Device::BLOCK), spi(spi)
{
    const int MAX_RETRY=20;//Maximum command retry before failing
    spi.setBitrate(100*1000); // 100 kHz SPI speed
    unsigned char resp;
    int i;
    for(i=0;i<MAX_RETRY;i++)
    {
        resp=send_cmd(CMD0,0);
        if(resp==1) break;
    }
    dbg("CMD0 required %d commands\n",i+1);
    
    if(resp!=1)
    {
        print_error_code(resp);
        dbgerr("Init failed\n");
        CS_HIGH();
        return; //Error
    }
    unsigned char n, cmd, ty=0, ocr[4];
    // Enter Idle state
    if(send_cmd(CMD8,0x1aa)==1)
    {   /* SDHC */
        for(n=0;n<4;n++) ocr[n]=spi_1_send(0xff);// Get return value of R7 resp
        if((ocr[2]==0x01)&&(ocr[3]==0xaa))
        {   // The card can work at vdd range of 2.7-3.6V
            for(i=0;i<MAX_RETRY;i++)
            {
                resp=send_cmd(ACMD41, 1UL << 30);
                if(resp==0)
                {
                    if(send_cmd(CMD58,0)==0)
                    {   // Check CCS bit in the OCR
                        for(n=0;n<4;n++) ocr[n]=spi_1_send(0xff);
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
                } else print_error_code(resp);
            }
            dbg("ACMD41 required %d commands\n",i+1);
        } else dbgerr("CMD8 failed\n");
    } else { /* SDSC or MMC */
        if(send_cmd(ACMD41,0)<=1)
        {
            ty=2;
            cmd=ACMD41; /* SDSC */
            dbg("SD card\n");
        } else {
            ty=1;
            cmd=CMD1;   /* MMC */
            dbg("MMC card\n");
        }
        for(i=0;i<MAX_RETRY;i++)
        {
            resp=send_cmd(cmd,0);
            if(resp==0)
            {
                if(send_cmd(CMD16,512)!=0)
                {
                    ty=0;
                    dbgerr("CMD16 failed\n");
                }
                break; //Exit from for
            } else print_error_code(resp);
        }
        dbg("CMD required %d commands\n",i+1);
    }

    if(ty==0)
    {
        CS_HIGH();
        return; //Error
    }
    cardType=ty;

    if(sd_status()<0)
    {
        dbgerr("Status error\n");
        CS_HIGH();
        return; //Error
    }

    CS_HIGH();
    //Configure the SPI interface to use at most 25MHz speed
    spi.setBitrate(25*1000*1000);

    dbg("Init done...\n");
}

} // namespace miosix
