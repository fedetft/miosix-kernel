 /***************************************************************************
  *   Copyright (C) 2012 by Terraneo Federico                               *
  *   Copyright (C) 2024 by Daniele Cattaneo                                *
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
  *   You should have received a copy of the GNU General Public License     *
  *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
  ***************************************************************************/

// RAM testing code
// Useful to test the external RAM of a board.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <unistd.h>

const unsigned int ramBase=0xc0000000; //Tune this to the right value
const unsigned int ramSize=512*1024;   //Tune this to the right value

unsigned int randState=0x7ad3c099;

template<typename T> bool ramTest()
{
    const unsigned int loggingStep=std::min(ramSize/16,1048576U);
    unsigned checkSeed=randState;
    for(unsigned int i=ramBase;i<ramBase+ramSize;i+=sizeof(T))
    {
        volatile T *p=reinterpret_cast<T*>(i);
        T v=static_cast<T>(rand_r(&randState));
        *p=v;
    }
    sleep(10); //To check SDRAM retention ability
    int totalFailures=0,failures=0;
    for(unsigned int i=ramBase;i<ramBase+ramSize;i+=sizeof(T))
    {
        volatile T *p=reinterpret_cast<T*>(i);
        T ck=static_cast<T>(rand_r(&checkSeed));
        T v=*p;
        if(v!=ck) failures++;
        if((i+sizeof(T)-ramBase) % loggingStep == 0) {
            unsigned int startAddr=i+sizeof(T)-loggingStep;
            iprintf("%08x-%08x: %d mismatches\n",startAddr,i+sizeof(T)-1,failures);
            totalFailures+=failures;
            failures=0;
        }
    }
    return totalFailures!=0;
}

int main()
{
    for(;;)
    {
        iprintf("RAM test\nTesting word size transfers\n");
        if(ramTest<unsigned int>()) return 1;
        iprintf("Testing halfword size transfers\n");
        if(ramTest<unsigned short>()) return 1;
        iprintf("Testing byte size transfers\n");
        if(ramTest<unsigned char>()) return 1;
        iprintf("Ok\n");
    }
}
