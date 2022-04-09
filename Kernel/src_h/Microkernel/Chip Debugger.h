/*
	Chip Debugger.h
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
	Terry Greeniaus	-	Tuesday, 3 November 1998	-	Original creation of file
*/
#ifndef __CHIP_DEBUGGER__
#define __CHIP_DEBUGGER__

#define	FIELD_OFFSET(type,field)	((UInt32)&((type*)0)->field)
#define	CHIP_REGISTER(type,field,flags)	{#field,sizeof(((type*)0)->field),flags,FIELD_OFFSET(type,field)}
#define	LAST_REGISTER				{nil,0,0,0}

enum
{
	REG_NOFLAGS		=	0,	// No flags
	REG_LE			=	1,	// Indicates that this is a little-endian register, so use byte-reversing instructions
	REG_SIDE_EFFECTS	=	2,	// Indicates that this register has side-effects when read from
	REG_READ_ONLY	=	4,	// Indicates that this register is read-only
	REG_WRITE_ONLY	=	8	// Indicates that this register is write-only
};

typedef struct RegisterDescriptor
{
	ConstASCII8Str		name;	// The name of this register
	UInt32			size;		// Must be 1, 2 or 4
	UInt32			flags;	// Flag bits for this register
	UInt32			offset;	// The offset of this register from the base of the chip
}RegisterDescriptor;

struct Chip
{
	class Chip*		next;
	class Chip*		prev;
	ConstASCII8Str		name;
	RegisterDescriptor*	regs;
	void*			addr;
	
	Chip(ConstASCII8Str name,RegisterDescriptor* regs,void* addr);
	~Chip();
};

void	DebugChips();

#endif /* __CHIP_DEBUGGER__ */