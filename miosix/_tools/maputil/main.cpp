/***************************************************************************
 *   Copyright (C) 2026 by Terraneo Federico                               *
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
#include <algorithm>
#include <list>
#include "maputils.h"

using namespace std;

void usage()
{
    cerr<<R"(
maputil, an utility for extracting diffable data from map files.
Currently only prints info about the .text section.
Options:
    -f Print at the granularity of function names)instead of translation units
    -s Sort output
    -n Do not print size, only list symbols/translation units
)";
    exit(1);
}

int main(int argc, char *argv[])
{
    list<string> opts;
    for(int i=1;i<argc;i++) opts.push_back(argv[i]);
    if(opts.empty() || find(opts.begin(),opts.end(),"-h")!=opts.end()) usage();
    enum class Mode { TranslationUnits, Functions } mode=Mode::TranslationUnits;
    bool printSizes=true;
    bool sorted=false;
    if(find(opts.begin(),opts.end(),"-f")!=opts.end())
    {
        opts.remove("-f");
        mode=Mode::Functions;
    }
    if(find(opts.begin(),opts.end(),"-s")!=opts.end())
    {
        opts.remove("-s");
        sorted=true;
    }
    if(find(opts.begin(),opts.end(),"-n")!=opts.end())
    {
        opts.remove("-n");
        printSizes=false;
    }
    for(auto f : opts)
    {
        cout<<"Map file: "<<f<<"\n";
        Mapfile map;
        switch(mode)
        {
            case Mode::TranslationUnits: map=loadMapFileByTranslationUnits(f); break;
            case Mode::Functions: map=loadMapFileByFunctionNames(f); break;
        }
        if(sorted) sort(begin(map),end(map));
        unsigned int totalSize=0;
        for(auto& s : map)
        {
            totalSize+=get<1>(s);
            cout<<get<0>(s);
            if(printSizes) cout<<"\t"<<get<1>(s);
            cout<<"\n";
        }
        cout<<"Total .text size = "<<totalSize<<"\n\n";
    }
}
