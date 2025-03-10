/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /G/wwlib/bittype.h 4     4/02/99 1:37p Eric_c $ */
/*************************************************************************** 
 ***                  Confidential - Westwood Studios                    *** 
 *************************************************************************** 
 *                                                                         * 
 *                 Project Name : Voxel Technology                         * 
 *                                                                         * 
 *                    File Name : BITTYPE.H                                * 
 *                                                                         * 
 *                   Programmer : Greg Hjelstrom                           * 
 *                                                                         * 
 *                   Start Date : 02/24/97                                 * 
 *                                                                         * 
 *                  Last Update : February 24, 1997 [GH]                   * 
 *                                                                         * 
 *-------------------------------------------------------------------------* 
 * Functions:                                                              * 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef BITTYPE_H
#define BITTYPE_H

#include <cstdint>

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using uint = uint32;

using sint8 = int8_t;
using sint16 = int16_t;
using sint32 = int32_t;
using sint64 = int64_t;
using sint = sint32;

using int8 = sint8;
using int16 = sint16;
using int32 = sint32;
using int64 = sint64;

using float32 = float;
using float64 = double;

using DWORD = unsigned long;
using WORD = unsigned short;
using BYTE = uint8;
using BOOL = int;
using USHORT = uint16;
using LPCSTR = const char*;
using UINT = unsigned int;
using ULONG = unsigned long;

using uintptr = uintptr_t;

#endif //BITTYPE_H
