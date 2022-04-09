/*
	Memory Utils.h
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
	Other sources			Project				Author			Notes
	===========			======				=====			====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 19 June 98	-	Added MemSet function
	Terry Greeniaus	-	Sunday, 9 August 98	-	Moved over from old Memory Manager.h file
*/
#ifndef __MEMORY_UTILS__
#define __MEMORY_UTILS__

void		MemCopy(const void* src, void* dst, UInt32 size);
void		MemZero(void* mem, UInt32 size);
void		MemSet(void* mem,UInt32 size, UInt8 val);
void		ReverseMem(register void* p,register UInt32 size);						// Reverses memory (in place)
void		SwapMem(register void* data1,register void* data2,register UInt32 len);		// Swaps memory
void		SwapMem_x8(register void* data1,register void* data2,register UInt32 len);	// Swaps memory, len must be a multiple of 8
void		SwapMem_x16(register void* data1,register void* data2,register UInt32 len);	// Swaps memory, len must be a multiple of 16
void		MoveMem_x8(register const void* src,register void* dest,register UInt32 len);	// Moves memory, len must be a multiple of 8, src/dest shouldn't overlap
void		MoveMem_x16(register const void* src,register void* dest,register UInt32 len);	// Moves memory, len must be a multiple of 16, src/dest shouldn't overlap

#endif /* __MEMORY_UTILS__ */