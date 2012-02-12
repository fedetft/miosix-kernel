/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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
 * DCC support inspired by dcc_stdio.c in OpenOCD
 */

#include <algorithm>
#include <cstring>
#include "pthread.h"
#include "miosix.h"
#include "dcc.h"

using namespace std;

/**
 * Send data to the host
 */
static void send(unsigned char c)
{
    const unsigned int busy=1;
    while(CoreDebug->DCRDR & busy) ;
    CoreDebug->DCRDR=(static_cast<unsigned int>(c)<<8) | busy;
}

/**
 * Send a line of text to the host.
 * OpenOCD will insert a \n after each line, unfortunately,
 * as this complicates things.
 */
void debugStr(const char *str, int length)
{
    //If not being debugged, don't print anything
    if((CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)==0) return;

    if(length<0) length=strlen(str);
    if(length>0 && str[0]=='\r') str++, length--; //TODO: better \r\n removal
    if(length>0 && str[0]=='\n') str++, length--;
    if(length==0) return;
    send(1); //1=sending a string
    send(0);
    send(length & 0xff);
    send(length>>8);
    for(int i=0;i<length;i++) send(*str++);
    //OpenOCD expects data in 4 byte blocks, so send trailing zeros to align
    int remaining=4-(length & 3);
    if(remaining<4) for(int i=0;i<remaining;i++) send(0);
}

/// Mutex for locking concurrent access to debug channel
static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

void debugWrite(const char *str, unsigned int len)
{
    pthread_mutex_lock(&mutex);
    debugStr(str,len);
    pthread_mutex_unlock(&mutex);
}

void IRQdebugWrite(const char *str)
{
    //Actually, there is a race condition if IRQdebugWrite is called from an
    //interrupt while the main code is printing with debugWrite, but it is
    //not such an issue since IRQdebugWrite is called only
    //- at boot before the kernel is started, so there are no other threads
    //- in case of a serious error, to print what went wrong before rebooting
    //In the second case, which is rare, the data may not be printed correctly
    debugStr(str,-1);
}
