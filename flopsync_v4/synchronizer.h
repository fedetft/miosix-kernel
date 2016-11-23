/***************************************************************************
 *   Copyright (C)  2013 by Terraneo Federico                              *
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

#ifndef SYNCHRONIZER_H
#define	SYNCHRONIZER_H
#include <utility>

/**
 * Base class from which synchronization schemes derive
 */
class Synchronizer
{
public:
    /**
     * Compute clock correction and receiver window given synchronization error
     * \param e synchronization error
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<int,int> computeCorrection(int e)=0;
    
    /**
     * Compute clock correction and receiver window when a packet is lost
     * \return a pair with the clock correction, and the receiver window
     */
    virtual std::pair<int,int> lostPacket()=0;
    
    /**
     * Used after a resynchronization to reset the controller state
     */
    virtual void reset()=0;
    
    /**
     * \return the synchronization error e(k)
     */
    virtual int getSyncError() const=0;
    
    /**
     * \return the clock correction u(k)
     */
    virtual int getClockCorrection() const=0;
    
    /**
     * \return the receiver window (w)
     */
    virtual int getReceiverWindow() const=0;

    /**
     * Destructor
     */
    virtual ~Synchronizer();
};

#endif //SYNCHRONIZER_H
