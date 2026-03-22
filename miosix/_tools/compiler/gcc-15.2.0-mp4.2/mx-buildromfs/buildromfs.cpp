 /***************************************************************************
  *   Copyright (C) 2011-2024 by Terraneo Federico                          *
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
 
 /*
  * buildromfs.cpp
  * Store a set of files in a lightweight read-only filesystem.
  * The output is a binary file suitable to be stored in a flash memory.
  */
 
#include <iostream>
#include <fstream>
#include "tree.h"
#include "mkromfs.h"

using namespace std;

int main(int argc, char *argv[])
{
    if(argc<4)
    {
        cerr<<"Miosix buildromfs utility v2.00"<<endl
            <<"use: buildromfs <target file> --from-directory <source directory>"<<endl;
        return 1;
    }

    // Build the tree of files and directories that compose the image
    string mode=argv[2];
    FilesystemEntry root;
    int uidGidOverride=0; //TODO: for now force all uid and gid to 0 (root)
    if(mode=="--from-directory")
    {
        if(int result=buildFromDir(root,argv[3],uidGidOverride)!=0) return result;
    } else {
        cerr<<argv[2]<<": unsupported option"<<endl;
        return 1;
    }

    // Open the output image
    fstream io(argv[1], ios::in | ios::out | ios::trunc | ios::binary);
    if(!io)
    {
        cerr<<"Can't open output file "<<argv[1]<<endl;
        return 1;
    }

    // Build the image and write it to file
    MkRomFs img(io,root);
    cout<<"RomFs size "<<img.size()<<endl;
    return 0;
}
