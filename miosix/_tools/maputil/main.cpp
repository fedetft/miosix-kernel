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
#include <map>
#include "maputils.h"

using namespace std;

void usage()
{
    cerr<<R"(
maputil, an utility for extracting diffable data from map files.
Currently only prints info about the .text section.
Options:
    -f Print at the granularity of function names instead of translation units
    -s Sort output
    -t Print totals for each library (only if -f not specified)
    -n Do not print size, only list symbols/translation units
)";
    exit(1);
}

string libname(const string& input)
{
    auto libend=input.find('(');
    if(libend==string::npos) return ""; //No library
    return input.substr(0,libend);
}

int main(int argc, char *argv[])
{
    list<string> opts;
    for(int i=1;i<argc;i++) opts.push_back(argv[i]);
    if(opts.empty() || find(opts.begin(),opts.end(),"-h")!=opts.end()) usage();
    enum class Mode { TranslationUnits, Functions } mode=Mode::TranslationUnits;
    bool printSizes=true;
    bool sorted=false;
    bool totals=true;
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
    if(find(opts.begin(),opts.end(),"-t")!=opts.end())
    {
        opts.remove("-t");
        totals=true;
    }
    if(find(opts.begin(),opts.end(),"-n")!=opts.end())
    {
        opts.remove("-n");
        printSizes=false;
    }
    for(auto f : opts)
    {
        cout<<"Map file: "<<f<<"\n";
        Mapfile mapfile;
        switch(mode)
        {
            case Mode::TranslationUnits: mapfile=loadMapFileByTranslationUnits(f); break;
            case Mode::Functions: mapfile=loadMapFileByFunctionNames(f); break;
        }
        if(sorted) sort(begin(mapfile),end(mapfile));
        unsigned int totalSize=0;
        map<string,unsigned int> libs;
        for(auto& s : mapfile)
        {
            totalSize+=get<1>(s);
            cout<<get<0>(s);
            if(printSizes) cout<<"\t"<<get<1>(s);
            cout<<"\n";
            if(totals==false) continue;
            string lib=libname(get<0>(s));
            if(lib.empty()) continue;
            if(libs.count(lib)==0) libs[lib]=get<1>(s);
            else libs[lib]+=get<1>(s);
        }
        if(totals)
        {
            cout<<"\nLibrary sizes\n";
            for(auto l : libs) cout<<l.first<<"\t"<<l.second<<"\n";
        }
        cout<<"Total .text size = "<<totalSize<<"\n\n";
    }
}
