/***************************************************************************
 *   Copyright (C) 2012 by Luigi Rucco and Terraneo Federico               *
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

#include <iostream>
#include "postlinker.h"

using namespace std;

int main(int argc, char *argv[])
{
    int stackSize=-1;
    int ramSize=-1;
    string prog;
    bool strip=false;
    for(int i=1;i<argc;i++)
    {
        string opt(argv[i]);
        if(opt=="--strip-sectheader")
            strip=true;
        else if(opt.substr(0,10)=="--ramsize=")
            ramSize=atoi(opt.substr(10).c_str());
        else if(opt.substr(0,12)=="--stacksize=")
            stackSize=atoi(opt.substr(12).c_str());
        else
            prog=opt;
    }
    if(stackSize<=0 || ramSize<=0 || prog=="")
    {
        cerr<<"usage:"<<endl<<"mx-postlinker prog.elf --ramsize=<size>"
            <<" --stacksize=<size> [--strip-sectheader]"<<endl;
        return 1;
    }
    PostLinker pl(prog);
    if(strip) pl.removeSectionHeaders();
    pl.setMxTags(stackSize,ramSize);
    pl.writeFile();
    return 0;
}
