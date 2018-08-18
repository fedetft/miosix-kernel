/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
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
 * This is a stub program for the program that will decode the logged data.
 * Fill in the TODO to make it work. 
 */

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <tscpp/stream.h>

//TODO: add here include files of serialized classes
#include "../LogStats.h"
#include "../ExampleData.h"

using namespace std;
using namespace tscpp;

int main(int argc, char *argv[])
try {
    TypePoolStream tp;
    //TODO: Register the serialized classes
    tp.registerType<LogStats>([](LogStats& t){ t.print(cout); cout<<'\n'; });
    tp.registerType<ExampleData>([](ExampleData& t){ t.print(cout); cout<<'\n'; });

    if(argc!=2) return 1;
    ifstream in(argv[1]);
    in.exceptions(ios::eofbit);
    UnknownInputArchive ia(in,tp);
    for(;;) ia.unserialize();
} catch(exception&) {
    return 0;
}
