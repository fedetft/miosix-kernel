/***************************************************************************
 *   Copyright (C) 2011-2025 by Terraneo Federico                          *
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

//This file contains private implementation details of pthread code, it's not
//meant to be used by end users

#pragma once

namespace miosix {

#ifdef WITH_PTHREAD_EXIT

/**
 * Exception type thrown when pthread_exit is called.
 *
 * NOTE: This type should not derive from std::exception on purpose, as it would
 * be bad if a \code catch(std::exception& e) \endcode would match this
 * exception.
 */
class PthreadExitException
{
public:
    /**
     * Constructor
     * \param returnValue thread return value passed to pthread_exit
     */
    PthreadExitException(void *returnValue) : returnValue(returnValue) {}

    /**
     * \return the thread return value passed to pthread_exit
     */
    void *getReturnValue() const { return returnValue; }

private:
    void *returnValue;
};

#endif //WITH_PTHREAD_EXIT

#ifdef WITH_PTHREAD_KEYS
/**
 * Called at thread exit to call destructors for pthread keys
 * \param pthreadKeyValues thread-local pthread_key values
 */
void callPthreadKeyDestructors(void *pthreadKeyValues[MAX_PTHREAD_KEYS]);
#endif //WITH_PTHREAD_KEYS

} //namespace miosix
