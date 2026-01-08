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

#include <fstream>
#include <stdexcept>
#include <regex>
#include <cxxabi.h>
#include "maputils.h"

using namespace std;

/**
 * Utility function to demangle a C++ symbol name
 * \param input string possibly being a C++ mangled symbol name
 * \return the demangled symbol name or the input string unchanged if the passed
 * string is not a C++ mangled name
 */
static string demangle(const string input)
{
    int status;
    char *demangled=abi::__cxa_demangle(input.c_str(),nullptr,nullptr,&status);
    if(status!=0) return input;
    string result=demangled;
    free(demangled);
    return result;
}

/**
 * Utility function to strip path from library names
 * \param input string possibly containing a path
 * \return the string without the path (if any)
 */
static string stripPath(const string& input)
{
    auto lastSlash=input.find_last_of("/");
    if(lastSlash!=string::npos) return input.substr(lastSlash+1);
    else return input;
}

Mapfile loadMapFileByFunctionNames(const string& filename)
{
    ifstream in(filename);
    if(!in) throw runtime_error("Can't open file");
    const regex textSectionBlock(R"(^ \.text.*)");
    const regex symbolName(R"(^\s+(0x[0-9a-fA-F]+)\s+(\S+))");
    const regex symbolSize(R"(^\s+0x[0-9a-fA-F]+\s+0x[0-9a-fA-F]+\s+\S+)");
    const regex anySectionBlockWithAddr(R"(^ \.\S+\s+(0x[0-9a-fA-F]+)\s+.*)");
    // Algorithm: we always compute a symbol size by difference between the next
    // symbol address and the current symbol address. Thus when we find a symbol
    // we "open" it and store its name and start address in prevName/prevAddress
    // then when we find the next symbol we "close" the previous one. Since we
    // only care about the .text section, we use validContext to only open
    // symbols in .text or .text.* sections. We exit the validContext every time
    // we find a line we don't understand, so we also need to account for code
    // compiled with -ffunction-section adding one more line per symbol.
    bool validContext=false;
    unsigned int prevAddr;
    string prevName;
    Mapfile result;
    string line;
    auto closeSymbol=[&](unsigned int addr) {
        if(prevName.empty()) return; //No open symbol
        unsigned int size=addr-prevAddr;
        //Only save if size > 0, prevents discarded/duplicated symbols
        if(size>0) result.push_back(make_tuple(demangle(prevName),size));
        prevName.clear(); //Mark symbols as closed
    };
    while(getline(in,line))
    {
        if(line=="Linker script and memory map") break; //Filter initial garbage
    }
    while(getline(in,line))
    {
        smatch sm;
        if(regex_match(line,textSectionBlock))
        {
            validContext=true;
        } else if(regex_match(line,sm,symbolName)) {
            unsigned int addr=stoul(sm[1],nullptr,16);
            closeSymbol(addr);
            if(validContext) //Do we need to open another symbol?
            {
                prevAddr=addr;
                prevName=sm[2];
            }
        } else if(regex_match(line,symbolSize)) {
            //Ignore these lines
        } else if(regex_match(line,sm,anySectionBlockWithAddr)) {
            //Prevent error in computing last .text symbol size
            closeSymbol(stoul(sm[1],nullptr,16));
            validContext=false;
        } else {
            validContext=false; //Unknown line understood as .text block end
        }
    }
    return result;
}

Mapfile loadMapFileByTranslationUnits(const string& filename)
{
    ifstream in(filename);
    if(!in) throw runtime_error("Can't open file");
    const regex textNoAddr(R"(^ \.text.*)");
    const regex textWithAddr(R"(^ \.text\S*\s+0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)\s+(.*))");
    const regex symbolName(R"(^\s+0x[0-9a-fA-F]+\s+\S+)");
    const regex symbolSize(R"(^\s+0x[0-9a-fA-F]+\s+(0x[0-9a-fA-F]+)\s+(\S+))");
    // Algorithm: just like in loadMapFileByFunctionNames we use validContext to
    // remember whether we're parsing .text or some other section. Then we just
    // read the data for translation units without -ffunction-section, and for
    // those with -ffunction-section we sum the sizes for the repeated entries
    // taking advantage of them being printed consecutively in the file
    bool validContext=false;
    Mapfile result;
    string line;
    auto addOrAppend=[&](const string& name, int size) {
        //If name is the same as previous entry append size, else add
        if(!result.empty() && get<0>(result.back())==name)
            get<1>(result.back())+=size;
        else result.push_back(make_tuple(name,size));
    };
    while(getline(in,line))
    {
        if(line=="Linker script and memory map") break; //Filter initial garbage
    }
    while(getline(in,line))
    {
        smatch sm;
        if(regex_match(line,sm,textWithAddr)) {
            validContext=true;
            string name=stripPath(sm[2]);
            unsigned int size=stoul(sm[1],nullptr,16);
            if(size>0) addOrAppend(name,size);
        } else if(regex_match(line,textNoAddr)) {
            //Must check for textNoAddr after textWithAddr as they overlap
            validContext=true;
        } else if(regex_match(line,symbolName)) {
            //Ignore these lines
        } else if(regex_match(line,sm,symbolSize)) {
            string name=stripPath(sm[2]);
            unsigned int size=stoul(sm[1],nullptr,16);
            if(validContext && size>0) addOrAppend(name,size);
        } else {
            validContext=false; //Unknown line understood as .text block end
        }
    }
    return result;
}
