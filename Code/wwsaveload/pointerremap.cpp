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
 *                     $Archive:: /Commando/Code/wwsaveload/pointerremap.cpp                  $*
 *                                                                                             *
 *                       Author:: Greg Hjelstrom                                               *
 *                                                                                             *
 *                     $Modtime:: 5/09/01 11:36a                                              $*
 *                                                                                             *
 *                    $Revision:: 9                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "pointerremap.h"
#include "refcount.h"
#include "wwdebug.h"


const int POINTER_TABLES_GROWTH_STEP = 4096;


PointerRemapClass::PointerRemapClass(void)
{
	PointerRequestTable.Set_Growth_Step(POINTER_TABLES_GROWTH_STEP);
	RefCountRequestTable.Set_Growth_Step(POINTER_TABLES_GROWTH_STEP);
	Reset();
}

PointerRemapClass::~PointerRemapClass(void)
{
}

void PointerRemapClass::Reset(void)
{
	PointerRequestTable.Delete_All();
	RefCountRequestTable.Delete_All();

	NextPointerID = 0u;
	IDToPointerMapping.clear();
	PointerToIDMapping.clear();
	PointerToIDMapping.emplace(nullptr, 0u);
}

void PointerRemapClass::Process(void)
{
	if ( PointerRequestTable.Count() > 0 )
	{
		WWASSERT(!IDToPointerMapping.empty());
		Process_Request_Table(PointerRequestTable,false);
	}

	// remap the ref-counted pointers
	if ( RefCountRequestTable.Count() > 0 )
	{
		WWASSERT(!IDToPointerMapping.empty());
		Process_Request_Table(RefCountRequestTable,true);
	}
}

void PointerRemapClass::Process_Request_Table(DynamicVectorClass<PtrRemapStruct> & request_table, bool refcount)
{
	// Remap the pointers
	for (int request_index = 0; request_index < request_table.Count(); ++request_index)
	{
		PtrRemapStruct& current_request = request_table[request_index];
		const uint32 old_ptr = request_table[request_index].OldPointer;
		auto search_itr = IDToPointerMapping.find(old_ptr);

		// we found the match, plug in the new pointer and add a ref if needed.
		if (search_itr != IDToPointerMapping.end())
		{
			*current_request.PointerToRemap = search_itr->second;

			if (refcount)
			{
				RefCountClass* refptr = static_cast<RefCountClass*>(*current_request.PointerToRemap);
				refptr->Add_Ref();
			}
		}
		// Failed to re-map the pointer.
		// If this happens, things could be going very wrong.  (find out why its happening!)
		else
		{
			*current_request.PointerToRemap = nullptr;
#ifdef WWDEBUG
			const char* file = current_request.File;
			int line = current_request.Line;
			WWDEBUG_SAY(("Warning! Failed to re-map pointer! old_ptr = 0x%X  file = %s  line = %d\r\n", old_ptr, file, line));
			WWASSERT(0);
#endif
		}
	}
}

uint32 PointerRemapClass::Convert_Pointer(void* pointer)
{
	auto search_itr = PointerToIDMapping.find(pointer);
	if (search_itr != PointerToIDMapping.end())
	{
		return search_itr->second;
	}
	else
	{
		const uint32 pointer_id = ++NextPointerID;
		PointerToIDMapping.emplace(pointer, pointer_id);
		WWASSERT(pointer_id > 0); // Wrapping is unsupported, but we'll probably run out of memory before that happens.
		return pointer_id;
	}
}

void PointerRemapClass::Register_Pointer(uint32 old_pointer, void *new_pointer)
{
	IDToPointerMapping.emplace(old_pointer, new_pointer);
}

#ifdef WWDEBUG
void PointerRemapClass::Request_Pointer_Remap(uint32 old_pointer_to_convert, void **pointer_to_convert,const char * file,int line)
{
	PtrRemapStruct remap(old_pointer_to_convert, pointer_to_convert);
	remap.File = file;
	remap.Line = line;
	PointerRequestTable.Add(remap);
}

void PointerRemapClass::Request_Ref_Counted_Pointer_Remap (uint32 old_pointer_to_convert, RefCountClass **pointer_to_convert,const char * file, int line)
{
	PtrRemapStruct remap(old_pointer_to_convert, (void**)pointer_to_convert);
	remap.File = file;
	remap.Line = line;
	RefCountRequestTable.Add(remap);
}

#else

void PointerRemapClass::Request_Pointer_Remap (uint32 old_pointer_to_convert, void **pointer_to_convert)
{
	PointerRequestTable.Add(PtrRemapStruct(old_pointer_to_convert, pointer_to_convert));
}

void PointerRemapClass::Request_Ref_Counted_Pointer_Remap (uint32 old_pointer_to_convert, RefCountClass **pointer_to_convert)
{
	RefCountRequestTable.Add(PtrRemapStruct(old_pointer_to_convert, (void**)pointer_to_convert));
}

#endif

