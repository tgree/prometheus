/*
	NKProcessors.h
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
	Terry Greeniaus	-	Friday, Sept. 4, 1998	-	Original creation of file
*/
#ifndef __NKPROCESSORS__
#define __NKPROCESSORS__

#include "NKTimers.h"
#include "NKAtomicOperations.h"

typedef struct ProcessorInfo
{
	UInt32			pvr;			// The pvr for this processor
	ConstASCII8Str		processorName;	// The name of this processor
	UInt32			number;		// The number for this processor (processors are numbered in consecutively increasing integers)
	UInt32			hz;			// The hz of this processor (CURRENTLY ONLY USED ON BOOT PROCESSOR!)
	UInt32			busHz;		// The speed of the system bus on this processor (CURRENTLY ONLY USED ON BOOT PROCESSOR!)
	UInt32			decHz;		// The speed of the decrementor on this processor (DITTO)
	UInt32			kernelRTOC;	// The RTOC of the kernel (for the preempter)
	class Thread*		thread;		// The current thread running on this processor
	class Thread*		idleThread;	// The idle thread for this processor
	class NKTimer*	timerHead;	// The queue of timers running on this processor
}ProcessorInfo;

void					NKInitProcessors();			// Called by the nanokernel to set up ProcessorInfo structs
void					NKInitThisProcessor();		// Initializes this processor.  Call this ONCE for each processor after page tables have been set up
ProcessorInfo*			NKGetThisProcessorInfo();	// Gets the ProcessorInfo struct for the processor which is running WHEN this routine is called (may be different by the time it returns, though!)
UInt32				NKGetNumProcessors();		// Returns the number of processors in the system
ProcessorInfo*			NKGetProcessorInfo(UInt32 n);	// Gets info for processor n (returns nil if no such processor n)

#endif /* __NKPROCESSORS__ */
