/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#ifndef PROCESS_H
#define	PROCESS_H

#include <vector>
#include <map>
#include <sys/types.h>
#include "kernel.h"
#include "elf_program.h"
#include "config/miosix_settings.h"

#ifdef WITH_PROCESSES

namespace miosix {

class Process
{
public:
    
    static Process *create(const ElfProgram& program);
    
private:
    
    static void *start(void *argv);
    
    static pid_t PKgetNewPid();
    
    Process(const Process&);
    Process& operator= (const Process&);
    
    ProcessImage image; ///<The RAM image of a process
    std::vector<Thread *> threads; ///<Threads that belong to the process
    
    ///Maps the pid to the Process instance
    static std::map<pid_t,Process *> processes;
    ///Used to assign a new pid to a process
    static pid_t pidCounter;
};

} //namespace miosix

#endif //WITH_PROCESSES

#endif //PROCESS_H
