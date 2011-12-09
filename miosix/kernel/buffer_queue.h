/***************************************************************************
 *   Copyright (C) 2011 by Terraneo Federico                               *
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

#include "kernel.h"
#include "error.h"

#ifndef BUFFER_QUEUE_H
#define	BUFFER_QUEUE_H

namespace miosix {

/**
 * \addtogroup Sync
 * \{
 */

/**
 * A class to handle double buffering, but also triple buffering and in general
 * N-buffering. Works between two threads but is especially suited to
 * synchronize between a thread and an interrupt routine.<br>
 * Note that unlike Queue, this class is only a data structure and not a
 * synchronization primitive. The synchronization between the thread and
 * the IRQ (or the other thread) must be done by the caller. Therefore, to
 * avoid race conditions all the member function are on purpose marked IRQ
 * meaning that can only be called with interrupt disabled or within an IRQ.<br>
 * The internal implementation treats the buffers as a circular queue of N
 * elements, hence the name.
 * \tparam T type of elements of the buffer, usually char or unsigned char
 * \tparam size maximum size of a buffer
 * \tparam numbuf number of buffers, the default is two resulting in a
 * double buffering scheme. Values 0 and 1 are forbidden
 */
template<typename T, unsigned int size, unsigned char numbuf=2>
class BufferQueue
{
public:
    /**
     * Constructor, all buffers are empty
     */
    BufferQueue() : put(0), get(0), cnt(0) {}

    ///Maximum size of a buffer
    static const unsigned int bufferMaxSize=size;

    ///Number of buffers available
    static const unsigned char numberOfBuffers=numbuf;

    /**
     * \return true if no buffer is available for reading
     */
    bool IRQisEmpty() const { return cnt==0; }

    /**
     * \return true if no buffer is available for writing
     */
    bool IRQisFull() const { return cnt==numbuf; }

    /**
     * This member function allows to retrieve a buffer ready to be written,
     * if available.
     * \param buffer the available buffer will be assigned here if available
     * \return true if a writable buffer has been found, false otherwise.
     * In this case the buffer parameter is not modified
     */
    bool IRQgetWritableBuffer(T *&buffer)
    {
        if(cnt==numbuf) return false;
        buffer=buf[put];
        return true;
    }

    /**
     * After having called getWritableBuffer() or IRQgetWritableBuffer() to
     * retrieve a buffer and having filled it, this member function allows
     * to mark the buffer as available on the reader side.
     * \param actualSize actual size of buffer. It usually equals bufferMaxSize
     * but can be a lower value in case there is less available data
     */
    void IRQbufferFilled(unsigned int actualSize)
    {
        if(++cnt>numbuf) errorHandler(UNEXPECTED);
        bufSize[put]=actualSize;
        put++;
        put%=numbuf;
    }

    /**
     * \return The number of buffers available for writing (0 to numbuf)
     */
    unsigned char IRQavailableForWriting() const { return numbuf-cnt; }

    /**
     * This member function allows to retrieve a buffer ready to be read,
     * if available.
     * \param buffer the available buffer will be assigned here if available
     * \param actualSize the actual size of the buffer, as reported by the
     * writer side
     * \return true if a readable buffer has been found, false otherwise.
     * In this case the buffer and actualSize parameters are not modified
     */
    bool IRQgetReadableBuffer(const T *&buffer, unsigned int& actualSize)
    {
        if(cnt==0) return false;
        buffer=buf[get];
        actualSize=bufSize[get];
        return true;
    }

    /**
     * After having called getReadableBuffer() or IRQgetReadableBuffer() to
     * retrieve a buffer and having read it, this member function allows
     * to mark the buffer as available on the writer side.
     */
    void IRQbufferEmptied()
    {
        if(--cnt<0) errorHandler(UNEXPECTED);
        get++;
        get%=numbuf;
    }

    /**
     * \return The number of buffers available for reading (0, to numbuf)
     */
    unsigned char IRQavailableForReading() const { return cnt; }

    /**
     * Reset the buffers. As a consequence, the queue becomes empty.
     */
    void IRQreset()
    {
        put=get=cnt=0;
    }

private:
    //Unwanted methods
    BufferQueue(const BufferQueue&);
    BufferQueue& operator=(const BufferQueue&);

    T buf[numbuf][size]; // The buffers
    unsigned int bufSize[numbuf]; //To handle partially empty buffers
    unsigned char put; //Put pointer, either 0 or 1
    unsigned char get; //Get pointer, either 0 or 1
    volatile unsigned char cnt; //Number of filled buffers, either (0 to numbuf)
};

//These two partial specialization are meant to produce compiler errors in case
//an attempt is made to allocate a BufferQueue with zero or one buffer, as it
//is forbidden
template<typename T, unsigned int size> class BufferQueue<T,size,0> {};
template<typename T, unsigned int size> class BufferQueue<T,size,1> {};

/**
 * \}
 */

} //namespace miosix

#endif //BUFFER_QUEUE_H
