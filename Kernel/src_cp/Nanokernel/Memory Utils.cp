/*
	Memory Utils.cp
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
	Terry Greeniaus	-	Sunday, 9 August 98	-	Original creation of file
*/
#include "Assembly.h"
#include  "Memory Utils.h"

void MemCopy(const void* src, void* dst, UInt32 size )
{
	Int8*	source = (Int8*)src;
	Int8*	dest = (Int8*)dst;
	
	// This can be *thoroughly* optimized!!!! Right now, we don't care about efficiency, though.
	
	while( size-- )
		*dest++ = *source++;
}

void MemZero( void* mem, UInt32 size )
{
	MemSet(mem,size,0);
}

void MemSet(void* mem,UInt32 size,UInt8 val)
{
	UInt8*	memory = ((UInt8*)mem);
	
	// This can be *thoroughly* optimized!!!! Right now, we don't care about efficiency, though.
	
	while( size-- > 0 )
		*memory++ = val;
}

__asm__ void ReverseMem(register void* p,register UInt32 size)
{
	srwi_(r0,r4,1);		// Divide by 2 and test if size == 0
	mtctr	r0;			// Loop counter
	add		r4,r3,r4		// Get the end of the buffer (pre-incremented too)
	subi		r3,r3,1;		// Pre-decrement start of buffer
	beqlr;				// Go home if size == 0

	@loop:
		lbz		r0,1(r3);
		lbz		r5,-1(r4);
		stbu		r0,-1(r4);
		stbu		r5,1(r3);
		bdnz		@loop;
	
	blr;
}

__asm__ void SwapMem(register void* data1,register void* data2,register UInt32 len)
{
	cmpwi	r5,0;
	subi		r3,r3,1;
	subi		r4,r4,1;
	mtctr	r5;
	beqlr;
	
	@loop:
		lbz		r0,1(r3);
		lbz		r5,1(r4);
		stbu		r0,1(r4);
		stbu		r5,1(r3);
		bdnz		@loop;
	
	blr;
}

__asm__ void SwapMem_x8(register void* data1,register void* data2,register UInt32 len)
{
	srwi_(r5,r5,3);		// Divide by 8
	subi		r3,r3,4;		// Pre-decrement pointers
	subi		r4,r4,4;
	mtctr	r5;
	beqlr;
	@loop:				// Unrolled loop to optimize memory accesses
		lwz		r0,4(r3);
		lwz		r6,4(r4);
		lwz		r5,8(r3);
		lwz		r7,8(r4);
		stw		r0,4(r4);
		stw		r6,4(r3);
		stwu		r5,8(r4);
		stwu		r7,8(r3);
		bdnz		@loop;
	blr;
}

__asm__ void SwapMem_x16(register void* data1,register void* data2,register UInt32 len)
{
	srwi_(r5,r5,4);		// Divide by 16
	subi		r3,r3,4;
	subi		r4,r4,4;
	mtctr	r5;
	beqlr;
	@loop:				// Unrolled loop to optimize memory accesses
		lwz		r0,4(r3);
		lwz		r6,4(r4);
		lwz		r5,8(r3);
		lwz		r8,8(r4);
		lwz		r7,12(r3);
		lwz		r10,12(r4);
		lwz		r9,16(r3);
		lwz		r11,16(r4);
		stw		r0,4(r4);
		stw		r6,4(r3);
		stw		r5,8(r4);
		stw		r8,8(r3);
		stw		r7,12(r4);
		stw		r10,12(r3);
		stwu		r9,16(r4);
		stwu		r11,16(r3);
		bdnz		@loop;
	blr;
}

__asm__ void MoveMem_x8(register const void* src,register void* dest,register UInt32 len)
{
	srwi_(r5,r5,3);		// Divide by 8
	subi		r3,r3,4;	// Pre-decrement pointers
	subi		r4,r4,4;
	mtctr	r5;
	beqlr;
	@loop:
		lwz		r0,4(r3);
		lwzu		r5,8(r3);
		stw		r0,4(r4);
		stwu		r5,8(r4);
		bdnz		@loop;
	blr;
}

__asm__ void MoveMem_x16(register const void* src,register void* dest,register UInt32 len)
{
	srwi_(r5,r5,4);	// Divide by 16
	subi		r3,r3,8;	// Pre-decrement pointers
	subi		r4,r4,8;
	mtctr	r5;
	beqlr;
	@loop:
/*		lwz		r0,4(r3);
		lwz		r5,8(r3);
		lwz		r6,12(r3);
		lwzu		r7,16(r3);
		stw		r0,4(r4);
		stw		r5,8(r4);
		stw		r6,12(r4);
		stwu		r7,16(r4);*/
		lfd		fp0,8(r3);
		lfdu		fp1,16(r3);
		stfd		fp0,8(r4);
		stfdu		fp1,16(r4);
		bdnz		@loop;
	blr;
}