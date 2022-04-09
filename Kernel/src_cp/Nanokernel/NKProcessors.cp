/*
	NKProcessors.cp
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
#include "NKProcessors.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKInterruptVectors.h"
#include "NKTimers.h"

static volatile Boolean	processorsInited = false;
static ProcessorInfo*	processor;
static NKSpinLock		processorInitLock;

// This is used to look up the PVR and get info on that chip.  If (_getPVR() & mask) == pvr then
// you have a match, however you MUST search the table from entry 0 to the end in order.
typedef struct PVRToNameTableEntry
{
	const UInt32		pvr;
	const UInt32		mask;
	ConstASCII8Str	name;
}PVRToNameTableEntry;

static PVRToNameTableEntry	processorTable[]=	{	// 601 Machines
											{0x00010000,0xFFFF0000,	"601 rev ?"},
											// 603 Machines
											{0x00030302,0xFFFFFFFF,	"603 rev 3.2"},
											{0x00030000,0xFFFF0000,	"603 rev ?"},
											// 604 Machines
											{0x00040300,0xFFFFFFFF,	"604 rev 3.01"},
											{0x00040301,0xFFFFFFFF,	"604 rev 3.1"},
											{0x00040302,0xFFFFFFFF,	"604 rev 3.2"},
											{0x00040303,0xFFFFFFFF,	"604 rev 3.3"},
											{0x00040304,0xFFFFFFFF,	"604 rev 3.4"},
											{0x00040305,0xFFFFFFFF,	"604 rev 3.5"},
											{0x00040306,0xFFFFFFFF,	"604 rev 3.6"},
											{0x00040307,0xFFFFFFFF,	"604 rev 3.7"},
											{0x00040400,0xFFFFFFFF,	"604 rev 4"},
											{0x00040500,0xFFFFFFFF,	"604 rev 5"},
											{0x00040501,0xFFFFFFFF,	"604 rev 5.1"},
											{0x00040601,0xFFFFFFFF,	"604 rev 6.1"},
											{0x00040000,0xFFFF0000,	"604 rev ?"},
											// 603e machines
											{0x00060101,0xFFFFFFFF,	"603e (Stretch) rev 1.1"},
											{0x00060102,0xFFFFFFFF,	"603e (Stretch) rev 1.2"},
											{0x00060103,0xFFFFFFFF,	"603e (Stretch) rev 1.3"},
											{0x00060104,0xFFFFFFFF,	"603e (Stretch) rev 1.4"},
											{0x00060202,0xFFFFFFFF,	"603e (Stretch) rev 2.2"},
											{0x00060300,0xFFFFFFFF,	"603e (Stretch) rev 3"},
											{0x00060400,0xFFFFFFFF,	"603e (Stretch) rev 4"},
											{0x00060000,0xFFFF0000,	"603e (Stretch) rev ?"},
											// 603e-PID7v machines
											{0x00070100,0xFFFFFFFF,	"603ev (Valiant) rev 1"},
											{0x00070200,0xFFFFFFFF,	"603ev (Valiant) rev 2"},
											{0x00070201,0xFFFFFFFF,	"603ev (Valiant) rev 2.1"},
											// 603e-PID7t machines
											{0x00071201,0xFFFFFFFF,	"603r (Goldeneye) rev 1"},
											{0x00070000,0xFFFF0000,	"603ev rev ?"},
											// 740/750 machines
											{0x00080100,0xFFFFFFFF,	"740/750 (Arthur) rev 1"},
											{0x00080200,0xFFFFFFFF,	"740/750 (Arthur) rev 2"},
											{0x00080201,0xFFFFFFFF,	"740/750 (Arthur) rev 2.1"},
											{0x00080202,0xFFFFFFFF,	"740/750 (Arthur) rev 2.2"},
											{0x00080000,0xFFFF0000,	"740/750 (Arthur) rev ?"},
											// 604e-PID9v machines
											{0x00090200,0xFFFFFFFF,	"604e (Sirocco) rev 2"},
											{0x00090201,0xFFFFFFFF,	"604e (Sirocco) rev 2.1"},
											{0x00090202,0xFFFFFFFF,	"604e (Sirocco) rev 2.2"},
											{0x00090203,0xFFFFFFFF,	"604e (Sirocco) rev 2.3"},
											{0x00090204,0xFFFFFFFF,	"604e (Sirocco) rev 2.4"},
											{0x00090000,0xFFFF0000,	"604e (Sirocco) rev ?"},
											// 604e-PID9q machines
											{0x000A0100,0xFFFFFFFF,	"604ev (Mach V) rev 1"},
											{0x000A0000,0xFFFF0000,	"604ev (Mach V) rev ?"},
											// 7400 machines
											{0x000C0100,0xFFFFFFFF,	"7400 (Max) rev 1"},
											{0x000C0200,0xFFFFFFFF,	"7400 (Max) rev 2.0"},
											{0x000C0201,0xFFFFFFFF,	"7400 (Max) rev 2.1"},
											{0x000C0202,0xFFFFFFFF,	"7400 (Max) rev 2.2"},
											{0x000C0206,0xFFFFFFFF,	"7400 (Max) rev 2.6"},
											{0x000C0207,0xFFFFFFFF,	"7400 (Max) rev 2.7"},
											{0x000C0000,0xFFFF0000,	"7400 (Max) rev ?"},
											// 620 machines (probably unsupported)
											{0x00141103,0xFFFFFFFF,	"620 (Red October) rev 1.3"},
											{0x00141104,0xFFFFFFFF,	"620 (Red October) rev 1.4"},
											{0x00141200,0xFFFFFFFF,	"620 (Red October) rev 2"},
											{0x00140000,0xFFFF0000,	"620 (Red October) rev ?"},
											// End of table
											{0x00000000,0x00000000,	nil}
										};
void NKInitProcessors()
{
	processorInitLock.init();
	
	processor = (ProcessorInfo*)machine.kernelEnd;
	machine.kernelEnd = (void*)((UInt32)machine.kernelEnd + sizeof(ProcessorInfo)*NKGetNumProcessors());
	_sync();
	processorsInited = true;
}

void NKInitThisProcessor()
{
	// Wait for the ProcessorInfo structs to be allocated
	while(!processorsInited)
		;
	
	// Zap all the time bases ASAP
	zapClockSync();
	
	// Only 1 processor at a time, just so text on the screen doesn't get messed up
	NKSpinLocker		processorInitLocker(processorInitLock);
	
	// Set up our ProcessorInfo struct and shove it into SPRG0
	ProcessorInfo*	myProcessorInfo = &processor[_getSPRG0()];
	myProcessorInfo->pvr = _getPVR();
	PVRToNameTableEntry*	entry = processorTable;
	while(entry->pvr)
	{
		if((myProcessorInfo->pvr & entry->mask) == entry->pvr)
		{
			myProcessorInfo->processorName = entry->name;
			break;
		}
		entry++;
	}
	if(!entry->pvr)
		myProcessorInfo->processorName = "Unknown processor!";
	myProcessorInfo->number = _getSPRG0();
	myProcessorInfo->hz = 0;
	myProcessorInfo->kernelRTOC = _getRTOC();
	myProcessorInfo->thread = nil;
	myProcessorInfo->idleThread = nil;
	myProcessorInfo->timerHead = nil;
	_setSPRG0((UInt32)myProcessorInfo);
	
	nkVideo << "Initializing processor " << myProcessorInfo->number << "\n";
	
	// Initialize the interrupt vectors first (must do before turning on page tables, for 603 chips)
	NKInitInterruptVectorsOnThisProcessor();
	
	// Set up the segment registers
	NKInitSRs();
	
	// Set up our page table descriptor register
	NKInitSDR1();
	
	// Flush 4MB to empty the caches
	UInt32 oldMSR = DisableDR();
	DisableIR();
	NKFlushCaches(0,4096*1024);
	SetMSR(oldMSR);
	
	// Invalidate the TLB's before switching over to page tables.
	NKInvalidateTLB();
	
	// Switch over to the Page Tables
	NKDisableBATMap();
}

ProcessorInfo* NKGetThisProcessorInfo()
{
	return (ProcessorInfo*)_getSPRG0();
}

UInt32 NKGetNumProcessors()
{
	return machine.numProcessors;
}

ProcessorInfo* NKGetProcessorInfo(UInt32 n)
{
	if(n < NKGetNumProcessors())
		return &processor[n];
	else
		return nil;
}
