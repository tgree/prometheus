/*
	NKVirtualMemory.cp
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
*/
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "NKAtomicOperations.h"
#include "NKProcesses.h"
#include "Kernel Types.h"
#include "Assembly.h"
#include "Macros.h"

#if BIG_ENDIAN

typedef union SegmentRegister
{
	UInt32	number;
	struct SegmentRegisterBits
	{
		UInt32	t	:	1;
		UInt32	ks	:	1;
		UInt32	kp	:	1;
		UInt32	n	:	1;
		UInt32	rsrv	:	4;
		UInt32	vsid	:	24;
	}bits;
}SegmentRegister;

#else
#error Better make a little-endian SegmentRegister structure.
#endif

enum
{
	htabMask64K	=	0x000,
	htabMask128K	=	0x001,
	htabMask256K	=	0x003,
	htabMask512K	=	0x007,
	htabMask1MB	=	0x00F,
	htabMask2MB	=	0x01F,
	htabMask4MB	=	0x03F,
	htabMask8MB	=	0x07F,
	htabMask16MB	=	0x0FF,
	htabMask32MB	=	0x1FF
};

enum
{
	dbat128K		=	0x00000000,
	dbat256K		=	0x00000001,
	dbat512K		=	0x00000003,
	dbat1MB		=	0x00000007,
	dbat2MB		=	0x0000000F,
	dbat4MB		=	0x0000001F,
	dbat8MB		=	0x0000003F,
	dbat16MB		=	0x0000007F,
	dbat32MB		=	0x000000FF,
	dbat64MB		=	0x000001FF,
	dbat128MB	=	0x000003FF,
	dbat256MB	=	0x000007FF
};

static void	NKInitPageTable(UInt32 htabMask);
static _PTE*	NKGetPTEG1(register void* logicalAddr,register UInt32 processID);
static _PTE*	NKGetPTEG2(register void* logicalAddr,register UInt32 processID);

static void*	NKVirtualFromPTE(_PTE* pte);
static void*	NKPhysicalFromPTE(_PTE* pte);
static void	NKAddPTE(_PTE* pte,UInt32 vsid,UInt32 h,void* logicalAddr,void* physicalAddr,UInt32 wimg,UInt32 pp);
static void	NKModifyPTE(_PTE* pte,UInt32 newVsid,UInt32 newH,void* newLogicalAddr,void* newPhysicalAddr,UInt32 newWimg,UInt32 newPP);
static _PTE*	NKFindPTE(void* logicalAddr,UInt32 processID);
static void	NKRemapPage(void* logicalAddr,void* newPhysicalAddr,UInt32 processID);

PTE*			PTBase = nil;
UInt32			PTLen = nil;
static UInt32		PTMask = nil;
static UInt32		ioMap = 0x10000000;
static NKSpinLock	PTLock;

void NKInitVirtualMemory(void)
{
	PTLock.init();
	
	// Set up the page table. -- Remember that the page table must be mapped via a BAT register - if you want to put it in high memory, make
	// sure you map that memory with a BAT register first.   Right now we put it directly after the kernel.
	NKInitPageTable(htabMask512K);
	
	// We also need segment registers and SDR1 set up earlier on the boot processor - the memory mapping routines (NKMapPage and friends)
	// read the SRs and SDR1 directly to find the page table, rather than relying on global variables here.
	NKInitSRs();
	NKInitSDR1();
	
	// Memory layout looks like this:
	//
	//	Code
	//	Data
	//	Guard page
	//	Stack
	//	Free memory
	//	Page table
	//
	
	// Map kernel code
	NKMapRange(machine.codeStart,machine.codeStart,(UInt32)machine.dataStart - (UInt32)machine.codeStart,WIMG_COHERENT,PP_READ_ONLY,PROCESS_KERNEL);
	
	// Map kernel data
	UInt32 stackGuardPage = ROUND_UP(4096,(UInt32)machine.stackBase);
	NKMapRange(machine.dataStart,machine.dataStart,(UInt32)stackGuardPage - (UInt32)machine.dataStart,WIMG_COHERENT,PP_READ_WRITE,PROCESS_KERNEL);
	
	// Map a guard page at the bottom of the stack
	NKMapRange((void*)stackGuardPage,(void*)stackGuardPage,4096,WIMG_COHERENT,PP_NO_ACCESS,PROCESS_KERNEL);
	
	// Map the stack
	NKMapRange((void*)(stackGuardPage + 4096),(void*)(stackGuardPage + 4096),(UInt32)machine.kernelEnd - stackGuardPage - 4096,WIMG_COHERENT,PP_READ_WRITE,PROCESS_KERNEL);
	
	// Map page table 1-1, user/supervisor access.
	NKMapRange((void*)PTBase,(void*)PTBase,PTLen,WIMG_COHERENT,PP_READ_WRITE,PROCESS_KERNEL);
	machine.freeMemStart = (void*)((UInt32)PTBase + PTLen);
	
	// Remap video for the nanokernel video driver - since we are turning off BATs, the video needs to still be mapped for the nanokernel video driver
	// to work correctly.
	NKRemapVideo();
}

__asm__ void NKDisableBATMap(void)
{
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,1;
	li		r3,0;
	beq		@disable601;
@disableNon601:
	sync;
	mtibatu(0,r3);
	mtibatu(1,r3);
	mtibatu(2,r3);
	mtibatu(3,r3);
	mtdbatu(0,r3);
	mtdbatu(1,r3);
	mtdbatu(2,r3);
	mtdbatu(3,r3);
@done:
	sync;
	blr;

@disable601:
	sync;
	mtibatl(0,r3);
	mtibatl(1,r3);
	mtibatl(2,r3);
	mtibatl(3,r3);
	sync;
	blr;
}

void NKSaveBATMap( UInt32 batNum, UInt32& up, UInt32& down )
{
	// Save IBat register
	switch(batNum)
	{
		case 0:
			up = _getIBAT0U();
			down = _getIBAT0L();
		break;
		case 1:
			up = _getIBAT1U();
			down = _getIBAT1L();
		break;
		case 2:
			up = _getIBAT2U();
			down = _getIBAT2L();
		break;
		case 3:
			up = _getIBAT3U();
			down = _getIBAT3L();
		break;
	}
}

void NKSetBATMap( UInt32 batNum, UInt32 up, UInt32 down )
{
	// Save IBat register
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		switch(batNum)
		{
			case 0:
				_setIBAT0L(0);
				_setIBAT0U(up);
				_setIBAT0L(down);
			break;
			case 1:
				_setIBAT1L(0);
				_setIBAT1U(up);
				_setIBAT1L(down);
			break;
			case 2:
				_setIBAT2L(0);
				_setIBAT2U(up);
				_setIBAT2L(down);
			break;
			case 3:
				_setIBAT3L(0);
				_setIBAT3U(up);
				_setIBAT3L(down);
			break;
		}
	}
	else
	{
		switch(batNum)
		{
			case 0:
				_setDBAT0U(0);
				_setDBAT0L(down);
				_setDBAT0U(up);
				_setIBAT0U(0);
				_setIBAT0L(down);
				_setIBAT0U(up);
			break;
			case 1:
				_setDBAT1U(0);
				_setDBAT1L(down);
				_setDBAT1U(up);
				_setIBAT1U(0);
				_setIBAT1L(down);
				_setIBAT1U(up);
			break;
			case 2:
				_setDBAT2U(0);
				_setDBAT2L(down);
				_setDBAT2U(up);
				_setIBAT2U(0);
				_setIBAT2L(down);
				_setIBAT2U(up);
			break;
			case 3:
				_setDBAT3U(0);
				_setDBAT3L(down);
				_setDBAT3U(up);
				_setIBAT3U(0);
				_setIBAT3L(down);
				_setIBAT3U(up);
			break;
		}
	}
}

void NKZapBATMap( UInt32 batNum )
{
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		// Zap 601 BAT
		switch(batNum)
		{
			case 0:
				_setIBAT0L(0);
			break;
			case 1:
				_setIBAT1L(0);
			break;
			case 2:
				_setIBAT2L(0);
			break;
			case 3:
				_setIBAT3L(0);
			break;
		}
	}
	else
	{
		// Zap non-601 BAT
		switch(batNum)
		{
			case 0:
				_setDBAT0U(0);
				_setIBAT0U(0);
			break;
			case 1:
				_setDBAT1U(0);
				_setIBAT1U(0);
			break;
			case 2:
				_setDBAT2U(0);
				_setIBAT2U(0);
			break;
			case 3:
				_setDBAT3U(0);
				_setIBAT3U(0);
			break;
		}
	}
}

void NKInitSRs(void)
{
	SegmentRegister	sr;
	
	sr.bits.t = 0;
	sr.bits.ks = sr.bits.kp = 1;
	sr.bits.n = 0;
	sr.bits.rsrv = 0;
	for(Int32 i = 0;i<16;i++)
	{
		sr.bits.vsid = 0x00100000*i | PROCESS_KERNEL;
		_setSR(i,sr.number);
	}
}

void NKInitPageTable(UInt32 htabMask)
{
	const PTE	invalidPTE = {0,0,0,0,0,0,0,0,0,0,0};
	PTMask = htabMask;
	PTLen = (htabMask + 1) << 16;
	PTBase = (PTE*)ROUND_UP(PTLen,(UInt32)machine.kernelEnd);
	PTE* pte = PTBase;
	
	for(UInt32 i=0;i<(PTLen/sizeof(PTE));i++,pte++)
		*pte = invalidPTE;
}

void NKInitSDR1(void)
{
	_setSDR1( ((UInt32)PTBase & 0xFFFF0000) | PTMask );
}

__asm__ void NKInvalidateTLB(void)
{
#if __MWERKS__
	machine 603;	// This works on every machine, however we need to trick the Metrowerks compiler into letting
				// us use these insrtuctions
#endif
	mfpvr(r5);
	srwi(r5,r5,16);
	cmpwi	r5,1;	// 601
	beq		@inval601TLB;
	cmpwi	r5,3;	// 603
	li		r3,32;
	beq		@invalNon601TLB;
	cmpwi	r5,6;	// 603e
	beq		@invalNon601TLB;
	cmpwi	r5,7;	// 603ev
	beq		@invalNon601TLB;
	cmpwi	r5,8;	// 750
	li		r3,64;
	beq		@invalNon601TLB;
	li		r3,128;	// 604 and 604e and 7400 chips will fall through
	
@invalNon601TLB:
	mtctr	r3;
	@invalLoopNon601:
		sync;
		tlbie		r4;
		sync;
		tlbsync;
		sync;
		addi		r4,r4,0x1000;
		bdnz		@invalLoopNon601;
	blr;
	
	
@inval601TLB:
	// 601 is different - it doesn't have a tlbsync instruction.
	li		r3,128;
	mtctr	r3;
	@invalLoop601:
		sync;
		tlbie		r4;
		sync;
		addi		r4,r4,0x1000;
		bdnz		@invalLoop601;
	blr;
}

static __asm__ _PTE* NKGetPTEG1(register void* logicalAddr,register UInt32 processID)
{
	// We assume that this routine does not touch register r8,r9,r10, r11, r12
	
	// Get the SR for this address
	cmpwi	r4,PROCESS_CURRENT;
	bne		@hash;
	mfsrin	r4,r3;

@hash:
	// Hash function
	rlwinm	r4,r4,0,13,31;	// Low order 19 bits of VSID from the processID (NOT SR NOW THAT WE HAVE REAL PROCESSES!)
	rlwinm	r3,r3,0,4,19;		// We just want the Page Index of logicalAddr
	srwi(r3,r3,12);
	xor		r3,r3,r4;			// r3 contains HASH1
	
	mfsdr1(r4);
	rlwinm	r5,r4,0,23,31;	// r5 contains HTABMASK
	slwi(r5,r5,16);			// Move the HTABMASK field into place
	slwi(r3,r3,6);				// Move the HASH field into place
	and		r5,r3,r5;			// r5 contains the shifted HTABMASK & HASH field
	rlwinm	r3,r3,0,16,25;	// r3 contains the bottom part of HASH and 6 zeroes
	or		r3,r3,r5;			// r3 contains everything except the HTABORG part
	rlwinm	r4,r4,0,0,15;		// r4 contains HTABORG
	or		r3,r3,r4;			// r3 contains the PTE group address;
	
	blr;
}

static __asm__ _PTE* NKGetPTEG2(register void* logicalAddr,register UInt32 processID)
{
	// We assume that this routine does not touch register r8,r9,r10, r11, r12
	
	// Get the SR for this address
	cmpwi	r4,PROCESS_CURRENT;
	bne		@hash;
	mfsrin	r4,r3;
	
@hash:
	// Hash function
	rlwinm	r4,r4,0,13,31;	// Low order 19 bits of VSID from the processID (NOT SR NOW THAT WE HAVE REAL PROCESSES!)
	rlwinm	r3,r3,0,4,19;		// We just want the Page Index of logicalAddr
	srwi(r3,r3,12);
	xor		r3,r3,r4;			// r3 contains HASH1
	not		r3,r3;			// r3 contains HASH2
	
	mfsdr1(r4);
	rlwinm	r5,r4,0,23,31;	// r5 contains HTABMASK
	slwi(r5,r5,16);			// Move the HTABMASK field into place
	slwi(r3,r3,6);				// Move the HASH field into place
	and		r5,r3,r5;			// r5 contains the shifted HTABMASK & HASH field
	rlwinm	r3,r3,0,16,25;	// r3 contains the bottom part of HASH and 6 zeroes
	or		r3,r3,r5;			// r3 contains everything except the HTABORG part
	rlwinm	r4,r4,0,0,15;		// r4 contains HTABORG
	or		r3,r3,r4;			// r3 contains the PTE group address;
	
	blr;
}

__asm__ PTE* NKGetPTE(register void* logicalAddr,register UInt32 processID)	// Returns the PTE which maps logicalAddr, returns nil if no such entry
{
	// We assume that GetPTEG1 and GetPTEG2 don't touch r8,r9,r10, r11 or r12
	cmpwi	r4,PROCESS_CURRENT;
	rrwinm(r9,r3,22,26,31);	// r9 contains the api for this search.
	mr		r10,r3;			// r10 contains the logical address we are searching for
	mflr		r12;				// r12 contains the return address when we are done
	beq		@processCurrent
	rrwinm(r11,r3,8,8,11);
	or		r11,r11,r4;
	b		@findAProcessPTE;
@processCurrent:
	// processID == PROCESS_CURRENT, so we are searching for the current mapping of this logical page
	mfsrin	r11,r10;
	rlwinm	r11,r11,0,8,31;
	
@findAProcessPTE:
	// This must execute with data relocation disabled.  It will be faster AND it will mean we can search the MacOS page table before we
	// replace it with our own.
	mfmsr	r8;
	rlwinm	r4,r8,0,28,26;
	sync;
	mtmsr	r4;
	sync;
	
	// Search the primary PTEG
	mr		r4,r11;
	bl		NKGetPTEG1;
	li		r4,0;
	bl		@searchPTEG;
	// Search the secondary PTEG
	mr		r3,r10;
	mr		r4,r11;
	bl		NKGetPTEG2;
	li		r4,0x0040;
	bl		@searchPTEG;
	
	// We didn't find it in the page table.  Return a nil physical address (no PTE can ever be found at nil)
	li		r3,0;
	b		@returnToCaller;

@searchPTEG:	
	// We assume that searchPTEG doesn't touch r9,r10,r11,r12
	// r3 contains the address of the first entry in the PTEG.  r11 contains the ProcessID for this search.
	// r12 contains the return address if we find an entry.  r4 contains the value in the H position of the
	// first word of the PTE.  r10 contains the logical address we are searching for.  r9 contains the
	// api for the search.
	subi		r3,r3,8;
	li		r0,8;
	mtctr	r0;
	
	@searchLoop:
		lwzu		r5,8(r3);
		rlwinm.	r0,r5,0,0,0;		// Check the valid bit
		beq-		@endLoop;
		//rlwinm	r0,r5,0,25,25;	// Check that the H bit is correct
		andi.		r0,r5,0x0040;
		cmpw	r0,r4;
		bne-		@endLoop;
		rrwinm(r0,r5,7,8,31);		// Check that the VSID is correct
		cmpw	r0,r11;
		bne-		@endLoop;
		rlwinm	r0,r5,0,26,31;	// Check that the api is correct
		cmpw	r0,r9;
		bne-		@endLoop;
		// Success, we found the PTE
		b		@returnToCaller;
	@endLoop:
		bdnz		@searchLoop;
		blr;		// Return from @searchPTEG (not back to the caller)

@returnToCaller
	sync;
	mtmsr	r8;
	sync;
	mtlr		r12;
	blr;
}

void* NKVirtualFromPTE(_PTE* pte)
{
	// Returns the virtual address that a particular PTE maps
	Assert(pte->v == 1);
	
	UInt32	pageIndexUpper = (pte->api << 10);
	UInt32	pageIndexLower = (((UInt32)pte >> 6) ^ pte->vsid);
	pageIndexLower = (pte->h ? ~pageIndexLower : pageIndexLower) & 0x000003FF;
	UInt32	pageIndex = (pageIndexUpper | pageIndexLower);
	UInt32	segment = ((pte->vsid >> 20) & 0x0000000F);
	UInt32	virtualAddr = ((segment << 28) | (pageIndex << 12));
	
	return (void*)virtualAddr;
}

void* NKPhysicalFromPTE(_PTE* pte)
{
	// Returns the logical address that a particular PTE maps
	return (void*)((UInt32)pte->rpn << 12);
}

void NKAddPTE(_PTE* _pte,UInt32 vsid,UInt32 h,void* logicalAddr,void* physicalAddr,UInt32 wimg,UInt32 pp)
{
	// The PTLock must have already been accquired at this point, so that some other processor didn't
	// modify this PTE between the call to here and getting here
	
	// Make sure this PTE isn't already in use
	Assert(_pte->v == false);
	
	PTE		tempPTE;
	PTE*	pte = (PTE*)_pte;
	tempPTE = *pte;
	
	// Change the second word
	tempPTE.pte.rpn = (((UInt32)physicalAddr & 0xFFFFF000) >> 12);
	tempPTE.pte.r = tempPTE.pte.c = 0;
	tempPTE.pte.wimg = wimg;
	tempPTE.pte.pp = pp;
	pte->pteData[1] = tempPTE.pteData[1];
	
	_eieio();
	
	// Change the first word
	tempPTE.pte.vsid = vsid;
	tempPTE.pte.h = h;
	tempPTE.pte.api = (((UInt32)logicalAddr & 0x0FC00000) >> 22);
	pte->pteData[0] = tempPTE.pteData[0];
	
	_sync();
	
	// Validate it!
	_pte->v = 1;
}

void NKModifyPTE(_PTE* _pte,UInt32 newVsid,UInt32 newH,void* newLogicalAddr,void* newPhysicalAddr,UInt32 newWimg,UInt32 newPP)
{
	// See notes in NKAddPTE
	
	// Make sure it already exists
	Assert(_pte->v == true);
	
	// Invalidate the current mapping
	_pte->v = 0;
	_sync();
	
	PTE		tempPTE;
	PTE*	pte = (PTE*)_pte;
	void*	oldEA = NKVirtualFromPTE(_pte);
	
	tempPTE = *pte;
	
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		// This is how we do it on a 601
		_tlbie((UInt32)oldEA);
		_sync();
		tempPTE.pte.vsid = newVsid;
		tempPTE.pte.h = newH;
		tempPTE.pte.api = (((UInt32)newLogicalAddr & 0x0FC00000) >> 22);
		pte->pteData[0] = tempPTE.pteData[0];
		
		tempPTE.pte.rpn = (((UInt32)newPhysicalAddr & 0xFFFFF000) >> 12);
		tempPTE.pte.wimg = newWimg;
		tempPTE.pte.pp = newPP;
		pte->pteData[1] = tempPTE.pteData[1];
		
		_sync();
		
		pte->pte.v = 1;
	}
	else
	{
		// This is how we do it on a non-601
		tempPTE.pte.rpn = (((UInt32)newPhysicalAddr & 0xFFFFF000) >> 12);
		tempPTE.pte.wimg = newWimg;
		tempPTE.pte.pp = newPP;
		pte->pteData[1] = tempPTE.pteData[1];
		
		_tlbie((UInt32)oldEA);
		_eieio();
		
		tempPTE.pte.vsid = newVsid;
		tempPTE.pte.h = newH;
		tempPTE.pte.api = (((UInt32)newLogicalAddr & 0x0FC00000) >> 22);
		tempPTE.pte.v = 1;
		pte->pteData[0] = tempPTE.pteData[0];
		
		_tlbsync();
		_sync();
	}
}

_PTE* NKFindPTE(void* logicalAddr,UInt32 processID)
{
	_PTE*	pteg = NKGetPTEG1(logicalAddr,processID);
	for(Int32 i=0;i<8;i++)
	{
		if(pteg[i].v && !pteg[i].h)
			if(NKVirtualFromPTE(&pteg[i]) == logicalAddr)
				return &pteg[i];
	}
	
	pteg = NKGetPTEG2(logicalAddr,processID);
	for(Int32 i=0;i<8;i++)
	{
		if(pteg[i].v && pteg[i].h)
			if(NKVirtualFromPTE(&pteg[i]) == logicalAddr)
				return &pteg[i];
	}
	
	return nil;
}

void NKRemapPage(void* logicalAddr,void* newPhysicalAddr,UInt32 processID)
{
	_PTE*	_pte = NKFindPTE(logicalAddr,processID);
	Assert(_pte != nil);
	
	if(_pte)
	{
		PTE		tempPTE;
		PTE*	pte = (PTE*)_pte;
		void*	oldEA = NKVirtualFromPTE(_pte);
		tempPTE = *pte;
		
		Assert(_pte->v != false);
		
		_pte->v = 0;
		_sync();
		
		tempPTE.pte.rpn = (((UInt32)newPhysicalAddr & 0xFFFFF000) >> 12);
		tempPTE.pte.r = tempPTE.pte.c = 0;
		pte->pteData[1] = tempPTE.pteData[1];
		
		_tlbie((UInt32)oldEA);
		if((_getPVR() & 0xFFFF0000) != 0x00010000)
		{
			_sync();
			_tlbsync();
		}
		_sync();
	}
}

void NKMapPage(void* logicalAddr,void* physicalAddr,UInt32 wimg,UInt32 pp,ProcessID processID)
{
	NKSpinLocker			ptLocker(PTLock);
	
	if((_getPVR() & 0xFFFF0000) == 1)
		wimg = wimg & 0x0000000E;	// No guarded bit on 601 - all pages are guarded I guess.
	
	if(processID == PROCESS_CURRENT)
		processID = _getSR(((UInt32)logicalAddr >> 28) & 0x0000000F) & 0x000FFFFF;
	
	UInt32 vsid = (((UInt32)logicalAddr >> 8) & 0x00F00000) | processID;
	
	_PTE* pte = NKGetPTEG1(logicalAddr,processID);
	for(Int32 i=0;i<8;i++,pte++)
	{
		if(!pte->v)
		{
			NKAddPTE(pte,vsid,0,logicalAddr,physicalAddr,wimg,pp);
			Assert(((UInt32)NKVirtualFromPTE(pte)) == ((UInt32)logicalAddr & 0xFFFFF000));
			return;
		}
	}
	
	pte = NKGetPTEG2(logicalAddr,processID);
	for(Int32 i=0;i<8;i++,pte++)
	{
		if(!pte->v)
		{
			NKAddPTE(pte,vsid,1,logicalAddr,physicalAddr,wimg,pp);
			Assert(((UInt32)NKVirtualFromPTE(pte)) == ((UInt32)logicalAddr & 0xFFFFF000));
			return;
		}
	}
	
	Panic("Failed to map a logical page because the PTEGs were full!\n");
}

void* NKUnmapPage(void* logicalAddr,ProcessID processID)
{
	NKSpinLocker	ptLocker(PTLock);
	PTE*		pte = NKGetPTE(logicalAddr,processID);
	Assert(pte != nil);
	void*		retVal = NKPhysicalFromPTE(&pte->pte);
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		pte->pte.v = 0;
		_sync();
		_tlbie((UInt32)logicalAddr);
		_sync();
	}
	else
	{
		pte->pte.v = 0;
		_sync();
		_tlbie((UInt32)logicalAddr);
		_eieio();
		_tlbsync();
		_sync();
	}
	
	return retVal;
}

void NKMapRange(void* logicalAddr,void* physicalAddr,UInt32 lenBytes,UInt32 wimg,UInt32 pp,ProcessID processID)
{
	// Assume this logical page has not been mapped yet (this will be fixed later)
	lenBytes += ((UInt32)logicalAddr & 0x00000FFF);
	logicalAddr = (void*)((UInt32)logicalAddr & 0xFFFFF000);
	physicalAddr = (void*)((UInt32)physicalAddr & 0xFFFFF000);
	UInt32 numPages = (ROUND_UP(4096,lenBytes)>>12);
	for(UInt32 i=0;i<numPages;i++)
		NKMapPage((void*)((UInt32)logicalAddr + (i<<12)),(void*)((UInt32)physicalAddr + (i<<12)),wimg,pp,processID);
}

void* NKIOMap(void* physicalAddr,UInt32 len,UInt32 wimg,UInt32 pp)
{
	len += ((UInt32)physicalAddr & 0x00000FFF);
	UInt32 realLen = ROUND_UP(4096,len);
	ioMap -= realLen;
	NKMapRange((void*)ioMap,physicalAddr,realLen,wimg,pp,PROCESS_KERNEL);
	void* logicalAddr = (void*)(ioMap | ((UInt32)physicalAddr & 0x00000FFF));

#if DISP_IO_MAP
	nkVideo << "New IO memory mapping:\n    Logical  " << ((UInt32)logicalAddr & 0xFFFFF000) << " - " << (((UInt32)logicalAddr & 0xFFFFF000) + realLen) << "\n";
	nkVideo << "    Physical " << ((UInt32)physicalAddr & 0xFFFFF000) << " - " << (((UInt32)physicalAddr & 0xFFFFF000) + realLen) << "\n";
#endif

	return logicalAddr;
}

__asm__ void* NKGetPhysical(register void* logicalAddr,register UInt32 processID)
{
	// Get the physical address of a logical address in a given process.  We simply get the page table entry
	// and then look up the real address from there.  We return 0 if there was no page table entry mapping
	// that process/address combination.  Address 0 is never mapped.
	mflr		r0;
	stw		r0,8(sp);
	stw		r3,-4(sp);
	stwu		sp,-64(sp);
	
	// If we are searching in the first segment (Kernel), then use PROCESS_KERNEL, because that segment
	// is always the kernel segment, regardless of which process' memory space we are really searching
	rlwinm.	r0,r3,0,0,3;
	bne		@notKernelSegment;
	li		r4,PROCESS_KERNEL;
@notKernelSegment:
	
	mfpvr(r5);
	srwi(r5,r5,16);
	cmpwi	r5,1;
	bne		@searchPageTable;
	
	// This is a 601 - see if this is a Memory Forced IO segment - don't know if this actually works or not, since
	// we never use Memory Forced IO.
	mfsrin	r5,r3;
	rlwinm.	r0,r5,0,0,0;		// See if T bit is set
	beq		@searchPageTable;
	rrwinm(r6,r5,20,23,31);	// Get Bus ID field
	cmpwi	r6,0x007F;
	bne		@outOfHere;		// I/O segment, but not Memory Forced, so this is some weird device mapping which we don't handle yet.
	rlwinm	r5,r5,28,0,3;		// Get the physical segment address
	rlwinm	r3,r3,0,4,31;		// Get the logical address offset
	or		r3,r3,r5;
	b		@outOfHere;
	
@searchPageTable:
	bl		NKGetPTE;
	
	cmpwi	r3,0;
	beq		@outOfHere;
	
	mfmsr	r4;
	rlwinm	r5,r4,0,28,26;
	sync;
	mtmsr	r5;
	sync;
	
	lwz		r3,4(r3);
	
	sync;
	mtmsr	r4;
	sync;
	
	addi		sp,sp,64;
	lwz		r4,-4(sp);
	rlwinm	r3,r3,0,0,19;
	rlwinm	r4,r4,0,20,31;
	or		r3,r3,r4;
	lwz		r0,8(sp);
	mtlr		r0;
	blr;

@outOfHere:
	addi		sp,sp,64;
	lwz		r0,8(sp);
	mtlr		r0;
	blr;
}

void NKBATMap(UInt32 batNum,void* logicalAddr,void* physicalAddr,UInt32 len,UInt32 wimg,UInt32 pp)
{
	UInt32	blockEndLog = (UInt32)logicalAddr + len;
	UInt32	blockEndPhys = (UInt32)physicalAddr + len;
	UInt32	blockStartLog;
	UInt32	blockStartPhys;
	UInt32	blockLen;
	UInt32	i;
	UInt32	maxLen = ((_getPVR() & 0xFFFF0000) == 0x00010000) ? 7 : 12;	// 601 has a maximum BAT block length of 8MB  ;-(
	
	// Do a search to find the smallest possible BAT mapping that encompasses these addresses
	for(i=0;i<maxLen;i++)
	{
		blockLen = (1UL << (i+17));
		blockStartLog = ROUND_DOWN(blockLen,(UInt32)logicalAddr);
		blockStartPhys = ROUND_DOWN(blockLen,(UInt32)physicalAddr);
		
		//if(blockStartLog >= blockEndLog - blockLen && blockStartPhys >= blockEndPhys - blockLen)
		if(blockStartLog + blockLen >= blockEndLog && blockStartPhys + blockLen >= blockEndPhys)
			break;
	}
	if(i == maxLen)
		return;
	
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		// Set up a 601 BAT map with WIM = wim(g), Ks = Ku = 1, PP = pp
		// (601 doesn't support the Guarded attribute so we always turn that bit off)
		switch(batNum)
		{
			case 0:
				_setIBAT0L(0);	// Disable IBAT
				_setIBAT0U((blockStartLog & 0xFFFE0000) | ((wimg & 0x0000000E) << 3) | 0x0000000C | pp);
				_setIBAT0L((blockStartPhys & 0xFFFE0000) | 0x00000040 | ((blockLen >> 17) - 1));
			break;
			case 1:
				_setIBAT1L(0);
				_setIBAT1U((blockStartLog & 0xFFFE0000) | ((wimg & 0x0000000E) << 3) | 0x0000000C | pp);
				_setIBAT1L((blockStartPhys & 0xFFFE0000) | 0x00000040 | ((blockLen >> 17) - 1));
			break;
			case 2:
				_setIBAT2L(0);
				_setIBAT2U((blockStartLog & 0xFFFE0000) | ((wimg & 0x0000000E) << 3) | 0x0000000C | pp);
				_setIBAT2L((blockStartPhys & 0xFFFE0000) | 0x00000040 | ((blockLen >> 17) - 1));
			break;
			case 3:
				_setIBAT3L(0);
				_setIBAT3U((blockStartLog & 0xFFFE0000) | ((wimg & 0x0000000E) << 3) | 0x0000000C | pp);
				_setIBAT3L((blockStartPhys & 0xFFFE0000) | 0x00000040 | ((blockLen >> 17) - 1));
			break;
		}
	}
	else
	{
		// Set up a PPC BAT map with Vs = Vp = 1, WIMG = wimg, PP = pp
		switch(batNum)
		{
			case 0:
				_setDBAT0U(0);
				_setDBAT0L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setDBAT0U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
				_setIBAT0U(0);
				_setIBAT0L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setIBAT0U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
			break;
			case 1:
				_setDBAT1U(0);
				_setDBAT1L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setDBAT1U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
				_setIBAT1U(0);
				_setIBAT1L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setIBAT1U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
			break;
			case 2:
				_setDBAT2U(0);
				_setDBAT2L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setDBAT2U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
				_setIBAT2U(0);
				_setIBAT2L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setIBAT2U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
			break;
			case 3:
				_setDBAT3U(0);
				_setDBAT3L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setDBAT3U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
				_setIBAT3U(0);
				_setIBAT3L((blockStartPhys & 0xFFFE0000) | (wimg << 3) | pp);
				_setIBAT3U((blockStartLog & 0xFFFE0000) | (((blockLen >> 17) - 1) << 2) | 0x00000003);
			break;
		}
	}
}

void NKFlushCaches(register void* addr,register UInt32 len)
{
	for(Int32 i=0;i<len;i += 32)
	{
		_dcbf((void*)((UInt32)addr + i));
		_icbi((void*)((UInt32)addr + i));
	}
}

UInt32 NKMaxContig(register void* logicalAddr,ProcessID processID)
{
	Int8*	addr = (Int8*)logicalAddr;
	Int8*	prevAddr;
	UInt32	leftOver = (0x00001000 - ((Int32)addr & 0x00000FFF));
	UInt32	numBytes = leftOver;
	
	addr += leftOver;
	if((Int32)NKGetPhysical(logicalAddr,processID) + leftOver == (Int32)NKGetPhysical(addr,processID))
	{
		numBytes += 0x00001000;
		
		prevAddr = addr;
		addr += 0x00001000;
		while((Int32)NKGetPhysical(prevAddr,processID) + 0x00001000 == (Int32)NKGetPhysical(addr,processID))
		{
			numBytes += 0x00001000;
			prevAddr = addr;
			addr += 0x00001000;
		}
	}
	
	return numBytes;
}
