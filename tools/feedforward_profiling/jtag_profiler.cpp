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

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include "fdstream.h"
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

int main()
{
    //Open script file
    ifstream script("gdb_init.script");
    if(!script) return 1;

    //Spawn gdb with pipes controlling standard input and output
    int gdbstdout[2];
    int gdbstdin[2];
    pipe(gdbstdin);
    pipe(gdbstdout);
    if(fork()==0)
    {
        close(0);
        close(1);
        close(gdbstdin[1]);
        close(gdbstdout[0]);
        dup2(gdbstdin[0],0);
        dup2(gdbstdout[1],1);
        execlp("arm-miosix-eabi-gdb","arm-miosix-eabi-gdb","main.elf",(char*)0);
    }
    close(gdbstdout[1]);
    close(gdbstdin[0]);
    fdistream childout(gdbstdout[0]);
    fdostream childin(gdbstdin[1]);

    //Read script file and send it to gdb
    string line;
    while(getline(script,line))
    {
        if(line.empty() || line.at(0)=='#' || line.at(0)=='\r') continue;
        //Parse the @<number> <command> syntax to repeat a command multiple time
        if(line.at(0)=='@')
        {
            stringstream ss(line);
            ss.get();
            int num;
            ss>>num;
            ss.get();
            getline(ss,line);
            if(num!=0)
                for(int i=0;i<num;i++) childin<<line<<endl;
        } else childin<<line<<endl;
    }
    childin<<"quit"<<endl<<"y"<<endl; //Close gdb

    //Read and parse output
    regex dataline("[[:digit:]]+:.*");
    regex continuation("  [[:alnum:]].*");
    bool skip=true;
    while(getline(childout,line))
    {
        if(regex_match(line,dataline)) { skip=false; cout<<endl<<line; }
        if(regex_match(line,continuation) && !skip) cout<<line;
    }
    cout<<endl;
    
    int status;
    wait(&status);
}
