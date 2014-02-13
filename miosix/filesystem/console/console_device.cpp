/***************************************************************************
 *   Copyright (C) 2013 by Terraneo Federico                               *
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

#include "console_device.h"
#include <errno.h>

namespace miosix {

//
// class TerminalDevice
//

TerminalDevice::TerminalDevice(intrusive_ref_ptr<Device> device)
        : FileBase(intrusive_ref_ptr<FilesystemBase>()), device(device),
          mutex(), echo(true), binary(false), skipNewline(false) {}

ssize_t TerminalDevice::write(const void *data, size_t len)
{
    if(binary) return device->writeBlock(data,len,0);
    //No mutex here to avoid blocking writes while reads are in progress
    const char *dataStart=static_cast<const char*>(data);
    const char *buffer=dataStart;
    const char *chunkstart=dataStart;
    //Try to write data in chunks, stop at every \n to replace with \r\n
    for(size_t i=0;i<len;i++,buffer++)
    {
        if(*buffer=='\n')
        {
            size_t chunklen=buffer-chunkstart;
            if(chunklen>0)
            {
                ssize_t r=device->writeBlock(chunkstart,chunklen,0);
                if(r<=0) return chunkstart==dataStart ? r : chunkstart-dataStart;
            }
            ssize_t r=device->writeBlock("\r\n",2,0);//Add \r\n
            if(r<=0) return chunkstart==dataStart ? r : chunkstart-dataStart;
            chunkstart=buffer+1;
        }
    }
    if(chunkstart!=buffer)
    {
        ssize_t r=device->writeBlock(chunkstart,buffer-chunkstart,0);
        if(r<=0) return chunkstart==dataStart ? r : chunkstart-dataStart;
    }
    return len;
}

ssize_t TerminalDevice::read(void *data, size_t len)
{
    if(binary)
    {
        ssize_t result=device->readBlock(data,len,0);
        if(echo && result>0) device->writeBlock(data,result,0);//Ignore write errors
        return result;
    }
    Lock<FastMutex> l(mutex); //Reads are serialized
    char *dataStart=static_cast<char*>(data);
    char *buffer=dataStart;
    //Trying to be compatible with terminals that output \r, \n or \r\n
    //When receiving \r skipNewline is set to true so we skip the \n
    //if it comes right after the \r
    for(int i=0;i<static_cast<int>(len);i++,buffer++)
    {
        ssize_t r=device->readBlock(buffer,1,0);
        if(r<=0) return buffer==dataStart ? r : buffer-dataStart;
        switch(*buffer)
        {
            case '\r':
                *buffer='\n';
                if(echo) device->writeBlock("\r\n",2,0);
                skipNewline=true;
                return i+1;    
            case '\n':
                if(skipNewline)
                {
                    skipNewline=false;
                    //Discard the \n because comes right after \r
                    i--; //Note that i may become -1, but it is acceptable.
                    buffer--;
                } else {
                    if(echo) device->writeBlock("\r\n",2,0);
                    return i+1;
                }
                break;
            case 0x7f: //Unix backspace
            case 0x08: //DOS backspace
            {
                int backward= i==0 ? 1 : 2;
                i-=backward; //Note that i may become -1, but it is acceptable.
                buffer-=backward;
                if(echo) device->writeBlock("\033[1D \033[1D",9,0);
                break;
            }
            default:
                skipNewline=false;
                if(echo) device->writeBlock(buffer,1,0);
        }
    }
    return len;
}

#ifdef WITH_FILESYSTEM

off_t TerminalDevice::lseek(off_t pos, int whence)
{
    return -EBADF;
}

int TerminalDevice::fstat(struct stat *pstat) const
{
    return device->fstat(pstat);
}

int TerminalDevice::isatty() const { return device->isatty(); }

#endif //WITH_FILESYSTEM

int TerminalDevice::ioctl(int cmd, void *arg)
{
    //TODO: trap ioctl to change terminal settings
    return device->ioctl(cmd,arg);
}

//
// class DefaultConsole 
//

DefaultConsole& DefaultConsole::instance()
{
    static DefaultConsole singleton;
    return singleton;
}

void DefaultConsole::IRQset(intrusive_ref_ptr<Device> console)
{
    //Note: should be safe to be called also outside of IRQ as set() calls
    //IRQset()
    atomic_store(&this->console,console);
    #ifndef WITH_FILESYSTEM
    atomic_store(&terminal,
        intrusive_ref_ptr<TerminalDevice>(new TerminalDevice(console)));
    #endif //WITH_FILESYSTEM
}

DefaultConsole::DefaultConsole() : console(new Device)
#ifndef WITH_FILESYSTEM
, terminal(new TerminalDevice(console))
#endif //WITH_FILESYSTEM      
{}

} //namespace miosix
