/*
	MacOS VMUtils.h
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
#ifndef __MACOS_VM_UTILS__
#define __MACOS_VM_UTILS__

/*
	MacOS VMUtils is a collection of virtual memory utilties that I have gathered
	from various places around the Internet.  It has been packaged here so that
	you can call these routines from either 68K or PPC code transparently.  Simply
	include this header and add the "MacOS VMUtils.cp" file to your project.  An
	explanation of the routines follows.  Note than unless otherwise specified, "page"
	refers to a number 0x000zzzzz where zzzzz is the actual page number.  Given any
	address 0xaaaaannn, the aaaaa part is the page number and corresponds to the
	zzzzz in the calls.
	
	MapPage			-	this takes logicalPage and makes it map to physicalPage.  Usually
						you will want to UnmapPage the logical one before you do this.
	UnmapPage		-	this un-maps a logical page so that that page no longer maps any
						physical memory.  You should always set isInited to 1 for this, don't
						really know what it does.
	GetPTBits			-	this looks up logicalPage in the page table and returns the physical
						page that that page corresponds to.  Note that this works even if
						the page is not locked!  The returned value is of the form
						0xaaaaabbb where aaaaa is the page number and bbb appears to be
						garbage.  GetPhysicalPage returns -1 if that page is not mapped.
	ChangePTBits		-	this changes the page table entry bits associated with the page.  A
						page table entry looks like this:
						
						0xAAAAABBB
						
						0xAAAAA	-	This is the physical page associated with the logical page
						0xBBB -		These are the page table bits.
						
						Returns 0 if succesful, -1 otherwise (I think)
*/

enum
{
	// Page table bits
	ptReadOnlyBit		=	2,
	ptReferencedBit	=	3,
	ptChangedBit		=	4,
	
	// Page table masks
	ptReadOnlyMask	=	(1UL << ptReadOnlyBit),
	ptReferencedMask	=	(1UL << ptReferencedBit),
	ptChangedMask		=	(1UL << ptChangedBit)
};

#if TARGET_CPU_68K

void MapPage(unsigned long logicalPage:__A0,unsigned long physicalPage:__A1)
	TWOWORDINLINE(0x7007,0xFE0A);

void UnmapPage(unsigned long logicalPage:__A0,unsigned long isInited:__A1)
	TWOWORDINLINE(0x7008,0xFE0A);

unsigned long GetPTBits(unsigned long logicalPage:__A0)
	TWOWORDINLINE(0x7013,0xFE0A):__D0;

unsigned long ChangePTBits(unsigned long physicalPTEntry:__A0,unsigned long logicalPage:__A1)
	TWOWORDINLINE(0x7014,0xFE0A);
	
OSErr MarkReadOnly(unsigned long address:__A0,unsigned long count:__A1)
	TWOWORDINLINE(0x7006,0xA05C):__D0;

OSErr MarkReadWrite(unsigned long address:__A0,unsigned long count:__A1)
	TWOWORDINLINE(0x7007,0xA05C):__D0;

#elif TARGET_CPU_PPC

void MapPage(unsigned long logicalPage,unsigned long physicalPage);
void UnmapPage(unsigned long logicalPage,unsigned long isInited);
unsigned long GetPTBits(unsigned long logicalPage);
unsigned long ChangePTBits(unsigned long physicalPTEntry,unsigned long logicalPage);
OSErr MarkReadOnly(unsigned long address,unsigned long count);
OSErr MarkReadWrite(unsigned long address,unsigned long count);

#else

#error Don't have MacOS VMUtils for this CPU!

#endif

#endif /* __MACOS_VM_UTILS__ */