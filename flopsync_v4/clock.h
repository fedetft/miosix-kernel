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

#ifndef CLOCK_H
#define	CLOCK_H

/**
 * Base class from which clocks derive
 */
class Clock
{
public:
    /**
     * This member function converts between root frame time, that is, a time
     * that starts goes from 0 to nominalPeriod-1 and is referenced to the root
     * node, to absolute time referenced to the local node time. It can be used
     * to send a packet to the root node at the the frame time it expects it.
     * \param root a frame time (0 to nominalPeriod-1) referenced to the root node
     * \return the local absolute time time corresponding to the given root time
     */
    virtual long long localTime(long long slotGlobalTime)=0;
    
    virtual long long globalTime(long long slotLocalTime)=0;

    /**
     * Destructor
     */
    virtual ~Clock();
};

#endif //CLOCK_H