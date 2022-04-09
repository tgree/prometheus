/*
	NKInterruptVectors.cp
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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Monday, 3 August 98	-	Removed all the 0x000n_nnnn interrupt stuff, we always use ROM now
	Terry Greeniaus	-	Monday, 24 August 98	-	Re-worked code so that sprg0 is now reserved (contains a current Thread pointer for the processor,
												looking ahead to multi-processing support).  This required some pretty complicated stuff - now we have NO
												scratch sprg registers at interrupt time!!!!!
	Patrick Varilly		-	Sunday, 28 March 99	-	Added support for 0x000n_nnnn interrupt stuff, since iMac requires it
												(only with iMac Update 1.1, though)
*/
#include "NKInterruptVectors.h"
#include "NKROMInterruptVectors.h"
#include "NKOurInterruptVectors.h"
#include "NKVideo.h"
#include "NKMachineInit.h"
#include "Macros.h"
#include "Assembly.h"
#include "Kernel Types.h"
#include "Config.h"
#include "ANSI.h"

static	void	UnimplementedVector(void);
static	void	SystemReset(void);
static	void	MachineCheck(void);
static	void	DataStorage(void);
static	void	InstructionStorage(void);
static	void	External(void);
static	void	Alignment(void);
static	void	Program(void);
static	void	FloatingPointUnavailable(void);
static	void	Decrementer(void);
static	void	SystemCall(void);
static	void	Trace(void);
static	void	FloatingPointAssist(void);
static	void IOControllerInterfaceError(void);

static	void	HandleInterrupt(UInt32 vecNum);

#define NUM_INT_STACKS	4
#define INT_STACK_SIZE		10240

static UInt32					handlerScratch[48][4];
UInt32						VectorTable[48][2];
static Int8*					interruptStack[NUM_INT_STACKS];
static Int8**					currInterruptStack;
static volatile Boolean			canInitOtherProcessors = false;
static Boolean					usingROMVectors = false;

void NKInitInterruptVectors(void)
{
	// Set up handlerScratch and VectorTable.  We do this here like this (a seperate table copied over) because it turns out
	// that if you do it with a global already set up, it generates a static initialization function to do it, and we don't call
	// the static initialization functions until we get to the micorkernel.
	UInt32		 tempVectorTable[48][2]	=	{	{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[0]},
											{FUNC_ADDR(SystemReset),(UInt32)&handlerScratch[1]},
											{FUNC_ADDR(MachineCheck),(UInt32)&handlerScratch[2]},
											{FUNC_ADDR(DataStorage),(UInt32)&handlerScratch[3]},
											{FUNC_ADDR(InstructionStorage),(UInt32)&handlerScratch[4]},
											{FUNC_ADDR(External),(UInt32)&handlerScratch[5]},
											{FUNC_ADDR(Alignment),(UInt32)&handlerScratch[6]},
											{FUNC_ADDR(Program),(UInt32)&handlerScratch[7]},
											{FUNC_ADDR(FloatingPointUnavailable),(UInt32)&handlerScratch[8]},
											{FUNC_ADDR(Decrementer),(UInt32)&handlerScratch[9]},
											{FUNC_ADDR(IOControllerInterfaceError),(UInt32)&handlerScratch[10]},	// A 601-only vector (should be UnimplementedVector on non-601)
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[11]},
											{FUNC_ADDR(SystemCall),(UInt32)&handlerScratch[12]},
											{FUNC_ADDR(Trace),(UInt32)&handlerScratch[13]},					// A non-601 vector (should be UnimplementedVector on 601)
											{FUNC_ADDR(FloatingPointAssist),(UInt32)&handlerScratch[14]},			// A non-601 vector (should be UnimplementedVector on 601)
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[15]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[16]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[17]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[18]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[19]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[20]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[21]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[22]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[23]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[24]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[25]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[26]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[27]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[28]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[29]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[30]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[31]},
											{FUNC_ADDR(Trace),(UInt32)&handlerScratch[32]},					// A 601-only vector (should be Unimplemented on non-601)
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[33]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[34]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[35]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[36]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[37]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[38]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[39]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[40]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[41]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[42]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[43]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[44]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[45]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[46]},
											{FUNC_ADDR(UnimplementedVector),(UInt32)&handlerScratch[47]}
										};
	static UInt32		tempHandlerScratch[48][4] ={	{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(SystemReset),0,0,0},
											{FUNC_RTOC(MachineCheck),0,0,0},
											{FUNC_RTOC(DataStorage),0,0,0},
											{FUNC_RTOC(InstructionStorage),0,0,0},
											{FUNC_RTOC(External),0,0,0},
											{FUNC_RTOC(Alignment),0,0,0},
											{FUNC_RTOC(Program),0,0,0},
											{FUNC_RTOC(FloatingPointUnavailable),0,0,0},
											{FUNC_RTOC(Decrementer),0,0,0},
											{FUNC_RTOC(IOControllerInterfaceError),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(SystemCall),0,0,0},
											{FUNC_RTOC(Trace),0,0,0},
											{FUNC_RTOC(FloatingPointAssist),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(Trace),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0},
											{FUNC_RTOC(UnimplementedVector),0,0,0}
										};
	memcpy(VectorTable,tempVectorTable,sizeof(VectorTable));
	memcpy(handlerScratch,tempHandlerScratch,sizeof(handlerScratch));
	// Steal memory past the end of the kernel for interrupt stacks.  These will be mapped
	// with the rest of the kernel in NKInitVirtualMemory()
	for(Int32 i=0;i<NUM_INT_STACKS;i++)
	{
		(Int8*)machine.kernelEnd += INT_STACK_SIZE;
		interruptStack[i] = (Int8*)machine.kernelEnd - 64;
		*((UInt32*)interruptStack[i]) = nil;
	}
	
	// Set the current interrupt stack to be the first one
	currInterruptStack = interruptStack;
	
	// Remember, must use ROM vectors on 603 chips because we don't implement TLB-miss handling and ROM does
	UInt32	pvr = _getPVR();
	if( machine.machineClass == classPDM || ((pvr >> 16) == cpu603 || (pvr >> 16) == cpu603e || (pvr >> 16) == cpu603ev))
	{
		// Use the vectors sitting in ROM.  This is useful if we have a problem, such as the screen wanting to live at PHYSICAL ADDRESS 0.  [STUPID APPLE ENGINEERS!]
		// So far, I think this only happens on PDM machines.  It sure would be nice to know how to move the frame buffer, though...
		nkVideo << "\nInitializing 0xFFFn_nnnn interrupt vectors\n";
		usingROMVectors = true;
		NKROMInterruptVectorsInit();
	}
	else
	{
		// Create our own vectors which dispatch to functions in VectorTable.  This is *necessary* at least on the iMac
		// with iMac Update 1.1
		nkVideo << "\nInitializing 0x000n_nnnn interrupt vectors\n";
		NKOurInterruptVectorsInit();
	}
	_sync();
	canInitOtherProcessors = true;
	
	// We call NKInitInterruptVectorsOnThisProcessor() immediately, bypassing the later call to it made in NKInitThisProcessor().  This
	// is to allow immediate use of the interrupt vectors on the main processor, for things such as debugging.
	NKInitInterruptVectorsOnThisProcessor();
}

void NKInitInterruptVectorsOnThisProcessor()
{
	// Wait for interrupt vectors to be all set up
	while(!canInitOtherProcessors)
		;
	
	// Init them on this processor
	if(usingROMVectors)
		NKROMInterruptVectorsInitOnThisProcessor();
	else
		NKOurInterruptVectorsInitOnThisProcessor();
}

// When called, the machine state is as follows:
//
//	sprg0	=	pointer to processor's current Thread
//	sprg1	=	original RTOC of interrupted code
//	sprg2	=	original lr of code we interrupted
//	sprg3	=	reserved, don't touch
//	rtoc		=	a pointer to this handler's "handlerScratch" area
//	lr		=	the end of the interrupt handler.  Our vector must return to the contents of the lr
//	sp		=	the original value of the stack.
//
// ALL registers except rtoc and lr must be preserved.  However, you MUST blr to the contents of lr at the end.
PPCRegisters		savedRegs[13];
static ASCII8Str	vecName[] = {	"Unimplemented Vector","System Reset","Machine Check","Data Storage","Instruction Storage","External","Alignment","Program",
							"Floating Point Unavailable","Decrementer","System Call","Trace","Floating Point Assist","I/O Controller Interface Error"};

// Pat: you were turning on MSR[IR/DR] in SETUP_REG_SAVE... any particular reason why?  This would hose
// all sorts of stuff on 603.
#define	SETUP_REG_SAVE(vecNum)	stw		r3,4(rtoc);						\
								stw		r31,8(rtoc);						\
								mr		r31,rtoc;							\
								lwz		rtoc,0(rtoc);						\
								lwz		r3,savedRegs(rtoc);					\
								addi		r3,r3,sizeof(PPCRegisters)*vecNum;

#define	SETUP_REG_RESTORE(vecNum)	lwz		r3,savedRegs(rtoc);					\
								addi		r3,r3,sizeof(PPCRegisters)*vecNum;

#if USE_MW_INST
#define	SAVE_GENERAL_REGS		stmw		r0,(PPCRegisters.r[0])(r3);	\
								lwz			r0,4(r31);				\
								lwz			r4,8(r31);				\
								stw			r0,(PPCRegisters.r[3])(r3);	\
								stw			r4,(PPCRegisters.r[31])(r3);
#else
#define	SAVE_GENERAL_REGS		stw			r0,(PPCRegisters.r[0])(r3);	\
								stw			r1,(PPCRegisters.r[1])(r3);	\
								stw			r2,(PPCRegisters.r[2])(r3);	\
								lwz			r0,4(r31);				\
								stw			r0,(PPCRegisters.r[3])(r3);	\
								stw			r4,(PPCRegisters.r[4])(r3);	\
								stw			r5,(PPCRegisters.r[5])(r3);	\
								stw			r6,(PPCRegisters.r[6])(r3);	\
								stw			r7,(PPCRegisters.r[7])(r3);	\
								stw			r8,(PPCRegisters.r[8])(r3);	\
								stw			r9,(PPCRegisters.r[9])(r3);	\
								stw			r10,(PPCRegisters.r[10])(r3);	\
								stw			r11,(PPCRegisters.r[11])(r3);	\
								stw			r12,(PPCRegisters.r[12])(r3);	\
								stw			r13,(PPCRegisters.r[13])(r3);	\
								stw			r14,(PPCRegisters.r[14])(r3);	\
								stw			r15,(PPCRegisters.r[15])(r3);	\
								stw			r16,(PPCRegisters.r[16])(r3);	\
								stw			r17,(PPCRegisters.r[17])(r3);	\
								stw			r18,(PPCRegisters.r[18])(r3);	\
								stw			r19,(PPCRegisters.r[19])(r3);	\
								stw			r20,(PPCRegisters.r[20])(r3);	\
								stw			r21,(PPCRegisters.r[21])(r3);	\
								stw			r22,(PPCRegisters.r[22])(r3);	\
								stw			r23,(PPCRegisters.r[23])(r3);	\
								stw			r24,(PPCRegisters.r[24])(r3);	\
								stw			r25,(PPCRegisters.r[25])(r3);	\
								stw			r26,(PPCRegisters.r[26])(r3);	\
								stw			r27,(PPCRegisters.r[27])(r3);	\
								stw			r28,(PPCRegisters.r[28])(r3);	\
								stw			r29,(PPCRegisters.r[29])(r3);	\
								stw			r30,(PPCRegisters.r[30])(r3);	\
								lwz			r0,8(r31);				\
								stw			r0,(PPCRegisters.r[31])(r3);
#endif

#define	SAVE_FP_REGS				/* Must enable FPU! */					\
								mfmsr		r4;						\
								ori			r4,r4,0x2000;				\
								sync;								\
								mtmsr		r4;						\
								isync;								\
								/* Check whether this is necessary */		\
								mfsrr1(r4);							\
								andi.			r4,r4,0x2000;				\
								beq			@noSaveFPU;				\
								stfd			fp0,(PPCRegisters.fp[0])(r3);	\
								stfd			fp1,(PPCRegisters.fp[1])(r3);	\
								stfd			fp2,(PPCRegisters.fp[2])(r3);	\
								stfd			fp3,(PPCRegisters.fp[3])(r3);	\
								stfd			fp4,(PPCRegisters.fp[4])(r3);	\
								stfd			fp5,(PPCRegisters.fp[5])(r3);	\
								stfd			fp6,(PPCRegisters.fp[6])(r3);	\
								stfd			fp7,(PPCRegisters.fp[7])(r3);	\
								stfd			fp8,(PPCRegisters.fp[8])(r3);	\
								stfd			fp9,(PPCRegisters.fp[9])(r3);	\
								stfd			fp10,(PPCRegisters.fp[10])(r3);\
								stfd			fp11,(PPCRegisters.fp[11])(r3);\
								stfd			fp12,(PPCRegisters.fp[12])(r3);\
								stfd			fp13,(PPCRegisters.fp[13])(r3);\
								stfd			fp14,(PPCRegisters.fp[14])(r3);\
								stfd			fp15,(PPCRegisters.fp[15])(r3);\
								stfd			fp16,(PPCRegisters.fp[16])(r3);\
								stfd			fp17,(PPCRegisters.fp[17])(r3);\
								stfd			fp18,(PPCRegisters.fp[18])(r3);\
								stfd			fp19,(PPCRegisters.fp[19])(r3);\
								stfd			fp20,(PPCRegisters.fp[20])(r3);\
								stfd			fp21,(PPCRegisters.fp[21])(r3);\
								stfd			fp22,(PPCRegisters.fp[22])(r3);\
								stfd			fp23,(PPCRegisters.fp[23])(r3);\
								stfd			fp24,(PPCRegisters.fp[24])(r3);\
								stfd			fp25,(PPCRegisters.fp[25])(r3);\
								stfd			fp26,(PPCRegisters.fp[26])(r3);\
								stfd			fp27,(PPCRegisters.fp[27])(r3);\
								stfd			fp28,(PPCRegisters.fp[28])(r3);\
								stfd			fp29,(PPCRegisters.fp[29])(r3);\
								stfd			fp30,(PPCRegisters.fp[30])(r3);\
								stfd			fp31,(PPCRegisters.fp[31])(r3);\
							@noSaveFPU:

#define	SAVE_FUNKY_REGS			mfsprg(r0,1);							\
								stw		r0,(PPCRegisters.r[2])(r3);		\
								mfsprg(r0,2);							\
								stw		r0,(PPCRegisters.lr)(r3);			\
								mfctr	r0;							\
								stw		r0,(PPCRegisters.ctr)(r3);		\
								mfxer	r0;							\
								stw		r0,(PPCRegisters.xer)(r3);		\
								mfcr		r0;							\
								stw		r0,(PPCRegisters.cr)(r3);		\
								mfsrr0(r0);							\
								stw		r0,(PPCRegisters.srr0)(r3);		\
								mfsrr1(r0);							\
								stw		r0,(PPCRegisters.srr1)(r3);		\
								mfdar(r0);							\
								stw		r0,(PPCRegisters.dar)(r3);		\
								mfdsisr(r0);							\
								stw		r0,(PPCRegisters.dsisr)(r3);		\
								/*mfmsr	r0;							*/	\
								/*stw		r0,(PPCRegisters.msr)(r3);	*/	\
								mfsr		r4,0;						\
								mfsr		r5,1;						\
								mfsr		r6,2;						\
								mfsr		r7,3;						\
								mfsr		r8,4;						\
								mfsr		r9,5;						\
								mfsr		r10,6;						\
								mfsr		r11,7;						\
								mfsr		r12,8;						\
								mfsr		r13,9;						\
								mfsr		r14,10;						\
								mfsr		r15,11;						\
								mfsr		r16,12;						\
								mfsr		r17,13;						\
								mfsr		r18,14;						\
								mfsr		r19,15;						\
								stw		r4,PPCRegisters.sr[0](r3);		\
								stw		r5,PPCRegisters.sr[1](r3);		\
								stw		r6,PPCRegisters.sr[2](r3);		\
								stw		r7,PPCRegisters.sr[3](r3);		\
								stw		r8,PPCRegisters.sr[4](r3);		\
								stw		r9,PPCRegisters.sr[5](r3);		\
								stw		r10,PPCRegisters.sr[6](r3);		\
								stw		r11,PPCRegisters.sr[7](r3);		\
								stw		r12,PPCRegisters.sr[8](r3);		\
								stw		r13,PPCRegisters.sr[9](r3);		\
								stw		r14,PPCRegisters.sr[10](r3);		\
								stw		r15,PPCRegisters.sr[11](r3);		\
								stw		r16,PPCRegisters.sr[12](r3);		\
								stw		r17,PPCRegisters.sr[13](r3);		\
								stw		r18,PPCRegisters.sr[14](r3);		\
								stw		r19,PPCRegisters.sr[15](r3);
									
#define	RESTORE_FUNKY_REGS		lwz		r4,PPCRegisters.sr[0](r3);		\
								lwz		r5,PPCRegisters.sr[1](r3);		\
								lwz		r6,PPCRegisters.sr[2](r3);		\
								lwz		r7,PPCRegisters.sr[3](r3);		\
								lwz		r8,PPCRegisters.sr[4](r3);		\
								lwz		r9,PPCRegisters.sr[5](r3);		\
								lwz		r10,PPCRegisters.sr[6](r3);		\
								lwz		r11,PPCRegisters.sr[7](r3);		\
								lwz		r12,PPCRegisters.sr[8](r3);		\
								lwz		r13,PPCRegisters.sr[9](r3);		\
								lwz		r14,PPCRegisters.sr[10](r3);		\
								lwz		r15,PPCRegisters.sr[11](r3);		\
								lwz		r16,PPCRegisters.sr[12](r3);		\
								lwz		r17,PPCRegisters.sr[13](r3);		\
								lwz		r18,PPCRegisters.sr[14](r3);		\
								lwz		r19,PPCRegisters.sr[15](r3);		\
								sync;								\
								mtsr		0,r4;						\
								mtsr		1,r5;						\
								mtsr		2,r6;						\
								mtsr		3,r7;						\
								mtsr		4,r8;						\
								mtsr		5,r9;						\
								mtsr		6,r10;						\
								mtsr		7,r11;						\
								mtsr		8,r12;						\
								mtsr		9,r13;						\
								mtsr		10,r14;						\
								mtsr		11,r15;						\
								mtsr		12,r16;						\
								mtsr		13,r17;						\
								mtsr		14,r18;						\
								mtsr		15,r19;						\
								isync;								\
								/*lwz		r0,(PPCRegisters.msr)(r3);	*/	\
								/*mtmsr	r0;							*/	\
								lwz		r0,(PPCRegisters.srr1)(r3);	\
								mtsrr1(r0);							\
								lwz		r0,(PPCRegisters.srr0)(r3);	\
								mtsrr0(r0);							\
								lwz		r0,(PPCRegisters.cr)(r3);		\
								mtcr(r0);								\
								lwz		r0,(PPCRegisters.xer)(r3);	\
								mtxer	r0;							\
								lwz		r0,(PPCRegisters.ctr)(r3);	\
								mtctr	r0;							\
								lwz		r0,(PPCRegisters.lr)(r3);		\
								mtsprg(2,r0);							\
								lwz		r0,(PPCRegisters.r[2])(r3);	\
								mtsprg(1,r0);

#define	RESTORE_FP_REGS			/* Check whether this is necessary */		\
								mfsrr1(r4);							\
								andi.			r4,r4,0x2000;				\
								beq			@noRestoreFPU;			\
								lfd			fp0,(PPCRegisters.fp[0])(r3);	\
								lfd			fp1,(PPCRegisters.fp[1])(r3);	\
								lfd			fp2,(PPCRegisters.fp[2])(r3);	\
								lfd			fp3,(PPCRegisters.fp[3])(r3);	\
								lfd			fp4,(PPCRegisters.fp[4])(r3);	\
								lfd			fp5,(PPCRegisters.fp[5])(r3);	\
								lfd			fp6,(PPCRegisters.fp[6])(r3);	\
								lfd			fp7,(PPCRegisters.fp[7])(r3);	\
								lfd			fp8,(PPCRegisters.fp[8])(r3);	\
								lfd			fp9,(PPCRegisters.fp[9])(r3);	\
								lfd			fp10,(PPCRegisters.fp[10])(r3);\
								lfd			fp11,(PPCRegisters.fp[11])(r3);\
								lfd			fp12,(PPCRegisters.fp[12])(r3);\
								lfd			fp13,(PPCRegisters.fp[13])(r3);\
								lfd			fp14,(PPCRegisters.fp[14])(r3);\
								lfd			fp15,(PPCRegisters.fp[15])(r3);\
								lfd			fp16,(PPCRegisters.fp[16])(r3);\
								lfd			fp17,(PPCRegisters.fp[17])(r3);\
								lfd			fp18,(PPCRegisters.fp[18])(r3);\
								lfd			fp19,(PPCRegisters.fp[19])(r3);\
								lfd			fp20,(PPCRegisters.fp[20])(r3);\
								lfd			fp21,(PPCRegisters.fp[21])(r3);\
								lfd			fp22,(PPCRegisters.fp[22])(r3);\
								lfd			fp23,(PPCRegisters.fp[23])(r3);\
								lfd			fp24,(PPCRegisters.fp[24])(r3);\
								lfd			fp25,(PPCRegisters.fp[25])(r3);\
								lfd			fp26,(PPCRegisters.fp[26])(r3);\
								lfd			fp27,(PPCRegisters.fp[27])(r3);\
								lfd			fp28,(PPCRegisters.fp[28])(r3);\
								lfd			fp29,(PPCRegisters.fp[29])(r3);\
								lfd			fp30,(PPCRegisters.fp[30])(r3);\
								lfd			fp31,(PPCRegisters.fp[31])(r3);\
							@noRestoreFPU:
								
#if USE_MW_INST
#define	RESTORE_GENERAL_REGS		lmw		r4,(PPCRegisters.r[4])(r3);	\
								lwz		r1,(PPCRegisters.r[1])(r3);	\
								lwz		r0,(PPCRegisters.r[0])(r3);	\
								lwz		r3,(PPCRegisters.r[3])(r3);
#else
#define	RESTORE_GENERAL_REGS		lwz		r4,(PPCRegisters.r[4])(r3);	\
								lwz		r5,(PPCRegisters.r[5])(r3);	\
								lwz		r6,(PPCRegisters.r[6])(r3);	\
								lwz		r7,(PPCRegisters.r[7])(r3);	\
								lwz		r8,(PPCRegisters.r[8])(r3);	\
								lwz		r9,(PPCRegisters.r[9])(r3);	\
								lwz		r10,(PPCRegisters.r[10])(r3);	\
								lwz		r11,(PPCRegisters.r[11])(r3);	\
								lwz		r12,(PPCRegisters.r[12])(r3);	\
								lwz		r13,(PPCRegisters.r[13])(r3);	\
								lwz		r14,(PPCRegisters.r[14])(r3);	\
								lwz		r15,(PPCRegisters.r[15])(r3);	\
								lwz		r16,(PPCRegisters.r[16])(r3);	\
								lwz		r17,(PPCRegisters.r[17])(r3);	\
								lwz		r18,(PPCRegisters.r[18])(r3);	\
								lwz		r19,(PPCRegisters.r[19])(r3);	\
								lwz		r20,(PPCRegisters.r[20])(r3);	\
								lwz		r21,(PPCRegisters.r[21])(r3);	\
								lwz		r22,(PPCRegisters.r[22])(r3);	\
								lwz		r23,(PPCRegisters.r[23])(r3);	\
								lwz		r24,(PPCRegisters.r[24])(r3);	\
								lwz		r25,(PPCRegisters.r[25])(r3);	\
								lwz		r26,(PPCRegisters.r[26])(r3);	\
								lwz		r27,(PPCRegisters.r[27])(r3);	\
								lwz		r28,(PPCRegisters.r[28])(r3);	\
								lwz		r29,(PPCRegisters.r[29])(r3);	\
								lwz		r30,(PPCRegisters.r[30])(r3);	\
								lwz		r31,(PPCRegisters.r[31])(r3);	\
								lwz		r1,(PPCRegisters.r[1])(r3);	\
								lwz		r0,(PPCRegisters.r[0])(r3);	\
								lwz		r3,(PPCRegisters.r[3])(r3);
#endif

#define	HANDLE_INTERRUPT_CODE(vecNum)	lwz		r3,currInterruptStack(rtoc);	\
									lwz		sp,0(r3);					\
									addi		r3,r3,4;					\
									stw		r3,currInterruptStack(rtoc);	\
									mflr		r0;						\
									stw		r0,8(sp);					\
									stwu		sp,-64(sp);				\
									li		r3,vecNum;				\
									mfmsr	r4;						\
									ori		r4,r4,0x0030;				\
									sync;							\
									mtmsr	r4;						\
									isync;							\
									bl		HandleInterrupt;			\
									mfmsr	r4;						\
									rlwinm	r4,r4,0,28,25;			\
									sync;							\
									mtmsr	r4;						\
									isync;							\
									addi		sp,sp,64;					\
									lwz		r0,8(sp);					\
									mtlr		r0;						\
									lwz		r3,currInterruptStack(rtoc);	\
									subi		r3,r3,4;					\
									stw		r3,currInterruptStack(rtoc);

#define	SAVE_VOLATILE_REGS(vecNum)	SETUP_REG_SAVE(vecNum);	\
									SAVE_GENERAL_REGS;		\
									SAVE_FUNKY_REGS;			\
									SAVE_FP_REGS;

#define	RESTORE_VOLATILE_REGS(vecNum)	SETUP_REG_RESTORE(vecNum);	\
									RESTORE_FUNKY_REGS;			\
									RESTORE_FP_REGS; 		/* You were testing the wrong srr1 bit*/		\
									RESTORE_GENERAL_REGS;

__asm__ void UnimplementedVector(void)
{
	SAVE_VOLATILE_REGS(0);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(0);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(0);		// Restore volatile registers and go home
	blr;
}

__asm__ void SystemReset(void)
{
	SAVE_VOLATILE_REGS(1);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(1);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(1);		// Restore volatile registers and go home
	blr;
}

__asm__ void MachineCheck(void)
{
	SAVE_VOLATILE_REGS(2);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(2);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(2);		// Restore volatile registers and go home
	blr;
}

__asm__ void DataStorage(void)
{
	SAVE_VOLATILE_REGS(3);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(3);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(3);		// Restore volatile registers and go home
	blr;
}

__asm__ void InstructionStorage(void)
{
	SAVE_VOLATILE_REGS(4);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(4);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(4);		// Restore volatile registers and go home
	blr;
}

__asm__ void External(void)
{
	SAVE_VOLATILE_REGS(5);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(5);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(5);		// Restore volatile registers and go home
	blr;
}

static void AlignmentTable(void);
__asm__ void Alignment(void)
{
	SAVE_VOLATILE_REGS(6);
	// Handle an alignment exception.  DSISR[15-21] tells us what kind of access it was:
	// (WHAT ABOUT MISALIGNED DOUBLE ACCESSES?)
	//
	//	lwz:		0b 00 0 0000	\
	//	lwzu:	0b 00 1 0000	 |
	//	lwzx:	0b 11 0 0000	 |
	//	lwzux:	0b 11 1 0000	/
	//	stw:		0b 00 0 0010	\
	//	stwu:	0b 00 1 0010	 |
	//	stwx:	0b 11 0 0010	 |
	//	stwux:	0b 11 1 0010	/
	//	lhz:		0b 00 0 0100	\
	//	lhzu:		0b 00 1 0100	 |
	//	lhzx:		0b 11 0 0100	 |
	//	lhzux:	0b 11 1 0100	/
	//	lha:		0b 00 0 0101	\
	//	lhau:		0b 00 1 0101	 |
	//	lhax:		0b 11 0 0101	 |
	//	lhaux:	0b 11 1 0101	/
	//	sth:		0b 00 0 0110	\
	//	sthu:		0b 00 1 0110	 |
	//	sthx:	0b 11 0 0110	 |
	//	sthux:	0b 11 1 0110	/
	//
	// There are also the following ones, but we don't handle them yet:
	//
	//	stwcx.:	0b 10 0 0010
	//	eciwx:	0b 10 1 0100
	//	lwaux:	0b 01 1 0101
	//	lwax:	0b 01 0 0101
	//	ecowx:	0b 10 1 0110
	//	lmw:		0b 00 0 0111
	//	stmw:	0b 00 1 0111
	//	lfs:		0b 00 0 1000
	//	lfsu:		0b 00 1 1000
	//	lfsx:		0b 11 0 1000
	//	lfsux:	0b 11 1 1000
	//	lwbrx:	0b 10 0 1000
	//	lswx:	0b 01 0 1000
	//	lswi:		0b 01 0 1001
	//	stfs:		0b 00 0 1010
	//	stfsu:	0b 00 1 1010
	//	stfsx:	0b 11 0 1010
	//	stfsux:	0b 11 1 1010
	//	stwbrx:	0b 10 0 1010
	//	stswx:	0b 01 0 1010
	//	stswi:	0b 01 0 1011
	//	lhbrx:	0b 10 0 1100
	//	ld,lwa:	0b 00 0 1101
	//	sthbrx:	0b 10 0 1110
	//	stfiwx:	0b 11 0 1111
	//	dcbz:	0b 10 1 1111
	//	std:		0b 00 0 1111
	
	lwz		r4,AlignmentTable(rtoc);
	mfdsisr(r5);
	mfdar(r6);
	rrwinm(r0,r5,8,26,29);
	lwz		r4,0(r4);
	add		r4,r4,r0;
	mtctr	r4;
	lwz		r4,PPCRegisters.srr0(r3);
	rrwinm(r7,r5,3,25,29);
	add		r7,r7,r3;
	addi		r7,r7,PPCRegisters.r[0];
	addi		r4,r4,4;
	stw		r4,PPCRegisters.srr0(r3);
	
	// For this to come even close to working, we will need data relocation matching whatever it was in the interrupted code!
	lwz		r8,PPCRegisters.srr1(r3);
	mfmsr	r9;
	rlwinm	r8,r8,0,27,27;	// Keep the DR bit
	or		r9,r8,r9;
	sync;
	mtmsr	r9;
	sync;
	bctr;
	
entry static AlignmentTable
	// On entry, r5 holds DSISR and r6 holds DAR, r7 holds addr of source/dest operand in VolatileRegs
	b	_lwz;	// 0b 0000
	b	_error;	// 0b 0001
	b	_stw;	// 0b 0010
	b	_error;	// 0b 0011
	b	_lhz;		// 0b 0100
	b	_lha;		// 0b 0101
	b	_sth;	// 0b 0110
	b	_error;	// 0b 0111
	b	_error;	// 0b 1000
	b	_error;	// 0b 1001
	b	_error;	// 0b 1010
	b	_error;	// 0b 1011
	b	_error;	// 0b 1100
	b	_error;	// 0b 1101
	b	_error;	// 0b 1110
	b	_error;	// 0b 1111
	
_lwz:
	lbz	r8,0(r6);
	lbz	r9,1(r6);
	lbz	r10,2(r6);
	lbz	r11,3(r6);
	slwi(r8,r8,24);
	slwi(r9,r9,16);
	slwi(r10,r10,8);
	or	r8,r8,r9;
	or	r8,r8,r10;
	or	r8,r8,r11;
	stw	r8,0(r7);
	b	testForUpdate;
	
_stw:
	lwz	r8,0(r7);
	stb	r8,3(r6);
	srwi(r8,r8,8);
	stb	r8,2(r6);
	srwi(r8,r8,8);
	stb	r8,1(r6);
	srwi(r8,r8,8);
	stb	r8,0(r6);
	b	testForUpdate;

_lhz:
	lbz	r8,0(r6);
	lbz	r9,1(r6);
	slwi(r8,r8,8);
	or	r8,r8,r9;
	stw	r8,0(r7);
	b	testForUpdate;

_lha:
	lbz	r8,0(r6);
	lbz	r9,1(r6);
	slwi(r8,r8,8);
	or	r8,r8,r9;
	extsh	r8,r8;
	stw	r8,0(r7);
	b	testForUpdate;

_sth:
	lwz	r8,0(r7);
	stb	r8,1(r6);
	srwi(r8,r8,8);
	stb	r8,0(r6);
	b	testForUpdate;

testForUpdate:
	rlwinm.	r0,r5,0,17,17;
	beq		out;
	rlwinm	r5,r5,2,25,29;
	add		r5,r5,r3;
	addi		r5,r5,PPCRegisters.r[0];
	stw		r6,0(r5);
	b		out;
	
_error:
	HANDLE_INTERRUPT_CODE(6);

out:
	// Turn DR back off
	mfmsr	r9;
	rlwinm	r9,r9,0,28,26;
	sync;
	mtmsr	r9;
	sync;
	
	RESTORE_VOLATILE_REGS(6);
	blr;
}

__asm__ void Program(void)
{
	SAVE_VOLATILE_REGS(7);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(7);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(7);		// Restore volatile registers and go home
	blr;
}

static UInt64						garbageFP = 0xDEADBEEFDEADBEEF;

__asm__ void FloatingPointUnavailable(void)
{
	// Do this manually:  we simply enable the FPU and clear all the fp registers (for security, so no process sees *anything* it shouldn't see)
	stw		r3,4(rtoc);
	stw		r31,8(rtoc);
	mr		r31,rtoc;
	lwz		rtoc,0(rtoc);
	
	// Enable FPU in calling process
	mfsrr1(r3);
	ori		r3,r3,0x2000;
	mtsrr1(r3);
	
	// Enable FPU in interrupt handler
	mfmsr	r3;
	ori		r3,r3,0x2000;
	sync;
	mtmsr	r3;
	isync;
	
	// Clear all FPU registers
	lfd		fp0,garbageFP(rtoc);
	fmr		fp1,fp0;
	fmr		fp2,fp0;
	fmr		fp3,fp0;
	fmr		fp4,fp0;
	fmr		fp5,fp0;
	fmr		fp6,fp0;
	fmr		fp7,fp0;
	fmr		fp8,fp0;
	fmr		fp9,fp0;
	fmr		fp10,fp0;
	fmr		fp11,fp0;
	fmr		fp12,fp0;
	fmr		fp13,fp0;
	fmr		fp14,fp0;
	fmr		fp15,fp0;
	fmr		fp16,fp0;
	fmr		fp17,fp0;
	fmr		fp18,fp0;
	fmr		fp19,fp0;
	fmr		fp20,fp0;
	fmr		fp21,fp0;
	fmr		fp22,fp0;
	fmr		fp23,fp0;
	fmr		fp24,fp0;
	fmr		fp25,fp0;
	fmr		fp26,fp0;
	fmr		fp27,fp0;
	fmr		fp28,fp0;
	fmr		fp29,fp0;
	fmr		fp30,fp0;
	fmr		fp31,fp0;
	
	// Restore state
	mr		rtoc,r31;
	lwz		r3,4(rtoc);
	lwz		r31,8(rtoc);
	
	blr;
}

__asm__ void Decrementer(void)
{
	SAVE_VOLATILE_REGS(9);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(9);		// Handle this interrupt
	RESTORE_VOLATILE_REGS(9);		// Restore volatile registers and go home
	blr;
}

__asm__ void SystemCall(void)
{
	SAVE_VOLATILE_REGS(10);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(10);	// Handle this interrupt
	RESTORE_VOLATILE_REGS(10);		// Restore volatile registers and go home
	blr;
}

__asm__ void Trace(void)
{
	SAVE_VOLATILE_REGS(11);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(11);	// Handle this interrupt
	RESTORE_VOLATILE_REGS(11);		// Restore volatile registers and go home
	blr;
}

__asm__ void FloatingPointAssist(void)
{
	SAVE_VOLATILE_REGS(12);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(12);	// Handle this interrupt
	RESTORE_VOLATILE_REGS(12);		// Restore volatile registers and go home
	blr;
}

__asm__ void IOControllerInterfaceError(void)
{
	SAVE_VOLATILE_REGS(13);		// Save volatile registers (this is INTERRUPT processing!)
	HANDLE_INTERRUPT_CODE(13);	// Handle this interrupt
	RESTORE_VOLATILE_REGS(13);		// Restore volatile registers and go home
	blr;
}

static NKExceptionHandler exceptionHandler[14]	=	{	nil,	// 0 = unimplemented exception
												nil,	// 1 = system reset exception
												nil,	// 2 = machine check exception
												nil,	// 3 = data storage exception
												nil,	// 4 = instruction storage exception
												nil,	// 5 = external exception
												nil,	// 6 = alignment exception
												nil,	// 7 = program exception
												nil,	// 8 = fpu unavailable exception
												nil,	// 9 = decrementor exception
												nil,	// 10 = system call exception
												nil,	// 11 = trace exception
												nil,	// 12 = fpu assist exception
												nil	// 13 = io controller exception
											};

NKExceptionHandler NKInstallExceptionHandler(NKExceptionHandler proc,UInt32 handlerID)
{
	FatalAssert(handlerID <= 13);
	NKExceptionHandler	prevHandler = exceptionHandler[handlerID];
	exceptionHandler[handlerID] = proc;
	return prevHandler;
}

void HandleInterrupt(UInt32 vecNum)
{
	// vecNum is a number which tells us which interrupt we are handling.  It is NOT the number of the PPC interrupt handler, but rather the
	// number shoved into r3 in one of our handlers above...
	
	// Through some sort of miracle, we always end up here (unless someone has patched a vector) regardless of whether we are coming
	// from the 0x000n_nnnn handlers, or from the 0xFFFn_nnnn (ROM) handlers!  603 tlb stuff is handled in ROM, thank god...
	
	// If an exception handler is installed, call it.
	if(exceptionHandler[vecNum])
	{
		if((*exceptionHandler[vecNum])(&savedRegs[vecNum]))
			return;
		nkVideo << "An exception handler failed to handle an exception! (Interrupt: " << vecName[vecNum] << ")\n";
		for(;;)
			;
	}
	
	// Ignore decrementer - this would only get called if MSR[EE] is enabled before preemptive threads
	if(vecNum == decrementerException)
		return;
	
	// Otherwise, try to handle it ourselves or with the debugger nub
	if(debuggerNub)
		debuggerNub->interrupt(vecNum,&savedRegs[vecNum]);
	else
	{
		UInt32 dsisr = _getDSISR();
		UInt32 dar = _getDAR();
		UInt32 pc = _getSRR0();
		UInt32 srr1 = _getSRR1();
		UInt32 msr = (srr1 & 0x0000FFFF);
		
		if(vecNum == 11) // Trace Exception
			nkVideo << "Trace: " << pc << "\n";
		else
		{
			nkVideo << "\n------------------------------\n";
			nkVideo << "Interrupt: " << vecName[vecNum] << "\n";
			
			nkVideo << "    DSISR: " << dsisr << "\n";
			nkVideo << "    DAR:   " << dar << "\n";
			nkVideo << "    PC:    " << pc << "\n";
			nkVideo << "    MSR:   " << msr << "\n";
			nkVideo << "    SRR1:  " << srr1 << "\n";
			nkVideo << "    LR:    " << savedRegs[vecNum].lr << "\n";
			nkVideo << "------------------------------\n\n";
			
			switch(vecNum)
			{
				case 10:	// System Call - do nothing
					//NKStartTrace();
					//savedRegs[10].srr1 |= 0x00000600;
				break;
				case 3:	// Data storage, OOPS!
					if(dar < 4096)
						nkVideo << "Don't use a nil pointer you moron!\n";
				default:		// Unhandled exception, stop processor
					for(;;)
						;
				break;
			}
		}
	}
}
