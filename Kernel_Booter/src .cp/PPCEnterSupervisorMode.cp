/*
	PPCEnterSupervisorMode.cp
	Copyright © 1998 by Patrick Varilly

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
	Patrick Varilly		-					-	Original creation of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Saturday, 3 Oct 98	-	Revised code to use MacOS VMUtils
	Terry Greeniaus	-	Saturday, 10 Oct 98	-	Changed sysCallPatchCode[] so that it preserved all registers (including cr)
*/
#include "MacOS VMUtils.h"
#include "PPCEnterSupervisorMode.h"
#include "Assembly.h"

#define	DISABLE_INTERRUPTS	1
#if DISABLE_INTERRUPTS
#define	MSR_MASK	0x29FF	// Turn off EE,PR,ME,SE,BE bits
#else
#define	MSR_MASK	0xBFFF	// Turn off PR bit only, useful for debugging purposes.
#endif

static void sc(void);

unsigned long	sysCallPatchCode[]=	{	0x7C200026,	//	mfcr		sp;
								0x2C035061,	//	cmpwi	r3,'Pa';
								0x40820034,	//	bne		@originalTrap;
								0x2C046E64,	//	cmpwi	r4,'nd';
								0x4082002C,	//	bne		@originalTrap;
								0x7C2803A6,	//	mtlr		sp;
								0x7C3B02A6,	//	mfsrr1	sp;
								0x70210000 | MSR_MASK,	//	andi.		sp,sp,MSR_MASK;
								0x7C3B03A6,	//	mtsrr1	sp;
								0x7C2802A6,	//	mflr		sp;
								0x7C2FF120,	//	mtcr		sp;
								0x7C3242A6,	//	mfsprg	sp, 2;
								0x7C2803A6,	//	mtlr		sp;
								0x7C3142A6,	//	mfsprg	sp, 1;
								0x4C000064,	//	rfi;
								0x7C2FF120,	//	@originalTrap:	mtcr		sp;
								0x3C200000,	//				lis		sp, ((origAddr >> 16) & 0xFFFF);
								0x60210000,	//				ori		sp, sp, (origAddr & 0xFFFF);
								0x7C2803A6,	//				mtlr		sp;
								0x4E800020	//				blr;
							};
							
char	superCodeInited = false;

void InitSuperCode(void)
{
	// Lock the syscall patch in place
	LockMemoryContiguous((void*)sysCallPatchCode,sizeof(sysCallPatchCode));
	
	// Set up the @bad vector correctly
	unsigned long	origSysVector = *(unsigned long*)0x68FFE450;
	sysCallPatchCode[16] |= ((origSysVector >> 16) & 0x0000FFFF);
	sysCallPatchCode[17] |= (origSysVector & 0x0000FFFF);
	
	// Make the code executable
	MakeDataExecutable((void*)sysCallPatchCode,sizeof(sysCallPatchCode));
	
	// Get the physical address of the new patch
	unsigned long	physicalPatch = ( (GetPTBits((unsigned long)sysCallPatchCode >> 12) & 0xFFFFF000 ) | ( (unsigned long)sysCallPatchCode & 0x00000FFF ));
	
	// Install the patch
	Ptr			ptr = NewPtr(8192);
	unsigned long	page = ((((unsigned long)ptr + 4096) & 0xFFFFF000) >> 12);
	unsigned long	physicalPage = (GetPTBits(page) >> 12);
	unsigned long*	vector = (unsigned long*)((page << 12) + 0x00000450);
	UnmapPage(page,true);
	MapPage(page,(GetPTBits(0x68FFE) >> 12));
	*vector = physicalPatch;
	
	// Restore the correct page mapping
	UnmapPage(page,true);
	MapPage(page,physicalPage);
	
	// Release our pointer
	DisposePtr(ptr);
	
	// Super code is now inited
	superCodeInited = true;
}

void DeinitSuperCode(void)
{
	if(!superCodeInited)
	{
		debugstr("Super code is not initialized!!!");
		return;
	}
	
	// Super code is no longer inited
	superCodeInited = false;
	
	// Get the original supermode vector
	unsigned long	origSysVector = ((sysCallPatchCode[15] << 16) & 0xFFFF0000) | (sysCallPatchCode[16] & 0x0000FFFF);
	
	// Remove the patch from the vector table
	Ptr			ptr = NewPtr(8192);
	unsigned long	page = ((((unsigned long)ptr + 4096) & 0xFFFFF000) >> 12);
	unsigned long	physicalPage = (GetPTBits(page) >> 12);
	unsigned long*	vector = (unsigned long*)((page << 12) + 0x00000450);
	UnmapPage(page,true);
	MapPage(page,(GetPTBits(0x68FFE) >> 12));
	*vector = origSysVector;
	
	// Restore the correct page mapping
	UnmapPage(page,true);
	MapPage(page,physicalPage);
	
	// Release our pointer
	DisposePtr(ptr);
	
	// Unlock the memory
	UnlockMemory((void*)sysCallPatchCode,sizeof(sysCallPatchCode));
}

void PPCEnterSupervisorMode(void)
{
	if(superCodeInited)
		sc();
	else
		debugstr("Super code is not initialized!!!" );
}

static asm void sc(void)
{
	// Load our special super mode key
	li		r3,'Pa';
	li		r4,'nd';
	sc;	// And we're off!!!
	
	blr;
}
