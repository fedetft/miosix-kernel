 /***************************************************************************
  *   Copyright (C) 2024 by Terraneo Federico                               *
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

#pragma once

#include <iostream>
#include <filesystem>
#include <list>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * Stores information about either a file, symlink or directory
 */
class FilesystemEntry
{
public:
    /**
     * Constructor, build an empty entry
     */
    FilesystemEntry() {}

    /**
     * \return true if the entry is a file
     */
    bool isFile() const { return (mode & S_IFMT) == S_IFREG; }

    /**
     * \return true if the entry is a symlink
     */
    bool isLink() const { return (mode & S_IFMT) == S_IFLNK; }

    /**
     * \return true if the entry is a directory
     */
    bool isDirectory() const { return (mode & S_IFMT) == S_IFDIR; }

    /**
     * Add subentries to a directory, keeping them sorted
     * \param entry entry to add. The entry is moved into the tree, not copied!
     * Remember to use std::move
     * \code
     * entry.addEntryToDirectory(std::move(subentry));
     * \endcode
     */
    void addEntryToDirectory(FilesystemEntry&& entry)
    {
        assert(isDirectory());
        //Insertion sort alphabetically, directories first
        for(auto it=begin(directoryEntries);it!=end(directoryEntries);++it)
        {
            if(!entry.isDirectory() && it->isDirectory()) continue;
            if((entry.isDirectory()^it->isDirectory())==0 && entry.name>it->name) continue;
            directoryEntries.insert(it,std::move(entry));
            return;
        }
        directoryEntries.push_back(std::move(entry));
    }

    // Heavy object not meant to be copyable, only move assignable.
    // This declaration implicitly deletes copy constructor and copy assignment
    // in a way that does not upset std::list.
    FilesystemEntry(FilesystemEntry&&)=default;
    FilesystemEntry& operator=(FilesystemEntry&&)=default;

    unsigned short mode=0, uid=0, gid=0; ///< unix file properties
    std::string name; ///< Name the entry will have in the RomFs image

    /// If entry is a file, path in the source filesystem
    /// If entry is a symlink, link target
    std::string path;

    /// If entry is directory, its content
    /// Do not add entries directly, use addEntryToDirectory instead
    std::list<FilesystemEntry> directoryEntries;
};

/**
 * Recursively build a FilesystemEntry tree starting from a root directory
 * \param entry an empty FilesystemEntry that will become the root of the tree
 * \param directory path to a directory on the host machine
 * \param uidGidOverride if >=0 set all files uid and gid to the given number
 */
inline int buildFromDir(FilesystemEntry& entry,
                        const std::filesystem::path& directory, int uidGidOverride=-1)
{
    using namespace std;
    using namespace std::filesystem;

    struct stat st;
    if(lstat(directory.c_str(),&st)!=0 || (st.st_mode & S_IFMT) != S_IFDIR)
    {
        cerr<<directory<<": not found or not a directory"<<endl;
        return 1;
    }
    entry.mode=st.st_mode;
    entry.uid=uidGidOverride>=0 ? uidGidOverride : st.st_uid;
    entry.gid=uidGidOverride>=0 ? uidGidOverride : st.st_gid;
    entry.name=directory.filename();
    directory_iterator end;
    for(directory_iterator it(directory);it!=end;++it)
    {
        FilesystemEntry subentry;
        bool good=true;
        if(is_directory(it->status()))
        {
            if(int result=buildFromDir(subentry, it->path(), uidGidOverride))
                return result;
        } else {
            struct stat st;
            if(lstat(it->path().c_str(),&st)!=0) throw std::runtime_error("stat");
            subentry.mode=st.st_mode;
            subentry.uid=uidGidOverride>=0 ? uidGidOverride : st.st_uid;
            subentry.gid=uidGidOverride>=0 ? uidGidOverride : st.st_gid;
            subentry.name=it->path().filename();
            switch(st.st_mode & S_IFMT)
            {
                case S_IFREG:
                    subentry.path=it->path();
                    break;
                case S_IFLNK:
                    subentry.path=read_symlink(it->path());
                    break;
                default:
                    cerr<<it->path()<<": unhandled file type found, skip"<<endl;
                    good=false;
            }
        }
        if(good) entry.addEntryToDirectory(std::move(subentry));
    }
    return 0;
}
