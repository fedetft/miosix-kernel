/***************************************************************************
 *   Copyright (C) 2026 by Terraneo Federico                               *
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

#include <vector>
#include <tuple>
#include <string>

/*
 * The issue in reading map files: static functions. Handling them is hard
 * as for code compiled without -ffunction-section they don't appear as
 * symbols in the map file. Only for code compiled with -ffunction-section
 * they do appear.
 *
 * We have three options, all of which suck in a different way:
 * 1) Both for code with/without -ffunction-section static function aren't
 *    considered and their size bloat up whatever symbol comes before (or after?)
 * 2) We process also symbolSize lines, but this does nothing to fix the
 *    bloat up in code compiled without -ffunciton-section
 * 3) We switch to reporting sizes at the translation unit granularity, but
 *    this way we lose valuable function-level information
 *
 * We provide code for option 1 (loadMapFileByFunctionNames)
 * and option 3 (loadMapFileByTranslationUnits), you choose which to use.
 */

/// The Mapfile type is just a vector of tuples with name, size
using Mapfile=std::vector<std::tuple<std::string,unsigned int>>;

/**
 * Load a map file attempting to extract the .text data at the granularity of
 * function names.
 * \param filename map file
 * \return a Mapfile type
 */
Mapfile loadMapFileByFunctionNames(const std::string& filename);

/**
 * Load a map file extracting the .text data at the granularity of translation
 * units.
 * \param filename map file
 * \return a Mapfile type
 */
Mapfile loadMapFileByTranslationUnits(const std::string& filename);
