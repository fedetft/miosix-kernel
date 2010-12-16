/***************************************************************************
 *   Copyright (C) 2008 2009 2010 by Terraneo Federico                     *
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

#ifndef CONFLICT_TABLE_H
#define	CONFLICT_TABLE_H

#include "config/miosix_settings.h"

namespace miosix {

/**
 * This class is used to keep information about open files.
 * Given the file name, this class can tell if it is currently open for reading
 * or writing. Also, for files opened for reading a reference count is kept
 * because it is possible to open the same file more times.
 * The goal of this class is to avoid opening the same file more than once if it
 * is already opened for writing, a thing that can cause crashes and/or
 * filesystem corruption.
 * Wish I could use std::set to implement this...
 */
class ConflictTable
{
public:

    /**
     * A file is considered in one of these three states
     */
    enum OpenState
    {
        NOT_OPEN, ///< File is not currently open
        OPEN_READ,///< File is open for reading
        OPEN_WRITE///< File is open for writing (or appending)
    };

    /**
     * Given a file name, return its state
     * \param filename the file name, including its path (if present)
     * Note that if the path is absent a / is added to the file name because
     * file.txt and /file.txt are the same file.
     * \return the file state
     */
    OpenState isOpen(const char *filename) const;

    /**
     * Add a file to the conflict table. To be called when a file is opened.
     * \param fd file descriptor, it is stored in the table and used to identify
     * the file when closing it
     * \param filename the file name, including its path (if present)
     * Note that if the path is absent a / is added to the file name because
     * file.txt and /file.txt are the same file.
     * \param mode can be either OPEN_READ or OPEN_WRITE, NOT_OPEN is not allowed
     */
    void open(int fd, const char *filename, OpenState mode);

    /**
     * To be called when a file is closed.
     * If the file is opened more times its reference count is decreased,
     * otherwise it is removed from the conflict table.
     * \param fd file descriptor of the file to remove.
     */
    void close(int fd);

private:

    /**
     * Hash function
     * \return index of table 0<= \result < MAX_OPEN_FILES
     */
    static unsigned int hash(const char *filename);

    class TableEntry
    {
        /**
         * Constructor, assign default values
         */
        TableEntry() : write(false), refcount(0), fd(0), filename(0) {}

        /**
         * \param filename file to check
         * \return true if this TableEntry corresponds to filename
         */
        bool isThisFile(const char *filename)
        {
            if(isEmpty()) return false;
            if(!strcmp(filename,this->filename)) return true;
            return false;
        }

        /**
         * \return true if this FileEntry is empty
         */
        bool isEmpty()
        {
            return filename==0;
        }

        bool write;///<\internal true if file open for writing
        unsigned short refcount;///<\internal reference count
		//This is where there is the problem: we need to store a vector of fd otherwise
		//there's no way to delete them...
        int fd;///<\internal file descriptor
        const char *filename;///<\internal file name + path, NULL if empty
    };

    TableEntry fileData[MAX_OPEN_FILES];
};

} //namespace miosix

#endif	// CONFLICT_TABLE_H
