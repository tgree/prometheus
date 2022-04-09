/*
	NKVirtualMemory.h
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
#ifndef __VIRTUAL_MEMORY__
#define __VIRTUAL_MEMORY__

#include "Compiler.h"
#include "NKProcesses.h"
#if BIG_ENDIAN

typedef struct _PTE
{
	// First long
	UInt32	v		:	1;
	UInt32	vsid		:	24;
	UInt32	h		:	1;
	UInt32	api		:	6;
	
	// Second long
	UInt32	rpn		:	20;
	UInt32	rsrv1	:	3;
	UInt32	r		:	1;
	UInt32	c		:	1;
	UInt32	wimg	:	4;
	UInt32	rsrv2	:	1;
	UInt32	pp		:	2;
}_PTE;

typedef union PTE
{
	_PTE		pte;
	UInt32	pteData[2];
}PTE;

#else

#error Don't know anything about little-endian PTE's

#endif /* __BIG_ENDIAN__ */

enum
{
	WIMG_GUARDED		=	1,
	WIMG_COHERENT		=	2,
	WIMG_CACHE_INHIBITED	=	4,
	WIMG_WRITE_THRU		=	8
};

enum
{
	PP_NO_ACCESS	=	0,
	PP_READ_ONLY		=	1,
	PP_READ_WRITE	=	2
};

extern PTE*			PTBase;
extern UInt32			PTLen;

void			NKInitVirtualMemory(void);
void			NKInvalidateTLB(void);
void			NKMapPage(void* logicalAddr,void* physicalAddr,UInt32 wimg,UInt32 pp,ProcessID processID = PROCESS_CURRENT);
void*		NKUnmapPage(void* logicalAddr,ProcessID processID);	// Returns the physical page address that the page was mapped to before unmapping
void			NKMapRange(void* logicalAddr,void* physicalAddr,UInt32 lenBytes,UInt32 wimg,UInt32 pp,ProcessID processID = PROCESS_CURRENT);
void*		NKIOMap(void* physicalAddr,UInt32 len,UInt32 wimg,UInt32 pp);	// Maps pages in the kernel IO space (starting below 0x10000000) and returns the logical mapping
void			NKBATMap(UInt32 batNum,void* logicalAddr,void* physicalAddr,UInt32 len,UInt32 wimg,UInt32 pp);
void			NKSaveBATMap( UInt32 batNum, UInt32& up, UInt32& down );
void			NKSetBATMap( UInt32 batNum, UInt32 up, UInt32 down );
void			NKZapBATMap( UInt32 batNum );
void			NKDisableBATMap(void);
void			NKFlushCaches(register void* addr,register UInt32 len);
PTE*		NKGetPTE(register void* logicalAddr,register UInt32 processID);
void*		NKGetPhysical(register void* logicalAddr,register UInt32 processID);
UInt32		NKMaxContig(register void* logicalAddr,ProcessID processID);		// Returns the number of contiguous physical bytes starting at this address

// The following functions are for use ONLY by NKInitThisProcessor()
void			NKInitSRs(void);
void			NKInitSDR1(void);

#endif /* __VIRTUAL_MEMORY__ */