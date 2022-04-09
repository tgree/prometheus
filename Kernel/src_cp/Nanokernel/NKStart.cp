/*
	NKStart.cp
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
	Terry Greeniaus	-	Friday, 4 Sept. 98	-	Added MP entry support to nanokernel boot procedure
	Terry Greeniaus	-	Wed, 27 Oct 99		-	Added support for G4 (7400) machines
										-	Disabled L2CR at boot - this should be autodetected by the nanokernel later in the boot process,
											because we can't guarantee that it was enabled prior to the kernel being loaded and want to leave the
											kernel in a known state.  Only disabled on 750 (G3) and 7400 (G4) processors.  (Of course,
											the code to autodetect is also proprietary and can't be released under GPL... feel free to remove
											the L2CR stuff in this file if you want your kernel to go faster until we can release autodetect code).
	Terry Greeniaus	-	Sun, 31 Jan 2000	-	Changes in preparation for MP stuff.  Removed L2 cache disable code (dumb idea anyhow).
	Terry Greeniaus	-	Wed, 2 Feb 2000	-	Removed G4 errata fix.  Those were available to me under NDA and cannot be distributed.  Hopefully
											this will not cause any problems in the kernel.
*/

#include "NKMachineInit.h"
#include "NKVideo.h"
#include "NKInterruptVectors.h"
#include "NKVirtualMemory.h"
#include "NKThreads.h"
#include "NKFonts.h"
#include "NKProcesses.h"
#include "NKProcessors.h"
#include "NKSerialDebugger.h"
#include "Gonzales.h"
#include "Assembly.h"
#include "Kernel Types.h"
#include "Macros.h"
#include "Config.h"
#include "PEFBinaryFormat.h"
#include "ANSI.h"
#include "Block Device.h"
#include "BootInfo.h"
#include "NKTimers.h"

#define	THREE_MB_MAP	false

extern "C"
{
	void	NKStart(void);	// This is the boot point entry into the nanokernel.  Must be a C function because for PEF Main Routine linkage
}
static	void	NKInit(void);								// NKStart jumps to here after it has set some stuff up
static	void	NKTestVarArgs(UInt32 sum,UInt32 numArgs,...);	// Takes a sum, an argument count and a bunch of UInt32 args.  It adds them up and compares the sum with the give sum.
extern	void	MicroKernelStart(void);

static UInt32	entryLock[4] = {0,0,0,0};	// Lock for entry into kernel (for MP support)
static UInt32	processorNumber = 0;
static UInt32	machineStructInitialized = false;	// Will be set to true when the processor with the boot info finished writing the "machine" global

__asm__ void NKStart(void)
{
	// r28 holds screen address for debugging purposes
	stw		r28,0(r28);
	sync;
	dcbf		r0,r28;
	sync;
	
	// Make sure the MSR is 0 - we are in code that is now mapped 1-1, so it is safe to turn off all relocations, without having
	// the PC "jump" from a virtual address to a physical one which doesn't map to the same virtual one.  Setting the MSR to zero
	// also results in low interrupt vectors being used, all floating point operations disabled, machine check exceptions disabled,
	// external exceptions disabled and lots of other neat stuff.  I'll document it later...  It is safe to do this on a 603, because with
	// translation disabled we have no need for memory management interrupts.
	li		r31,0;
	sync;
	mtmsr	r31;
	sync;
	
	// Accquire a lock on the "processorNumber" global
	li		r31,1;
	lwz		r30,entryLock(rtoc);
@entryLockSpin:
	lwarx	r29,r0,r30;
	cmpwi	r29,0;
	bne		@entryLockSpin;
	stwcx.	r31,r0,r30;
	bne-		@entryLockSpin;
	isync;
	
	// Get our processor number and increment by one
	lwz		r27,processorNumber(rtoc);
	mtsprg(0,r27);
	addi		r27,r27,1;
	stw		r27,processorNumber(rtoc);
	
	// Release the kernel entry lock
	li		r31,0;
	stw		r31,0(r30);
	
	// Info is passed in from the booter to us here.  We record it in the MachineInfo structure for later use.
	//	r0	=	end of kernel memory, just before stack
	//	r3	=	bootInfo struct
	//	r4	=	number of processors if boot processor, 0 if secondary processor
	//	r7	=	kernel end
	//	r14	=	kernel stack base
	cmpwi	r4,0;
	lwz		r31,machine(rtoc);
	bne		@bootInfoProcessor;
@waitForBootInfo:
	lwz		r3,machineStructInitialized(rtoc);
	cmpwi	r3,0;
	beq		@waitForBootInfo;
	b		@initProcessor;
	
@bootInfoProcessor:
	//mfpvr(r4);
	//stw		r4,MachineInfo.pvr(r31);
	
	lwz		r4,BootInfo.gestaltMachType(r3);
	stw		r4,MachineInfo.gestaltMachineType(r31);
	
	lwz		r4,BootInfo.busSpeed(r3);
	stw		r4,MachineInfo.busClock(r31);
	
	lwz		r4,BootInfo.physMemSize(r3);
	stw		r4,MachineInfo.memSize(r31);
	
	lwz		r4,BootInfo.pefKernelCodeSection(r3);
	stw		r4,MachineInfo.kernelStart(r31);
	
	stw		r0,MachineInfo.kernelPreStack(r31);
	
	addi		r7,r7,0x001F;		// Round kernelEnd up to a 32 byte boundary
	rlwinm	r7,r7,0,0,26;
	stw		r7,MachineInfo.kernelEnd(r31);
	
	stw		r14,MachineInfo.stackBase(r31)
	
	lwz		r4,BootInfo.pefKernelCodeSection(r3);
	stw		r4,MachineInfo.codeStart(r31);
	
	lwz		r4,BootInfo.pefKernelDataSection(r3);
	stw		r4,MachineInfo.dataStart(r31);
	
	lwz		r4,BootInfo.pefKernelLoaderSection(r3);
	stw		r4,MachineInfo.pefLoaderInfo(r31);
	
	lwz		r4,BootInfo.screenAddr(r3);
	stw		r4,MachineInfo.videoParams.logicalAddr(r31);
	
	lhz		r4,BootInfo.pixRes(r3);
	stw		r4,MachineInfo.videoParams.pixSize(r31);
	
	lhz		r4,BootInfo.rowBytes(r3);
	stw		r4,MachineInfo.videoParams.rowBytes(r31);
	
	lhz		r4,BootInfo.width(r3);
	stw		r4,MachineInfo.videoParams.width(r31);
	
	lhz		r4,BootInfo.height(r3);
	stw		r4,MachineInfo.videoParams.height(r31);
	
	lwz		r4,BootInfo.kernelDirID(r3);
	stw		r4,MachineInfo.dirInfo.dirID(r31);
	
	lwz		r4,BootInfo.kernelVolumeChecksum(r3);
	stw		r4,MachineInfo.dirInfo.checkSum(r31);
	
	lwz		r4,BootInfo.openFirmwareTreeLen(r3);
	stw		r4,MachineInfo.openFirmwareFlattenedLen(r31);
	
	lwz		r4,BootInfo.numProcessors(r3);
	stw		r4,MachineInfo.numProcessors(r31);
	
	// Get the physical address of the screen.  We have to do this before we kill the segment registers.  No need to create a stack frame
	// since we don't need to store any machine state for this routine (it never returns).
	lwz		r3,MachineInfo.videoParams.logicalAddr(r31);
	li		r4,PROCESS_CURRENT;
	bl		NKGetPhysical;
	stw		r3,MachineInfo.videoParams.physicalAddr(r31);
	
	// Leave a nil back-chain on the stack
	li		r3,0;
	stw		r3,0(sp);
	
	// Let other processors jump in now
	li		r3,1;
	stw		r3,machineStructInitialized(rtoc);
	
	// Initialize HID registers here.  Add more selectors for different chips later.
	// Also set up a memory map with the BAT registers, so that the first 24MB of memory
	// are mapped 1-1 through BAT0-BAT2.  BAT3 is left for video later.  Data/Code relocation
	// is turned on at the end.  The reason we set up this mapping is so that we can use video before we have
	// page tables set up.  Video must be marked as write-through (at least) memory, so that writing to
	// video memory writes to the screen.  Default WIMG bits when address translation is turned off
	// results in video data being cached, and so the output look weird on the screen (many bits don't
	// get drawn to the screen until they are randomly flushed later).  However, if we want to map
	// video memory, we need to map code too.  Note that when messing with memory mapping, we don't
	// need to use any synchronizing instructions because relocation is disabled.
@initProcessor:
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,1;
	lwz		r12,MachineInfo.kernelStart(r31);
	beq		@601Init;
	b		@non601Init;

@601Init:	// Initialize 601 HID registers to Hard RESET values.  This makes tracing work later, since the 601 uses
	// a non-standard single-step trace exception.  It must be configured through HID0 first.
	lis		r30,0x8001;
	ori		r30,r30,0x0080;
	mtspr	HID0,r30;
	li		r30,0;
	mtspr	HID2,r30;
	mtspr	DABR,r30;
	oris		r30,r30,0x0080;
	mtspr	HID1,r30;
	sync;
	
	// Disable BAT mapping
	li		r3,0;
	mtibatl(0,r3);
	mtibatl(1,r3);
	mtibatl(2,r3);
	mtibatl(3,r3);

#if THREE_MB_MAP
	// Map first 3MB _from start of kernel_ 1-1 via BAT0-BAT2 - we assume that the kernel is aligned to a 1MB boundary.
	rlwinm	r3,r12,0,0,14;	// r12 contains the start of the kernel
	ori		r3,r3,0x001E;		// WIM = M, Ks = Ku = 1, PP = r/w
	mtibatu(0,r3);
	addis		r3,r3,0x0010;
	mtibatu(1,r3);
	addis		r3,r3,0x0010;
	mtibatu(2,r3);
	rlwinm	r3,r12,0,0,14;
	ori		r3,r3,0x0047;		// V = 1, BSM = 1MB
	mtibatl(0,r3);
	addis		r3,r3,0x0010;
	mtibatl(1,r3);
	addis		r3,r3,0x0010;
	mtibatl(2,r3);
#else
	// Map first 24MB of memory 1-1 via BAT0-BAT2.
	li		r3,0x001E;
	mtibatu(0,r3);
	addis		r3,r3,0x0080;
	mtibatu(1,r3);
	addis		r3,r3,0x0080;
	mtibatu(2,r3);
	li		r3,0x007F;
	mtibatl(0,r3);
	addis		r3,r3,0x0080;
	mtibatl(1,r3);
	addis		r3,r3,0x0080;
	mtibatl(2,r3);
#endif
	b		@doneInit;
	
@non601Init:
	// A generic init routine for non-601 processors.
	
	// [Configure HID registers here]
	
	// Disable BAT mapping
	li		r3,0;
	mtibatu(0,r3);
	mtibatu(1,r3);
	mtibatu(2,r3);
	mtibatu(3,r3);
	mtdbatu(0,r3);
	mtdbatu(1,r3);
	mtdbatu(2,r3);
	mtdbatu(3,r3);
	
#if THREE_MB_MAP
	// Map first 3MB _from start of kernel_ 1-1 via BAT0-BAT2 - we assume that the kernel is aligned to a 1MB boundary.
	rlwinm	r3,r12,0,0,14;	// r12 contains the start of the kernel
	ori		r3,r3,0x0012;		// WIMG = M, PP = r/w
	mtdbatl(0,r3);
	mtibatl(0,r3);
	addis		r3,r3,0x0010;
	mtdbatl(1,r3);
	mtibatl(1,r3);
	addis		r3,r3,0x0010;
	mtdbatl(2,r3);
	mtibatl(2,r3);
	rlwinm	r3,r12,0,0,14;
	ori		r3,r3,0x001F;		// BL = 1MB, Vs = Vp = 1
	mtdbatu(0,r3);
	mtibatu(0,r3);
	addis		r3,r3,0x0010;
	mtdbatu(1,r3);
	mtibatu(1,r3);
	addis		r3,r3,0x0010;
	mtdbatu(2,r3);
	mtibatu(2,r3);
#else
	// Map first 24MB of memory 1-1 via BAT0-BAT2.
	li		r3,0x0012;
	mtdbatl(0,r3);
	mtibatl(0,r3);
	addis		r3,r3,0x0080;
	mtdbatl(1,r3);
	mtibatl(1,r3);
	addis		r3,r3,0x0080;
	mtdbatl(2,r3);
	mtibatl(2,r3);
	li		r3,0x00FF;
	mtdbatu(0,r3);
	mtibatu(0,r3);
	addis		r3,r3,0x0080;
	mtdbatu(1,r3);
	mtibatu(1,r3);
	addis		r3,r3,0x0080;
	mtdbatu(2,r3);
	mtibatu(2,r3);
#endif
	
	// Do some G3 and G4 specific errata fixes
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,0x0008;	// G3 processor
	b		@initG3;
	cmpwi	r3,0x000C;	// G4 processor
	b		@initG4;
	b		@doneInit;
	
@initG3:
	// Disable HID0[DPM] on G3, fix for errata concerning L2 cache
	mfhid0(r3);
	rlwinm	r3,r3,0,12,10;	// No HID0[DPM];
	mthid0(r3);
	b		@doneInit;
	
@initG4:
	// Lots of G4 errata
	// I removed this code because I am under NDA and cannot distribute information regarding G4 errata.  However I left in the
	// table so when the errata are published we can just pop in the fixes here.
	mfpvr(r3);
	rlwinm	r3,r3,0,16,31;
	cmpwi	r3,0x0200;
	ble		@G4errata2_0;
	cmpwi	r3,0x0201;
	ble		@G4errata2_1;
	cmpwi	r3,0x0202;
	ble		@G4errata2_2;
	cmpwi	r3,0x0206;
	ble		@G4errata2_6;
	cmpwi	r3,0x0207;
	ble		@G4errata2_7;
	b		@doneInit;
	@G4errata2_0:	// Fix for errata on rev 2.0 or less [4,6,7,8,9,10]
		// Place errata [4,6,7,8,9,10] fix here
	@G4errata2_1:	// Fix for errata on rev 2.1 [12]
		// Place errata [12] fix here
	@G4errata2_2:	// Fix for errata on rev 2.2 [13,14]
	@G4errata2_6:	// Fix for errata on rev 2.6 [13,14]
	@G4errata2_7:	// Fix for errata on rev 2.7 [13,14]
		// Place errata [13,14] fix here
		b	@doneInit;
	
@doneInit:
	// Kill all the segment registers.  This is basically for 601 chips - they have a special "Memory Forced I/O" setting that allows a segment
	// register to map an entire 256 MB segment.  The trick is that for memory translation, the segment register will take precedence _OVER_
	// the BAT register, if a memory forced I/O segment is set up.  This is contrary to the PPC specification that BAT registers always take
	// precedence over segment registers.  Also, it can't hurt to zap all the segment registers anyhow.
	lis		r3,0x7012;
	ori		r3,r3,0x3456;	// T = 0, Ks = Kp = 1, N = 1 (no execute), VSID = 0x00123456
	mtsr		0,r3;
	mtsr		1,r3;
	mtsr		2,r3;
	mtsr		3,r3;
	mtsr		4,r3;
	mtsr		5,r3;
	mtsr		6,r3;
	mtsr		7,r3;
	mtsr		8,r3;
	mtsr		9,r3;
	mtsr		10,r3;
	mtsr		11,r3;
	mtsr		12,r3;
	mtsr		13,r3;
	mtsr		14,r3;
	mtsr		15,r3;
	
	// Kill the TLB (just in case)
	bl		NKInvalidateTLB;
	
	// Enable Data/Code relocation via the new BAT map.  Also turn IP prefix high so on 603 we can use the page table mapping
	// of the screen before we initialize ROM interrupt vectors.  Finally, enable the FPU
	mfmsr	r3;
	ori		r3,r3,0x2070;
	sync;
	mtmsr	r3;
	sync;
	
	// See if we are the boot processor
	mfsprg(r3,0);
	cmpwi	r3,0;
	
	// Trash all registers so we don't use them accidentally
	lis		r0,0xDEAD;
	ori		r0,r0,0xBEEF;
	mr		r3,r0;
	mr		r4,r0;
	mr		r5,r0;
	mr		r6,r0;
	mr		r7,r0;
	mr		r8,r0;
	mr		r9,r0;
	mr		r10,r0;
	mr		r11,r0;
	mr		r12,r0;
	mr		r13,r0;
	mr		r14,r0;
	mr		r15,r0;
	mr		r16,r0;
	mr		r17,r0;
	mr		r18,r0;
	mr		r19,r0;
	mr		r20,r0;
	mr		r21,r0;
	mr		r22,r0;
	mr		r23,r0;
	mr		r24,r0;
	mr		r25,r0;
	mr		r26,r0;
	mr		r27,r0;
	mr		r28,r0;
	mr		r29,r0;
	mr		r30,r0;
	mr		r31,r0;
@secondaryWait:
	bne		@secondaryWait;	// Secondary processors don't wake up yet.
	
	b		NKInit;
}

void NKInit(void)
{
	// Once we get here, we are running with data relocation on.  The first 3MB of memory are mapped 1-1 via the first three BAT register
	// arrays.  The fourth BAT register is reserved for video mapping.  We can proceed to boot mostly in C++ code now.
	
	// Wait for all processors to finish initializing
	//while(processorNumber != NKGetNumProcessors())
	//	;
	
	// Initialize the parameters for the machine struct
	NKMachineInit();
	
	// Initialize the fonts
	NKInitFonts();
	
	// Initialize the nanokernel video driver.
	Rect	videoBounds = {0,4*machine.videoParams.height/5,machine.videoParams.width,machine.videoParams.height};
	NKInitVideo(&nkVideo,&videoBounds,true,gonzales5);
	
	// Hello world!
	nkVideo << "Nanokernel " << nanokernelVers << "\n__________________\n";
	
	// Set up interrupts to vector to our low memory locations, rather than high-memory ROM locations.  This must be done (on a 603) BEFORE
	// paged virtual memory is turned on, so that the 603 memory management exceptions will already be functioning.  On the other hand, if you are
	// debugging stuff on a non-603 chip, it is safe to postpone this call until later, when drivers that rely on functioning interrupts are initialized.
	// Also, it grabs some memory after the end of the kernel for stacks, and thus must be mapped later by NKInitVirtualMemory().
	NKInitInterruptVectors();
	
	// Initialize the debugger nub.
	nkVideo << "Initializing debugger nub\n";
	NKInitDebuggerNub();
	dout << "Testing simple debugger nub\n";
	
	// Initialize processors.  This sets up ProcessorInfo structs for each processor.  Each processor must call NKInitThisProcessor() on it's own later.
	nkVideo << "Initializing multi-processing info (" << NKGetNumProcessors() << " processors)\n";
	NKInitProcessors();
	
	// Set up paged VM.  It maps all of the kernel 1-1, the page table 1-1, the IO device space (not 1-1), and calls NKRemapVideo to remap the video with the page table (not 1-1).
	// After the call to NKInitVirtualMemory(), you must NEVER use machine.kernelEnd to grab more memory for nanokernel resources.
	nkVideo << "Initializing paged virtual memory\n";
	NKInitVirtualMemory();
	
	// Seed the ANSI random number generator
	srand((UInt32)_getClock());
	
	// Initialize the boot processor.  This turns on the page tables for this processor and all memory stuff is done via the page tables after this call.
	// It also zeroes the time base on all processors, so it's done after the rand seed is done
	nkVideo << "Initializing boot processor\n";
	NKInitThisProcessor();
	
	// Initialize the serial debugger if we can.  Currently this is only done on 72/73/75/76/86/86/95/9600 machines...  see NKMachineInit for more info.
	NKInitSerialDebugger();
	
	// Initialize the memory manager
	nkVideo << "Initializing memory manager\n";
	NKInitMemoryManager();
	
	// Set up processes.  This must be done before initializing threads, as all threads stuff depends on knowing the current process and having a current process.
	// It is safe to call this before initializing new() and delete(), and before turning on interrupts, so we place the call in the nano kernel.
	nkVideo << "Initializing processes\n";
	NKInitProcesses();
	
	// Start timers
	NKInitTimers();
	
	// Test variable length arguments
	NKTestVarArgs(15,5,1,2,3,4,5);
	
	// Start booting the kernel
	nkVideo << "\nBooting Microkernel!\n";
	MicroKernelStart();
}

void NKTestVarArgs(UInt32 sum,UInt32 numArgs,...)
{
	va_list	argList;
	UInt32	mySum = 0;
	
	nkVideo << "Varargs test: ";
	va_start(argList,numArgs);
	for(UInt32 i=0;i<numArgs;i++)
	{
		UInt32	arg = va_arg(argList,UInt32);
		mySum += arg;
		nkVideo << arg << " ";
	}
	nkVideo << ": " << (mySum == sum ? "Succeeded\n" : "Failed\n");
	
	va_end(argList);
}
