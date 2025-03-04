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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WWSaveLoad                                                   *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwsaveload/pointerremap.h                    $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 4/30/01 1:54p                                               $*
 *                                                                                             *
 *                    $Revision:: 6                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#if defined(_MSC_VER)
#pragma once
#endif


#ifndef POINTERREMAP_H
#define POINTERREMAP_H

#ifdef _UNIX
#include "osdep/osdep.h"
#endif

#include "always.h"
#include "bittype.h"
#include "vector.h"

#include <unordered_map>

class RefCountClass;

class PointerRemapClass 
{
	public:
		
		PointerRemapClass(void);
		~PointerRemapClass(void);

		void		Reset(void);
		void		Process(void);

		void		Register_Pointer (uint32 old_pointer_to_convert, void *new_pointer);

#ifdef WWDEBUG
		void		Request_Pointer_Remap (uint32 old_pointer_to_convert, void **pointer_to_convert,const char * file,int line);
		void		Request_Ref_Counted_Pointer_Remap (uint32 old_pointer_to_convert, RefCountClass **pointer_to_convert,const char * file, int line);
#else
		void		Request_Pointer_Remap (uint32 old_pointer_to_convert, void **pointer_to_convert);
		void		Request_Ref_Counted_Pointer_Remap (uint32 old_pointer_to_convert, RefCountClass **pointer_to_convert);
#endif

		// Convert a pointer to an ID. Will return an existing ID if this pointer has been seen before
		uint32 Convert_Pointer(void* pointer);
	private:

		struct PtrPairStruct
		{
			PtrPairStruct(void) {}
			PtrPairStruct(uint32 oldptr,void * newptr) : OldPointer(oldptr),NewPointer(newptr) {}
			bool operator == (const PtrPairStruct & that) const { return ((OldPointer == that.OldPointer) && (NewPointer == that.NewPointer)); } 
			bool operator != (const PtrPairStruct & that) const { return !(*this == that); } 
			
			uint32		OldPointer;
			void *		NewPointer;
		};
		
		struct PtrRemapStruct
		{
			PtrRemapStruct(void) {}
			PtrRemapStruct(uint32 oldptr, void** pointerToRemap) : OldPointer(oldptr), PointerToRemap(pointerToRemap) {}
			bool operator == (const PtrRemapStruct & that) const { return OldPointer == that.OldPointer && PointerToRemap == that.PointerToRemap; }
			bool operator != (const PtrRemapStruct & that) const { return !(*this == that); } 
			
			uint32 OldPointer;
			void** PointerToRemap;
#ifdef WWDEBUG
			const char *	File;
			int				Line;
#endif
		};

		void		Process_Request_Table(DynamicVectorClass<PtrRemapStruct> & request_table,bool refcount);

		/*
		**	Array of pointers associated with ID values to assist in swizzling.
		*/
		std::unordered_map<uint32, void*> IDToPointerMapping;
		std::unordered_map<void*, uint32> PointerToIDMapping;
		DynamicVectorClass<PtrRemapStruct> PointerRequestTable;
		DynamicVectorClass<PtrRemapStruct> RefCountRequestTable;
		uint32 NextPointerID = 0;
};


#endif
