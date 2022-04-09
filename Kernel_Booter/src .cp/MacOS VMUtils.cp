/*
	MacOS VMUtils.cp
	Copyright © 1998 by Terry Greeniaus

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*
	Version History
	============
	Terry Greeniaus	-	Monday, 28 Sept 98	-	Original creation of file
*/
#include "MacOS VMUtils.h"

#if TARGET_CPU_PPC

unsigned short	MapPage_68K[]			=	{0x7007,0xFE0A,0x4E75};
unsigned short	UnmapPage_68K[]		=	{0x7008,0xFE0A,0x4E75};
unsigned short	GetPhysicalPage_68K[]	=	{0x7013,0xFE0A,0x4E75};
unsigned short	ChangePTBits_68K[]		=	{0x7014,0xFE0A,0x4E75};
unsigned short	MarkReadOnly_68K[]	=	{0x7006,0xA05C,0x4E75};
unsigned short	MarkReadWrite_68K[]	=	{0x7007,0xA05C,0x4E75};

enum
{
	MapPage_68KProcInfo = kRegisterBased
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_ROUTINE_PARAMETER(2,kRegisterA1,SIZE_CODE(sizeof(unsigned long))),
	UnmapPage_68KProcInfo = kRegisterBased
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_ROUTINE_PARAMETER(2,kRegisterA1,SIZE_CODE(sizeof(unsigned long))),
	GetPhysicalPage_68KProcInfo = kRegisterBased
		| RESULT_SIZE(SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_RESULT_LOCATION(kRegisterD0)
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long))),
	ChangePTBits_68KProcInfo = kRegisterBased
		| RESULT_SIZE(SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_RESULT_LOCATION(kRegisterD0)
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_ROUTINE_PARAMETER(2,kRegisterA1,SIZE_CODE(sizeof(unsigned long))),
	MarkReadOnly_68KProcInfo = kRegisterBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSErr)))
		| REGISTER_RESULT_LOCATION(kRegisterD0)
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_ROUTINE_PARAMETER(2,kRegisterA1,SIZE_CODE(sizeof(unsigned long))),
	MarkReadWrite_68KProcInfo = kRegisterBased
		| RESULT_SIZE(SIZE_CODE(sizeof(OSErr)))
		| REGISTER_RESULT_LOCATION(kRegisterD0)
		| REGISTER_ROUTINE_PARAMETER(1,kRegisterA0,SIZE_CODE(sizeof(unsigned long)))
		| REGISTER_ROUTINE_PARAMETER(2,kRegisterA1,SIZE_CODE(sizeof(unsigned long)))
};

UniversalProcPtr	MapPage_68Kupp = NewRoutineDescriptor((ProcPtr)MapPage_68K,MapPage_68KProcInfo,kM68kISA | kOld68kRTA);
UniversalProcPtr	UnmapPage_68Kupp = NewRoutineDescriptor((ProcPtr)UnmapPage_68K,UnmapPage_68KProcInfo,kM68kISA | kOld68kRTA);
UniversalProcPtr	GetPhysicalPage_68Kupp = NewRoutineDescriptor((ProcPtr)GetPhysicalPage_68K,GetPhysicalPage_68KProcInfo,kM68kISA | kOld68kRTA);
UniversalProcPtr	ChangePTBits_68Kupp = NewRoutineDescriptor((ProcPtr)ChangePTBits_68K,ChangePTBits_68KProcInfo,kM68kISA | kOld68kRTA);
UniversalProcPtr	MarkReadOnly_68Kupp = NewRoutineDescriptor((ProcPtr)MarkReadOnly_68K,MarkReadOnly_68KProcInfo,kM68kISA | kOld68kRTA);
UniversalProcPtr	MarkReadWrite_68Kupp = NewRoutineDescriptor((ProcPtr)MarkReadWrite_68K,MarkReadWrite_68KProcInfo,kM68kISA | kOld68kRTA);

void MapPage(unsigned long logicalPage,unsigned long physicalPage)
{
	CallUniversalProc(MapPage_68Kupp,MapPage_68KProcInfo,logicalPage,physicalPage);
}

void UnmapPage(unsigned long logicalPage,unsigned long isInited)
{
	CallUniversalProc(UnmapPage_68Kupp,UnmapPage_68KProcInfo,logicalPage,isInited);
}

unsigned long GetPTBits(unsigned long page)
{
	return CallUniversalProc(GetPhysicalPage_68Kupp,GetPhysicalPage_68KProcInfo,page);
}

unsigned long ChangePTBits(unsigned long physicalPTEntry,unsigned long logicalPage)
{
	return CallUniversalProc(ChangePTBits_68Kupp,ChangePTBits_68KProcInfo,physicalPTEntry,logicalPage);
}

OSErr MarkReadOnly(unsigned long address,unsigned long count)
{
	return CallUniversalProc(MarkReadOnly_68Kupp,MarkReadOnly_68KProcInfo,address,count);
}

OSErr MarkReadWrite(unsigned long address,unsigned long count)
{
	return CallUniversalProc(MarkReadWrite_68Kupp,MarkReadWrite_68KProcInfo,address,count);
}

#endif