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
#include <interfaces/console.h> //FIXME: remove

namespace miosix {

//
// class NullInterruptOutputDevice
//

ssize_t NullConsoleDevice::write(const void *data, size_t len)
{
    return len;
}

ssize_t NullConsoleDevice::read(void *data, size_t len)
{
    return -EBADF;
}

off_t NullConsoleDevice::lseek(off_t pos, int whence)
{
    switch(whence)
    {
        case SEEK_SET:
        case SEEK_CUR:
        case SEEK_END:
            return -EBADF;
        default:
            return -EINVAL;
    }
}

int NullConsoleDevice::fstat(struct stat *pstat) const
{
    return -EBADF; //TODO
}

void NullConsoleDevice::IRQwrite(const char* str) {}

bool NullConsoleDevice::IRQtxComplete()
{
    return true;
}

//
// class ConsoleAdapter
//

// FIXME temporary -- begin
ssize_t ConsoleAdapter::write(const void *data, size_t len)
{
    Console::write(reinterpret_cast<const char*>(data),len);
    return len;
}
ssize_t ConsoleAdapter::read(void *data, size_t len)
{
    char *d=reinterpret_cast<char*>(data);
    for(size_t i=0;i<len;i++) *d++=Console::readChar();
    return len;
}
off_t ConsoleAdapter::lseek(off_t pos, int whence) { return -EBADF; }
int ConsoleAdapter::fstat(struct stat *pstat) const
{
    memset(pstat,0,sizeof(struct stat));
    pstat->st_mode=S_IFCHR;//Character device
    pstat->st_blksize=0; //Defualt file buffer equals to BUFSIZ
    return 0;
}
int ConsoleAdapter::isatty() const { return 1; }
void ConsoleAdapter::IRQwrite(const char* str) { Console::IRQwrite(str); }
bool ConsoleAdapter::IRQtxComplete() { return Console::IRQtxComplete(); }
// FIXME temporary -- end

//
// class DefaultConsole 
//

DefaultConsole& DefaultConsole::instance()
{
    static DefaultConsole singleton;
    return singleton;
}

void DefaultConsole::IRQset(intrusive_ref_ptr<ConsoleDevice> console)
{
    atomic_store(&rawConsole,console);
    atomic_store(&terminal,
        intrusive_ref_ptr<TerminalDevice>(new TerminalDevice(rawConsole)));
}

void DefaultConsole::checkInit()
{
    if(!rawConsole)
        IRQset(intrusive_ref_ptr<ConsoleDevice>(new NullConsoleDevice));
}

} //namespace miosix
