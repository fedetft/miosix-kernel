/***************************************************************************
 *   Copyright (C) 2026 by Radu Raul                                       *
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
#include "../UUID/UUID.h"
namespace GPT 
{
#define MAKE_UUID_NAME_PAIR(uuid, name) {DEF_UUID(uuid), name}
constexpr std::initializer_list<std::pair<UUID::UUID, 
                const char*>> GPT_PARTITION_IDS = {
                    MAKE_UUID_NAME_PAIR("00000000-0000-0000-0000-000000000000", "Empty"),
                    MAKE_UUID_NAME_PAIR("024DEE41-33E7-11D3-9D69-0008C781F39F", "MBR Partition Scheme"),
                    MAKE_UUID_NAME_PAIR("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7", "Microsoft Basic Data Partition"),
                };
} // namespace GPT