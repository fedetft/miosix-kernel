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

#ifndef SYSCALLS_TYPES_H
#define	SYSCALLS_TYPES_H

#include "error.h"

namespace __cxxabiv1
{
struct __cxa_exception; //A forward declaration of this one is enough

/*
 * This struct was taken from libsupc++/unwind-cxx.h Unfortunately that file
 * is not deployed in the gcc installation so we can't just #include it.
 * It is required on a per-thread basis to make C++ exceptions thread safe.
 */
struct __cxa_eh_globals
{
    __cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
    //Should be __ARM_EABI_UNWINDER__ but that's only usable inside gcc
    #ifdef __ARM_EABI__
    __cxa_exception* propagatingExceptions;
    #endif //__ARM_EABI__
};

} //namespace __cxxabiv1

namespace miosix {

//Forward declaration of a class to hide accessors to ExceptionHandlingData
class ExceptionHandlingAccessor;

/**
 * This is a wrapper class that contains all per-thread data required to make
 * C++ exceptions thread safe, irrespective of the ABI.
 */
class ExceptionHandlingData
{
public:
    /**
     * Constructor, initializes the exception related data to their default
     * value
     */
    ExceptionHandlingData()
    {
        eh_globals.caughtExceptions=0;
        eh_globals.uncaughtExceptions=0;
        #ifdef __ARM_EABI__
        eh_globals.propagatingExceptions=0;
        #else //__ARM_EABI__
        sjlj_ptr=0;
        #endif //__ARM_EABI__
    }

    /**
     * Destructor, checks that no memory is leaked (should never happen)
     */
    ~ExceptionHandlingData()
    {
        if(eh_globals.caughtExceptions!=0) errorHandler(UNEXPECTED);
    }

private:
    /// With the arm eabi unwinder, this is the only data structure required
    /// to make C++ exceptions threadsafe.
    __cxxabiv1::__cxa_eh_globals eh_globals;
    #ifndef __ARM_EABI__
    /// On other no arm architectures, it looks like gcc requires also this
    /// pointer to make the unwinder thread safe
    void *sjlj_ptr;
    #endif //__ARM_EABI__

    friend class ExceptionHandlingAccessor;
};

} //namespace miosix

#endif //SYSCALLS_TYPES_H
