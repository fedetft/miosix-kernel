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

#pragma once

#include "stringpart.h"

namespace miosix {

/**
 * Class for iterating a normalized path, splitting into its individual elements
 * This class is meant to handle with already normalized paths, not containing
 * multiple consecutive / characters, such as "//bin//ls", nor "/./" elements.
 * The path can however start with either "/" or not, and the initial "/" is
 * not part of the returned elements.
 *
 * NOTE: This class uses the capability of StringPart to allow making in-place
 * substrings of the original string without allocating additional memory.
 * As a result, while the path is being split (i.e, before the last call to
 * next(), the one which returns nullptr) the path string that is passed to the
 * constructor should not be deallocated, reassigned or accessed, as it is
 * being modified to make the substrings.
 * Destructing the NormalizedPathWalker will also restore the path string to its
 * original content.
 *
 * Example code:
 * \code
 * string s="/dir1/dir2/file1";
 * StringPart path(s);
 * NormalizedPathWalker pw(path);
 * while(auto element=pw.next())
 * {
 *     printf("%s\n",element->c_str());
 * }
 * \endcode
 * This example prints the strings "dir1", "dir2", "file1.txt"
 */
class NormalizedPathWalker
{
public:
    /**
     * Constructor
     * \param path normalized path to split into its components. It is stored
     * by reference and shuld not be deallocated, reassigned or used until the
     * last call to next(), the one that returns nullptr or until this class is
     * destructed, whichever comes first
     */
    NormalizedPathWalker(StringPart& path);

    /**
     * Get the next path element.
     * \return a pointer to a StringPart containing the next path element, or
     * nullptr if the previous called returned the last path element.
     * Calling next() again or destructing the NormalizedPathWalker invalidates
     * the returned pointer.
     */
    StringPart *next();

private:
    StringPart& path;   ///< Reference to the path passed through the constructor
    size_t index;       ///< Index of the first unhandled char in the path
    StringPart element; ///< Temporary string storing individual path elements
};

} //namespace miosix
