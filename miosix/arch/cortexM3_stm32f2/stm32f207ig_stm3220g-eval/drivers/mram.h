/***************************************************************************
 *   Copyright (C) 2012 by Terraneo Federico                               *
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

#include <pthread.h>

#ifndef MRAM_H
#define	MRAM_H

/**
 * Class to access the mram memory
 */
class Mram
{
public:
    /**
     * Singleton
     * \return an instance of the Mram class 
     */
    static Mram& instance();
    
    /**
     * \return the MRAM's size in bytes 
     */
    unsigned int size() const;
    
    /**
     * Write a block of data into the mram
     * \param addr start address into the mram where the data block will be
     * written
     * \param data data block
     * \param size data block size
     * \return true on success, false on failure
     */
    bool write(unsigned int addr, const void *data, int size);
    
    /**
     * Read a block of data from the mram
     * \param addr start address into the mram where the data block will be read
     * \param data data block
     * \param size data block size
     * \return true on success, false on failure
     */
    bool read(unsigned int addr, void *data, int size);
    
    /**
     * This member function needs to be called before accessing the mram
     */
    void exitSleepMode();
    
    /**
     * This member function can be called after performing operations on the
     * mram to put it back in sleep mode
     */
    void enterSleepMode();
    
    /**
     * \return true if the mram is in sleep mode 
     */
    bool isSleeping() const { return sleepMode; }
    
private:
    Mram();
    
    Mram(const Mram&);
    Mram& operator= (const Mram&);
    
    pthread_mutex_t mutex; ///Mutex to protect concurrent access to the hardware
    bool sleepMode;
};

#endif //MRAM_H
