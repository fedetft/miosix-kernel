/***************************************************************************
 *   Copyright (C) 2008, 2009, 2010 by Terraneo Federico                   *
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

/* *****************************************************
Miosix boot system
Stage 2 boot process
This code will initialize system peripherals, and will
start the kernel and filesystem.
***************************************************** */

#include <cstdio>
#include <stdexcept>
/* Low level hardware functionalities */
#include "interfaces/bsp.h"
/* Miosix kernel */
#include "kernel.h"
#include "filesystem/filesystem.h"
#include "error.h"
#include "logging.h"
/* settings for miosix */
#include "config/miosix_settings.h"
#include "util/util.h"
#include "util/version.h"

using namespace std;

///<\internal Entry point for application code.
extern int main(int argc, char *argv[]);

namespace miosix {

/**
 * \internal
 * Performs the part of initialization that must be done after the kernel is
 * started, and finally calls main()
 */
static void mainLoader(void *argv)
{
    //If reaches here kernel is started, print Ok
    bootlog("Ok\r\n");

    #ifdef WITH_FILESYSTEM
    bootlog("Starting Filesystem... ");
    switch(Filesystem::instance().mount())
    {
        case 0:
            bootlog("Ok\r\n");
            break;
        case 1:
            bootlog("Failed\r\n");
            break;
        case 2:
            bootlog("No disk\r\n");
            break;
        case 3:
            bootlog("Error\r\n");
            break;
    }
    #endif //WITH_FILESYSTEM
    
    //Starting part of bsp that must be started after kernel
    bspInit2();

    #ifndef WITH_STDIN_BUFFER
    //Make stdin unbuffered to save some ram
    setbuf(stdin,NULL);
    #endif //WITH_STDIN_BUFFER

    #ifdef WITH_BOOTLOG
    iprintf("Available heap %d out of %d Bytes\n",
            MemoryProfiling::getCurrentFreeHeap(),
            MemoryProfiling::getHeapSize());
    #endif

    //Run application code
    #ifdef __NO_EXCEPTIONS
    main(0,NULL);
    #else //__NO_EXCEPTIONS
    try {
        main(0,NULL);
    } catch(std::exception& e)
    {
        errorHandler(PROPAGATED_EXCEPTION);
        errorLog("what():");
        errorLog(e.what());
        errorLog("\r\n");
    } catch(...)
    {
        errorHandler(PROPAGATED_EXCEPTION);
    }
    #endif //__NO_EXCEPTIONS
    
    //If main returns, shutdown
    shutdown();
}

/**
 * \internal
 * Performs the part of initialization that must be done before the kernel is
 * started, and starts the kernel.
 * This function is called by the stage 1 boot which is architecture dependent.
 */
extern "C" void _init()
{
    using namespace miosix;
    if(areInterruptsEnabled()) errorHandler(INTERRUPTS_ENABLED_AT_BOOT);
    IRQbspInit();
    //After IRQbspInit() serial port is initialized, so we can use BOOTLOG
    IRQbootlog(getMiosixVersion());
    IRQbootlog("\r\nStarting Kernel... ");
    //Create the first thread, and start the scheduler.
    Thread::create(mainLoader,MAIN_STACK_SIZE,MAIN_PRIORITY,NULL);
    startKernel();
    //Never reach here
}

} //namespace miosix