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

#ifndef FLOODING_SCHEME_H
#define	FLOODING_SCHEME_H

/**
 * Base class from which flooding schemes derive
 */
class FloodingScheme
{
public:
    /**
     * Constructor
     * \param syncPeriod synchronization period
     */
    FloodingScheme(long long syncPeriod) : syncPeriod(syncPeriod) {}
    
    /**
     * Needs to be periodically called to send/receive the synchronization
     * packet. This member function sleeps till it's time to send the packet,
     * then sends it and returns. If this function isn't called again within
     * the synchronization period, the synchronization won't work.
     * \return true if the node desynchronized
     */
    virtual bool synchronize()=0;
    
    /**
     * Called if synchronization is lost
     */
    virtual void resynchronize();
    
    /**
     * \return The (local) time when the synchronization packet was
     * actually received
     */
    virtual long long getMeasuredFrameStart() const=0;
    
    /**
     * \return The (local) time when the synchronization packet was
     * expected to be received
     */
    virtual long long getComputedFrameStart() const=0;
    
    /**
     * \return synchronization period
     */
    long long getSyncPeriod() const { return syncPeriod; }

    /**
     * Destructor
     */
    virtual ~FloodingScheme();
    
protected:
    const long long syncPeriod;
};

#endif //FLOODING_SCHEME_H
