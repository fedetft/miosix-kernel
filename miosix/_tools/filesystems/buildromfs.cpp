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
#include <cstring>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "romfs_types.h"

using namespace std;
using namespace std::filesystem;

/**
 * \param an unsigned int
 * \return the same int forced into little endian representation
 */
unsigned int toLittleEndian(unsigned int x)
{
    static bool first=true, little;
    union {
        unsigned int a;
        unsigned char b[4];
    } endian;
    if(first)
    {
        endian.a=0x12;
        little=endian.b[0]==0x12;
        first=false;
    }
    if(little) return x;
    endian.a=x;
    swap(endian.b[0],endian.b[3]);
    swap(endian.b[1],endian.b[2]);
    return endian.a;
}

int main(int argc, char *argv[])
{
    if(argc!=3)
    {
        cerr<<"Miosix buildromfs utility v1.01"<<endl
            <<"use: buildromfs <source directory> <target file>"<<endl;
        return 1;
    }
    if(is_directory(argv[1])==false)
    {
        cerr<<argv[1]<<": source directory not found"<<endl;
        return 1;
    }

    //Traverse the input directory storing path of files
    path inDir(argv[1]);
    directory_iterator end;
    vector<path> files;
    for(directory_iterator it(inDir);it!=end;++it)
    {
        if(is_directory(it->status()))
        {
            cerr<<"Warning: ignoring subdirectory \""<<it->path()<<"\""<<endl;
        } else if(it->path().filename().string().length()>romFsFileMax)
        {
            cerr<<"Error: file name \""<<it->path().filename()<<"\" is too long"<<endl;
            return 1;
        } else files.push_back(it->path());
    }

    // Constructing the filesystem header
    RomFsHeader header;
    memset(&header,0,sizeof(RomFsHeader));
    memcpy(header.marker,"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww",32);
    strncpy(header.fsName,"RomFs 1.01",16);
    strncpy(header.osName,"Miosix",8);
    header.fileCount=toLittleEndian(files.size());

    // Building the output filesystem
    ofstream out(argv[2],ios::binary);
    if(!out)
    {
        cerr<<"Can't open otput file"<<endl;
        return 1;
    }
    out.write((char*)&header,sizeof(RomFsHeader)); //Write header
    int start=sizeof(RomFsHeader)+sizeof(RomFsFileInfo)*files.size();
    out.seekp(start,ios::beg); //Skip inodes and write individual file contents
    vector<RomFsFileInfo> fileInfos;
    for(int i=0;i<files.size();i++)
    {
        ifstream in(files[i].string().c_str(),ios::binary);
        RomFsFileInfo file;
        memset(&file,0,sizeof(RomFsFileInfo));
        file.start=toLittleEndian(start);
        in.seekg(0,ios::end);
        file.length=toLittleEndian(in.tellg());
        start+=in.tellg();
        strcpy(file.name,files[i].filename().c_str());
        fileInfos.push_back(file);
        in.seekg(0,ios::beg);
        out<<in.rdbuf();
    }

    out.seekp(sizeof(RomFsHeader),ios::beg); //Get back to after the header and write inodes
    for(int i=0;i<fileInfos.size();i++)
        out.write((char*)&fileInfos[i],sizeof(RomFsFileInfo));
    return 0;
}
