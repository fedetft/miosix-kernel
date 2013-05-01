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

/***********************************************************************
* filesystem.h Part of the Miosix Embedded OS.
* Wrapper of FatFs filesystem library for Miosix.
************************************************************************/

#ifndef FILESYSYTEM_H
#define FILESYSYTEM_H

#include "config/miosix_settings.h"

//Only if WITH_FILESYSTEM in miosix_setings.h is defined we need to include
//the code in this file
#ifdef WITH_FILESYSTEM

#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "ff.h"
#include "kernel/sync.h"


namespace miosix {

/**
 * \addtogroup Filesystem
 * \{
 */

/**
 * This class allows low level access to the filesystem.
 * It contains member functions to mount and unmount the filesystem, and to
 * check if it is mounted.
 * There are also functions to perform file access that are primarily meant to
 * be used by the C/C++ standard library integration layer (syscalls.cpp).
 */
class Filesystem
{
public:
    /**
     * \return the instance of the filesystem (singleton).
     */
    static Filesystem& instance();

    /**
     * Try to mount the flesystem. Might take a long time
     * (~ 1 sec if no errors, up to 3 sec in case of errors)
     * \return
     * - 0 = on success
     * - 1 = in case of errors
     * - 2 = if no drive was found (for example there is no uSD card)
     * - 3 = if the filesystem is already mounted
     */
    char mount();

    /**
     * Unmounts the filesystem if it is mounted.
     * All files that are open at this point are closed, and their file
     * descriptors are invalidated.
     * Allows safely removing the drive (example uSD card)
     */
    void umount();

    /**
     * \return true if the filesystem is mounted
     */
    bool isMounted() const
    {
        return mounted;
    }

    /**
     * \internal
     * Open a file.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * Note that trying to open the same file more than one time, where one
     * or more times for write access will cause undefined behaviour and must
     * avoided. The user code is responsible for this.
     * \param name file name + path.
     * If the file is in the root directory either "file.ext" or "/file.ext" are
     * allowed.
     * \param flags open mode flags as defined in sys/fcntl.h. Currently only
     * these flags are supported:
     * - _FREAD read enabled
     * - _FWRITE write enabled
     * - _FAPPEND append (writes guaranteed at the end)
     * - _FTRUNC open with truncation
     * \param mode, file open permissions, ignored on FAT32, reserved for future
     * use if some unix-like filesystem is added.
     * \return a file descriptor that can be later used with file handling
     * member functions, or a negative number if any error.
     * File descriptor numbers start from 3 because 0, 1 and 2 are reserved for
     * stdion,stdout and stderr.
     */
    int openFile(const char *name, int flags, int mode);

    /**
     * \internal
     * Read some data from a file.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param fd file descriptor, obtained with openFile
     * \param buffer read buffer. Read data is stored here
     * \param bufSize size of buffer
     * \return On success it returns the number of bytes read, on failure it
     * returns -1.
     */
    int readFile(int fd, void *buffer, int bufSize);

    /**
     * \internal
     * Write data to a file.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param fd file descriptor, obtained with openFile
     * \param buffer data to write to the file
     * \param bufSize size of buffer
     * \return On success it returns the number of bytes written, on failure it
     * returns -1.
     */
    int writeFile(int fd, const void *buffer, int bufSize);

    /**
     * \internal
     * Move file position pointer.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param fd file descriptor, obtained with openFile
     * \param pos a positive or negative number whose meaning depends on whence
     * \param whence, can be one of these three constants
     * - SEEK_SET newFileOffset = pos, pos >=0
     * - SEEK_CUR newFileOffset = oldFileOffset + pos
     * - SEEK_END newFileOffset = FileSize + pos
     * \return On success it returns the offset in bytes from the beginning of
     * the file. On failure it returns -1
     */
    int lseekFile(int fd, int pos, int whence);

    /**
     * \internal
     * Get file status, given file descriptor.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param fd file descriptor, obtained with openFile
     * \param buf a pointer to a stat struct
     * \return On success 0 is returned, on failure -1.
     */
    int fstatFile(int fd, struct stat *pstat);

    /**
     * \internal
     * Get file status, given file name.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param file file name + path
     * \param buf a pointer to a stat struct
     * \return On success 0 is returned, on failure -1.
     */
    int statFile(const char *file, struct stat *pstat);

    /**
     * \internal
     * Close the file.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param fd file descriptor, obtained with openFile
     * \return On success 0 is returned, on failure -1.
     */
    int closeFile(int fd);

    /**
     * \internal
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \return true if the file is open
     */
    bool isFileOpen(int fd) const;

    /**
     * \internal
     * Create a directory
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param path path where to create the directory.
     * \param mode, file open permissions, ignored on FAT32, reserved for future
     * use if some unix-like filesystem is added.
     * \return 0 on success, -1 on failure, -2 if the directory already exists
     */
    int mkdir(const char *path, int mode);

    /**
     * \internal
     * Remove a file.
     * User code should not use this member function directly, it should just
     * use C/C++ standard file access functions.
     * \param path file name + path
     * \return On success 0 is returned, on failure -1.
     */
    int unlink(const char *path);

private:
    /**
     * Constructor.
     */
    Filesystem();

    /**
     * Find the slot where the file data is stored given the file descriptor.
     * To be called when fsMutex is already locked.
     * \return x where files[x].fd==fd or -1 if fd is not associated with any
     * slot.
     */
    int findSlot(int fd) const;

    /**
     * Find an empty slot to be used to create a file, and fills the fd field
     * with a new file descriptor obtained from newFileDescriptor()
     * To be called when fsMutex is already locked.
     * \return x where files[x].isEmpty()==true or -1 if there are no empty
     * slots
     */
    int findAndFillEmptySlot();

    /**
     * Remove the slot identified by fd, also deallocates the memory for the FIL
     * object.
     * \param fd slot to free
     */
    void freeSlot(int fd);

    /**
     * \return a new file descriptor, suitable to be given to a new file
     */
    int newFileDescriptor();

    /**
     * \internal
     * An element of a data structure used to keep track of all open files
     */
    class FileEntry
    {
    public:
        /**
         * Constructor. Initialize fptr and fd to a known state
         */
        FileEntry(): fptr(0), fd(0) {}
        
        /**
         * \internal Pointer to a dynamically allocatd file object. If it is
         * zero this slot is empty (not an open file).
         */
        FIL *fptr;

        int fd;///<\internal File descriptor associated with this slot

        /**
         * \internal
         * \return true if this slot is empty
         */
        bool isEmpty() const
        {
            return fptr==0;
        }
    };

    bool mounted;///<\internal True if filesystem is mounted
    int fileDescriptorCounter;///<\internal used to generate file descriptors
    FATFS filesystem;///<\internal Filesystem struct used by FatFs
    FileEntry files[MAX_OPEN_FILES];///<\internal table of file entries
};

/**
 * \enum Attrib
 * File attribures, as returned by Directory::next(). More flags can be set at
 * the same time
 */
enum Attrib
{
    READ_ONLY = 0x01,///< Entry is read-only
    HIDDEN    = 0x02,///< Entry is hidden
    SYSTEM    = 0x04,///< Entry is a system file
    VOLUME_ID = 0x08,///< Volume label (?)
    DIRECTORY = 0x10,///< If this flag is set, entry is a directory, otherwise it's a file
    ARCHIVE   = 0x20 ///< Entry is marked as archive
};

/**
 * This class represent a directory. It has methods for listing files inside
 * them.
 * All methods can be called only if the filesystem is mounted.
 *
 * IMPORTANT:<br>
 * Name of files and directories is limited to old DOS 8.3 format. The name of
 * the file must not exceed 8 chracter, and the extension must not exceed 3
 * chars. Also the name must not contain spaces.
 *
 * The directory listing function will return the name of file truncated if
 * necessary (example "My_long_file_name.txt" will be reported as
 * "MY_LON~1.TXT")
 *
 * To open a file created on a computer, that has a long file name, you must use
 * the truncated name too.
 */
class Directory
{
public:
    /**
     * This is the maximum length of entry name reported by Directory::next().
     * Use it to create buffers to pass to Directory::next(). This dimension
     * includes the null termination character.
     */
    static const unsigned int FILENAME_LEN=13;

    /**
     * Constructor.
     */
    Directory();

    /**
     * Open a directory, to list its content
     * \param name name and path of directory in unix notation (example:
     * "/mydir" and not "C:\mydir")<br>
     * Note that name of files and directories is limited to old DOS 8.3 format.
     * \return 0 on success. Error codes are:
     * - 1 = Error
     * - 3 = Filesystem not mounted
     */
    char open(const char *name);

    /**
     * Get next directory entry.
     * \param name a string where the file name will be copied. The string size
     * must be >=FILENAME_LEN
     * \param size size of the entry
     * \param attrib entry attributes (allow undrstanding if entry is a file or
     * directory)
     *  \return true while there are entries in the directory
     * Note that if the return value is false, name size and attrib are not valid
     */
    bool next(char *name, unsigned int& size, unsigned char& attrib);

    /**
     * \param name name and path of directory in unix notation (example:
     * "/mydir" and not "C:\mydir")<br>
     * Note that name of files and directories is limited to old DOS 8.3 format.
     * \return true if directory exists.
     */
    static bool exists(const char *name);

    /*
     * Uses default destructor.
     */
private:
    //Unwanted methods
    Directory(const Directory& d);///< No copy constructor
    Directory& operator= (const Directory& d);///< No operator =
    //Data
    DIR d;///< FatFs directory variable
};

/**
 * \}
 */

}; //namespace miosix

#endif //WITH_FILESYSTEM

#endif //FILESYSYTEM_H
