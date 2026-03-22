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

#include <istream>
#include <ostream>
#include <cassert>

/**
 * A binary image class, makes it easy to put/get structs at given offsets
 * Data is stored in a generic iostream, so it can be written to file (fstream)
 * or memory (stringstream)
 * \tparam T integer size for the image. It is expected that T will be unsigned
 * int for filesystems structurally bound to images < 4GByte, and unsigned long
 * long for larger filesystems.
 */
template<typename T>
class Image
{
public:
    /**
     * Constructor
     * \param io iostream that will contain the image data, stored by reference,
     * must remain valid for the entire lifetime of this class
     */
    Image(std::iostream& io) : io(io) {}

    /**
     * Append a struct to the image
     * \tparam U struct type
     * \param data struct to add
     * \param alignment pad the image to the desired alignment before
     * storing the struct. No padding is added after the struct
     * \return offset in bytes from the image start where the struct was placed
     */
    template<typename U>
    unsigned int append(const U& data, unsigned int alignment=1)
    {
        align(alignment);
        unsigned int offset=totalSize;
        io.write(reinterpret_cast<const char*>(&data),sizeof(U));
        totalSize=io.tellp();
        return offset;
    }

    /**
     * Append a stream to the image
     * \param is istream to add
     * \param alignment pad the image to the desired alignment before
     * storing the stream. No padding is added after the stream
     * \return offset in bytes from the image start where the struct was placed
     */
    unsigned int appendStream(const std::istream& is, unsigned int alignment=1)
    {
        align(alignment);
        unsigned int offset=totalSize;
        io<<is.rdbuf();
        // Clear the fail bit on the output if the input was empty
        if(io.fail() && is.good()) io.clear();
        totalSize=io.tellp();
        return offset;
    }

    /**
     * Append a string of arbitrary length, nul terminated to the image.
     * \param s string to add
     * \param nulTerminate if true, add '\0' afther the string
     * \param alignment pad the image to the desired alignment before
     * storing the string. No padding is added after the string
     * \return offset in bytes from the image start where the struct was placed
     */
    unsigned int appendString(const std::string& s, bool nulTerminate=true,
                              unsigned int alignment=1)
    {
        align(alignment);
        unsigned int offset=totalSize;
        io<<s;
        if(nulTerminate) io<<'\0';
        totalSize=io.tellp();
        return offset;
    }

    /**
     * Pad the image till the total image size is aligned as desired
     */
    void align(unsigned int alignment)
    {
        unsigned int newTotalSize=(totalSize+alignment-1)/alignment*alignment;
        io.seekp(0,std::ios::end);
        //Padding with 0xff makes it easier to see where padding is placed
        io<<std::string(newTotalSize-totalSize,'\xff');
        totalSize=io.tellp();
        assert(totalSize==newTotalSize);
    }

    /**
     * Put a struct at the desired offset. Unlike append the image must already
     * have enough storage to contain the entire struct. Previously present data
     * is overwritten
     * \tparam U struct type
     * \param data struct to add
     * \param offset offset in bytes from the image start
     */
    template<typename U>
    void put(const U& data, unsigned int offset)
    {
        assert(offset+sizeof(U)<=totalSize);
        io.seekp(offset);
        io.write(reinterpret_cast<const char*>(&data),sizeof(U));
    }

    /**
     * Get a struct from the desired offset
     * \tparam U struct type
     * \param offset offset in bytes from the image start
     * \return struct read from the image
     */
    template<typename U>
    U get(unsigned int offset)
    {
        assert(offset+sizeof(U)<=totalSize);
        io.seekg(offset);
        U result;
        io.read(reinterpret_cast<char*>(&result),sizeof(U));
        return result;
    }

    /**
     * \return the image size
     */
    unsigned int size() const { return totalSize; }

private:
    std::iostream& io;        ///< Backing storage
    unsigned int totalSize=0; ///< Image size so far
};
