/*
	NKDebugger.cp
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
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "ANSI.h"
#include "NKDebugger.h"
#include "NKVideo.h"
#include "NKMachineInit.h"
#include "Gonzales.h"
#include "Time.h"
#include "Assembly.h"
#include "NKInterruptVectors.h"
#include "Streams.h"
#include "Macros.h"
#include "External Interrupt Internals.h"
#include "NKDisassembly.h"
#include "ADB.h"
#include "NKDebuggerInternal.h"
#include "debugger_support.h"
#include "Config.h"
#include "ADBMouse.h"
#include "NKUnmangler.h"
#include "Memory Utils.h"

// Hidden ADB functions for no interrupt mode
void		ADBToggleInterrupt( Boolean noInterruptMode );
Boolean	ADBGetInterruptMode( void );

const UInt32		kDisassembleLines = 30;
const UInt32		kDisassemblyWindowLines = 5;

const UInt32		kDebuggerWindowWidth = 512;
const UInt32		kDebuggerWindowHeight = 38*6 + kDisassemblyWindowLines*6 + 12 + 10;

const UInt32		kDebuggerStackSize = 4*1024;

enum EntryCause {
	kNotInDebugger,
	kNMICause,
	kStepTraceCause,
	kMachineCheckCause,
	kDataStorageCause,
	kInstructionFetchCause,
	kTrapCause,
	kFloatingPointCause,
	kPriviledgedInstructionCause,
	kIllegalInstructionCause,
	kDebugStrCause
};

typedef UInt8	DebuggerRow[kDebuggerWindowWidth];

typedef struct NKDebugger
{
	UInt32			debugWindowX, debugWindowY;
	DebuggerRow		*saveWindow;
	NKVideo			causeBar, registerWindow, debugConsole, commandLine, disassemblyWindow;
	UInt32			saveTraceMSR, saveTraceVector[2];
	PPCRegisters*		regs;	// This contains the PPCRegisters* for the current debugger entry
	Boolean			shouldExit;
	Boolean			windowSwappedOut;
	
	void				openWindow( void );
	void				closeWindow( void );
	void				swapWindowMemory( void );
	ASCII8Str			getCommand( ASCII8Str command );
	
	void				enter( PPCRegisters* machineRegs );
	
	void				printCause( void );
	void				printRegister( UInt32 regNum );
	void				printRegisters( void );
	void				printDisassemblyWindow( UInt32 pc );
	
	void				disassembleInstructions( UInt32 pc );
	void				disassembleCentered( UInt32 pc );
	void				setupTrace( Boolean stepIn = true );
	void				desetupTrace( void );
	void				dumpStackTrace( void );
	ASCII8Str			decodeTraceBack( UInt32 pc, ASCII8Str funcName );
	void				printHelp( void );

public:
					NKDebugger();
} NKDebugger;

typedef struct TraceBackTable
{
	UInt32	zero;
	UInt32	vers;		// ???
	UInt32	whoKnows;	// ???
	UInt32	funcLen;
	UInt16	nameLen;
	ASCII8	name[];
}TraceBackTable;

#define ExceptionPatch(name,exceptionCause)	__asm__ void name(void) {mtsprg(1,r3); li r3,exceptionCause; stw r3,entryCause(rtoc); mfsprg(r3,1); b ASMEnterDebugger;}

static __asm__ void ASMEnterDebugger( void );
static __asm__ void MachineCheckException( void );
static __asm__ void TraceInterrupt( void );
static __asm__ void DataException( void );
static __asm__ void InstructionException( void );
static __asm__ void FPUUnavailableException( void );

static void IntentionalDebugPC( void );
static void EnterDebugger( register PPCRegisters* theRegs );

static NKDebugger		*debugger;
static UInt32			entryCause = kNotInDebugger;
static PPCRegisters		intentionalEntryRegs;
static Boolean			firstTimeEntry;
static Int8*			debuggerStack;
static UInt32			inDebugger = 0;
static Boolean			debuggerInited = false;
static ASCII8Str		causeStrings[] = {	"Not in debugger!",
									"Intentional debugger break",
									"Stepping",
									"Machine Check Exception",
									"Data Storage Exception (Bus Error)",
									"Instruction Fetch Exception (Bus Error)",
									"Weird trap",
									"Floating Point Exception",
									"Priviledged Instruction Violation",
									"Illegal Instruction",
									"Intentional debugger break (DebugStr)",
								};

void NKInitDebugger( void )
{
	firstTimeEntry = true;
	debugger = new NKDebugger;
	debuggerInited = true;
}

Boolean NKDebuggerInited()
{
	return debuggerInited;
}

NKDebugger::NKDebugger()
{
	saveWindow = new DebuggerRow[kDebuggerWindowHeight];
	
	// Center the window on the screen
	debugWindowX = ROUND_DOWN(16,((machine.videoParams.width-kDebuggerWindowWidth)/2));	// 16-byte align it for SwapMem_x16
	debugWindowY = (machine.videoParams.height-kDebuggerWindowHeight)/2;
	
	debuggerStack = new Int8[kDebuggerStackSize];
	debuggerStack = (Int8*)ROUND_DOWN(16,(UInt32)debuggerStack);
	debuggerStack += kDebuggerStackSize -16;	// leave a little space at the top of the stack
	
	COPY_TVECTOR( ASMEnterDebugger, VectorTable[0x07] );
	COPY_TVECTOR( MachineCheckException, VectorTable[0x02] );
	COPY_TVECTOR( DataException, VectorTable[0x03] );
	COPY_TVECTOR( InstructionException, VectorTable[0x04] );
	COPY_TVECTOR( FPUUnavailableException, VectorTable[0x08] );
	
	nkVideo << greenMsg << "Kernel Debugger Initialized\n" << whiteMsg;
}

void
NKDebugger::openWindow( void )
{
	UInt32		screenOffset = debugWindowY*machine.videoParams.rowBytes;
	
	if( firstTimeEntry )
	{
		MouseShield	mouseShield;
		
		for( Int32 i = 0; i < kDebuggerWindowHeight; i++ )
			MemCopy( ((Int8*)(((Int32)machine.videoParams.logicalAddr) + ((Int32)(i*machine.videoParams.rowBytes)) + 
				((Int32)debugWindowX) + screenOffset)), saveWindow[i], kDebuggerWindowWidth );
		
		Rect		bounds = { debugWindowX, debugWindowY, debugWindowX+kDebuggerWindowWidth,
						debugWindowY+kDebuggerWindowHeight };
		NKBlackOutRect( &bounds, 0 );
		
		bounds.x1++;	bounds.y1++;
		bounds.x2--;	bounds.y2--;
		
		Rect		causeBarBounds = bounds;
		causeBarBounds.y2 = debugWindowY+11;
		NKInitVideo( &causeBar, &causeBarBounds, false, gonzales5 );
		
		bounds.y1 = causeBarBounds.y2;
		causeBarBounds = bounds;
		causeBarBounds.x2 = 6*16 + causeBarBounds.x1 + 4;
		NKInitVideo( &registerWindow, &causeBarBounds, false, gonzales5 );
		
		bounds.x1 = causeBarBounds.x2;
		causeBarBounds = bounds;
		causeBarBounds.y2 -= 10 + ((kDisassemblyWindowLines+1)*6) + 4;
		NKInitVideo( &debugConsole, &causeBarBounds, true, gonzales5 );
		
		bounds.y1 = causeBarBounds.y2;
		causeBarBounds = bounds;
		causeBarBounds.y2 -= 10;
		NKInitVideo( &disassemblyWindow, &causeBarBounds, false, gonzales5 );
		
		bounds.y1 = causeBarBounds.y2;
		NKInitVideo( &commandLine, &bounds, false, gonzales5 );
		
		debugConsole << "Welcome to Prometheus' debugger! Last revision: 16 May 98\n";
		
		firstTimeEntry = false;
		windowSwappedOut = false;
	}
	else if(!inDebugger)
		swapWindowMemory();
}

void
NKDebugger::closeWindow( void )
{
	if(!inDebugger)
		swapWindowMemory();
}

void
NKDebugger::swapWindowMemory( void )
{
	UInt32			screenOffset = debugWindowY*machine.videoParams.rowBytes;
	MouseShield		mouseShield;
	for( Int32 i = 0; i < kDebuggerWindowHeight; i++ )
		SwapMem_x16(machine.videoParams.logicalAddr + (i*machine.videoParams.rowBytes) + debugWindowX + screenOffset,saveWindow[i],kDebuggerWindowWidth);
}

ExceptionPatch(MachineCheckException,kMachineCheckCause)
ExceptionPatch(DataException,kDataStorageCause)
ExceptionPatch(InstructionException,kInstructionFetchCause)
ExceptionPatch(FPUUnavailableException,kFloatingPointCause)

__asm__ void Debugger( void )
{
	trapNow(kNMICause);
	
entry IntentionalDebugPC;
	blr;
}

__asm__ void DebugStr(register ConstASCII8Str str)
{
	trapNow(kDebugStrCause);
	blr;
}

__asm__ void ASMEnterDebugger( void )
{
	// Save all General Purpose Registers
	mtsprg(1,r3);
	lwz			r3,intentionalEntryRegs(rtoc);
#if USE_MW_INST
	stmw		r0,(PPCRegisters.r[0])(r3);	// Store them all
#else
	stw			r0,(PPCRegisters.r[0])(r3);
	stw			r1,(PPCRegisters.r[1])(r3);
	stw			r2,(PPCRegisters.r[2])(r3);
	stw			r3,(PPCRegisters.r[3])(r3);
	stw			r4,(PPCRegisters.r[4])(r3);
	stw			r5,(PPCRegisters.r[5])(r3);
	stw			r6,(PPCRegisters.r[6])(r3);
	stw			r7,(PPCRegisters.r[7])(r3);
	stw			r8,(PPCRegisters.r[8])(r3);
	stw			r9,(PPCRegisters.r[9])(r3);
	stw			r10,(PPCRegisters.r[10])(r3);
	stw			r11,(PPCRegisters.r[11])(r3);
	stw			r12,(PPCRegisters.r[12])(r3);
	stw			r13,(PPCRegisters.r[13])(r3);
	stw			r14,(PPCRegisters.r[14])(r3);
	stw			r15,(PPCRegisters.r[15])(r3);
	stw			r16,(PPCRegisters.r[16])(r3);
	stw			r17,(PPCRegisters.r[17])(r3);
	stw			r18,(PPCRegisters.r[18])(r3);
	stw			r19,(PPCRegisters.r[19])(r3);
	stw			r20,(PPCRegisters.r[20])(r3);
	stw			r21,(PPCRegisters.r[21])(r3);
	stw			r22,(PPCRegisters.r[22])(r3);
	stw			r23,(PPCRegisters.r[23])(r3);
	stw			r24,(PPCRegisters.r[24])(r3);
	stw			r25,(PPCRegisters.r[25])(r3);
	stw			r26,(PPCRegisters.r[26])(r3);
	stw			r27,(PPCRegisters.r[27])(r3);
	stw			r28,(PPCRegisters.r[28])(r3);
	stw			r29,(PPCRegisters.r[29])(r3);
	stw			r30,(PPCRegisters.r[30])(r3);
	stw			r31,(PPCRegisters.r[31])(r3);
#endif
	mfsprg(r4,0);
	stw			r4,(PPCRegisters.r[2])(r3);
	mfsprg(r4,1);
	stw			r4,(PPCRegisters.r[3])(r3);
	
	// Save all segment registers
	mfsr		r16,0;
	mfsr		r17,1;
	mfsr		r18,2;
	mfsr		r19,3;
	mfsr		r20,4;
	mfsr		r21,5;
	mfsr		r22,6;
	mfsr		r23,7;
	mfsr		r24,8;
	mfsr		r25,9;
	mfsr		r26,10;
	mfsr		r27,11;
	mfsr		r28,12;
	mfsr		r29,13;
	mfsr		r30,14;
	mfsr		r31,15;
#if USE_MW_INST
	stmw	r16,(PPCRegisters.sr[0])(r3);
#else
	stw			r16,(PPCRegisters.sr[0])(r3);
	stw			r17,(PPCRegisters.sr[1])(r3);
	stw			r18,(PPCRegisters.sr[2])(r3);
	stw			r19,(PPCRegisters.sr[3])(r3);
	stw			r20,(PPCRegisters.sr[4])(r3);
	stw			r21,(PPCRegisters.sr[5])(r3);
	stw			r22,(PPCRegisters.sr[6])(r3);
	stw			r23,(PPCRegisters.sr[7])(r3);
	stw			r24,(PPCRegisters.sr[8])(r3);
	stw			r25,(PPCRegisters.sr[9])(r3);
	stw			r26,(PPCRegisters.sr[10])(r3);
	stw			r27,(PPCRegisters.sr[11])(r3);
	stw			r28,(PPCRegisters.sr[12])(r3);
	stw			r29,(PPCRegisters.sr[13])(r3);
	stw			r30,(PPCRegisters.sr[14])(r3);
	stw			r31,(PPCRegisters.sr[15])(r3);
#endif
	// Save all useful Special Purpose Registers
	mfsprg(r4,2);
	stw			r4,(PPCRegisters.lr)(r3);
	mfctr		r4;
	stw			r4,(PPCRegisters.ctr)(r3);
	mfxer		r4;
	stw			r4,(PPCRegisters.xer)(r3);
	mfcr			r4;
	stw			r4,(PPCRegisters.cr)(r3);
	mfsrr0(r4);
	stw			r4,(PPCRegisters.srr0)(r3);
	mfsrr1(r4);
	stw			r4,(PPCRegisters.srr1)(r3);
	
	// Save all Floating-Point Registers
	/*stfd			fp0,(PPCRegisters.fpr[0])(r3);
	stfd			fp1,(PPCRegisters.fpr[1])(r3);
	stfd			fp2,(PPCRegisters.fpr[2])(r3);
	stfd			fp3,(PPCRegisters.fpr[3])(r3);
	stfd			fp4,(PPCRegisters.fpr[4])(r3);
	stfd			fp5,(PPCRegisters.fpr[5])(r3);
	stfd			fp6,(PPCRegisters.fpr[6])(r3);
	stfd			fp7,(PPCRegisters.fpr[7])(r3);
	stfd			fp8,(PPCRegisters.fpr[8])(r3);
	stfd			fp9,(PPCRegisters.fpr[9])(r3);
	stfd			fp10,(PPCRegisters.fpr[10])(r3);
	stfd			fp11,(PPCRegisters.fpr[11])(r3);
	stfd			fp12,(PPCRegisters.fpr[12])(r3);
	stfd			fp13,(PPCRegisters.fpr[13])(r3);
	stfd			fp14,(PPCRegisters.fpr[14])(r3);
	stfd			fp15,(PPCRegisters.fpr[15])(r3);
	stfd			fp16,(PPCRegisters.fpr[16])(r3);
	stfd			fp17,(PPCRegisters.fpr[17])(r3);
	stfd			fp18,(PPCRegisters.fpr[18])(r3);
	stfd			fp19,(PPCRegisters.fpr[19])(r3);
	stfd			fp20,(PPCRegisters.fpr[20])(r3);
	stfd			fp21,(PPCRegisters.fpr[21])(r3);
	stfd			fp22,(PPCRegisters.fpr[22])(r3);
	stfd			fp23,(PPCRegisters.fpr[23])(r3);
	stfd			fp24,(PPCRegisters.fpr[24])(r3);
	stfd			fp25,(PPCRegisters.fpr[25])(r3);
	stfd			fp26,(PPCRegisters.fpr[26])(r3);
	stfd			fp27,(PPCRegisters.fpr[27])(r3);
	stfd			fp28,(PPCRegisters.fpr[28])(r3);
	stfd			fp29,(PPCRegisters.fpr[29])(r3);
	stfd			fp30,(PPCRegisters.fpr[30])(r3);
	stfd			fp31,(PPCRegisters.fpr[31])(r3);*/
	
	// Enable Data and Instruction Relocation
	mfmsr		r4;
	ori			r4,r4,0x0030;
	sync;
	mtmsr		r4;
	
	// Figure out why we are here
	lwz			r4,entryCause(rtoc);
	cmpwi		r4,kNotInDebugger;
	bne			@alreadyKnow;
	// So we got here through the Program exception
	//mfsrr1(r0);
	lwz			r0,PPCRegisters.srr1(r3);
	li			r4,kFloatingPointCause;
	rlwinm.		r0,r0,0,11,11;
	bne			@alreadyKnow;
	li			r4,kPriviledgedInstructionCause;
	rlwinm.		r0,r0,0,13,13;
	bne			@alreadyKnow;
	li			r4,kIllegalInstructionCause;
	rlwinm		r0,r0,0,12,12;
	bne			@alreadyKnow;
	//mfsrr0(r4);
	lwz			r4,PPCRegisters.srr0(r3);
	addi			r4,r4,4;				// Increment the PC over the trap instruction
	stw			r4,(PPCRegisters.srr0)(r3);
	lwz			r4,-4(r4);			// Decode the flag from the IMM field of the trap instruction
	rlwinm		r4,r4,0,16,31;
	
@alreadyKnow:
	// Remember why we are here
	stw			r4,entryCause(rtoc);
	
	// Switch to Debugger Stack
	lwz			sp,debuggerStack(rtoc);
	
	// Build Stack Frame
	mflr			r0;
	stw			r0,8(sp);
	stwu			sp,-64(sp);
	
	// Enter Debugger (r3 contains the PPCRegisters*)
	bl			EnterDebugger;
	
	// Destroy Stack Frame
	addi			sp,sp,64;
	lwz			r0,8(sp);
	mtlr			r0;
	
	// Not in the debugger anymore!
	li			r3,kNotInDebugger;
	stw			r3,entryCause(rtoc);
	
	// Disable Data and Instruction Relocation
	mfmsr		r3;
	rlwinm		r3,r3,0,28,25;
	sync;
	mtmsr		r3;
	sync;
	
	// Restore useful Special Purpose Registers
	lwz			r3,intentionalEntryRegs(rtoc);
	lwz			r4,(PPCRegisters.srr1)(r3);
	mtsrr1(r4);
	lwz			r4,(PPCRegisters.srr0)(r3);
	mtsrr0(r4);
	lwz			r4,(PPCRegisters.cr)(r3);
	mtcr(r4);
	lwz			r4,(PPCRegisters.xer)(r3);
	mtxer		r4;
	lwz			r4,(PPCRegisters.ctr)(r3);
	mtctr		r4;
	lwz			r4,(PPCRegisters.lr)(r3);
	mtsprg(2,r4);
	
	// Restore Floating Point Registers
	/*lfd			fp0,(PPCRegisters.fpr[0])(r3);
	lfd			fp1,(PPCRegisters.fpr[1])(r3);
	lfd			fp2,(PPCRegisters.fpr[2])(r3);
	lfd			fp3,(PPCRegisters.fpr[3])(r3);
	lfd			fp4,(PPCRegisters.fpr[4])(r3);
	lfd			fp5,(PPCRegisters.fpr[5])(r3);
	lfd			fp6,(PPCRegisters.fpr[6])(r3);
	lfd			fp7,(PPCRegisters.fpr[7])(r3);
	lfd			fp8,(PPCRegisters.fpr[8])(r3);
	lfd			fp9,(PPCRegisters.fpr[9])(r3);
	lfd			fp10,(PPCRegisters.fpr[10])(r3);
	lfd			fp11,(PPCRegisters.fpr[11])(r3);
	lfd			fp12,(PPCRegisters.fpr[12])(r3);
	lfd			fp13,(PPCRegisters.fpr[13])(r3);
	lfd			fp14,(PPCRegisters.fpr[14])(r3);
	lfd			fp15,(PPCRegisters.fpr[15])(r3);
	lfd			fp16,(PPCRegisters.fpr[16])(r3);
	lfd			fp17,(PPCRegisters.fpr[17])(r3);
	lfd			fp18,(PPCRegisters.fpr[18])(r3);
	lfd			fp19,(PPCRegisters.fpr[19])(r3);
	lfd			fp20,(PPCRegisters.fpr[20])(r3);
	lfd			fp21,(PPCRegisters.fpr[21])(r3);
	lfd			fp22,(PPCRegisters.fpr[22])(r3);
	lfd			fp23,(PPCRegisters.fpr[23])(r3);
	lfd			fp24,(PPCRegisters.fpr[24])(r3);
	lfd			fp25,(PPCRegisters.fpr[25])(r3);
	lfd			fp26,(PPCRegisters.fpr[26])(r3);
	lfd			fp27,(PPCRegisters.fpr[27])(r3);
	lfd			fp28,(PPCRegisters.fpr[28])(r3);
	lfd			fp29,(PPCRegisters.fpr[29])(r3);
	lfd			fp30,(PPCRegisters.fpr[30])(r3);
	lfd			fp31,(PPCRegisters.fpr[31])(r3);*/
	
	// Restore segment registers
#if USE_MW_INST
	lmw		r16,(PPCRegisters.sr[0])(r3);
#else
	lwz		r16,PPCRegisters.sr[0](r3);
	lwz		r17,PPCRegisters.sr[1](r3);
	lwz		r18,PPCRegisters.sr[2](r3);
	lwz		r19,PPCRegisters.sr[3](r3);
	lwz		r20,PPCRegisters.sr[4](r3);
	lwz		r21,PPCRegisters.sr[5](r3);
	lwz		r22,PPCRegisters.sr[6](r3);
	lwz		r23,PPCRegisters.sr[7](r3);
	lwz		r24,PPCRegisters.sr[8](r3);
	lwz		r25,PPCRegisters.sr[9](r3);
	lwz		r26,PPCRegisters.sr[10](r3);
	lwz		r27,PPCRegisters.sr[11](r3);
	lwz		r28,PPCRegisters.sr[12](r3);
	lwz		r29,PPCRegisters.sr[13](r3);
	lwz		r30,PPCRegisters.sr[14](r3);
	lwz		r31,PPCRegisters.sr[15](r3);
#endif
	isync;
	mtsr		0,r16;
	mtsr		1,r17;
	mtsr		2,r18;
	mtsr		3,r19;
	mtsr		4,r20;
	mtsr		5,r21;
	mtsr		6,r22;
	mtsr		7,r23;
	mtsr		8,r24;
	mtsr		9,r25;
	mtsr		10,r26;
	mtsr		11,r27;
	mtsr		12,r28;
	mtsr		13,r29;
	mtsr		14,r30;
	mtsr		15,r31;
	isync;
	
	// Restore General Purpose Registers
	lwz			r4,(PPCRegisters.r[2])(r3);
	mtsprg(0,r4);
#if USE_MW_INST
	lmw			r4,(PPCRegisters.r[4])(r3);
#else
	lwz		r4,(PPCRegisters.r[4])(r3);
	lwz		r5,(PPCRegisters.r[5])(r3);
	lwz		r6,(PPCRegisters.r[6])(r3);
	lwz		r7,(PPCRegisters.r[7])(r3);
	lwz		r8,(PPCRegisters.r[8])(r3);
	lwz		r9,(PPCRegisters.r[9])(r3);
	lwz		r10,(PPCRegisters.r[10])(r3);
	lwz		r11,(PPCRegisters.r[11])(r3);
	lwz		r12,(PPCRegisters.r[12])(r3);
	lwz		r13,(PPCRegisters.r[13])(r3);
	lwz		r14,(PPCRegisters.r[14])(r3);
	lwz		r15,(PPCRegisters.r[15])(r3);
	lwz		r16,(PPCRegisters.r[16])(r3);
	lwz		r17,(PPCRegisters.r[17])(r3);
	lwz		r18,(PPCRegisters.r[18])(r3);
	lwz		r19,(PPCRegisters.r[19])(r3);
	lwz		r20,(PPCRegisters.r[20])(r3);
	lwz		r21,(PPCRegisters.r[21])(r3);
	lwz		r22,(PPCRegisters.r[22])(r3);
	lwz		r23,(PPCRegisters.r[23])(r3);
	lwz		r24,(PPCRegisters.r[24])(r3);
	lwz		r25,(PPCRegisters.r[25])(r3);
	lwz		r26,(PPCRegisters.r[26])(r3);
	lwz		r27,(PPCRegisters.r[27])(r3);
	lwz		r28,(PPCRegisters.r[28])(r3);
	lwz		r29,(PPCRegisters.r[29])(r3);
	lwz		r30,(PPCRegisters.r[30])(r3);
	lwz		r31,(PPCRegisters.r[31])(r3);
#endif
	lwz			r1,(PPCRegisters.r[1])(r3);
	lwz			r0,(PPCRegisters.r[0])(r3);
	lwz			r3,(PPCRegisters.r[3])(r3);
	
	// Exit Interrupt
	blr;
}

void EnterDebugger( register PPCRegisters* theRegs )
{
	debugger->enter(theRegs);
}

void PassException( register PPCRegisters* theRegs )
{
	entryCause = kNMICause;
	debugger->enter(theRegs);
	entryCause = kNotInDebugger;
}

void PrintDebuggerMessage(ConstASCII8Str str)
{
	debugger->debugConsole << str;
}

void
NKDebugger::enter( register PPCRegisters* theRegs )
{
	regs = theRegs;
	openWindow();
	inDebugger++;
	
	Boolean			saveNoInterruptMode = ADBGetInterruptMode();
	
	ADBToggleInterrupt( true );
	
	debugConsole << "Error at pc: " << regs->srr0 << "\n";
	
	switch(entryCause)
	{
		case kNMICause:
			if( regs->srr0 == ((Int32*)IntentionalDebugPC)[0] )
				regs->srr0 = regs->lr;
		break;
		case kStepTraceCause:
			desetupTrace();
		break;
		case kMachineCheckCause:
			debugConsole << "\nMachine Check Exception at " << _getSRR0() << "\n";
			if(!(_getSRR1() & 0x00000002))
				debugConsole << "Not ";
			debugConsole << "Recoverable\n";
		break;
		case kDataStorageCause:
			debugConsole << "\nBus Error while trying to access data at " << _getDAR()
				<< ((_getDAR() < 4096) ? ("\n(Don't use a nil pointer, you moron!)") : ("")) << "\n";
			debugConsole << "Useful info: ";
			
			UInt32		regDSISR = _getDSISR();
			if( regDSISR & 0x80000000 )
				debugConsole << "Exception caused by a direct store\n";
			if( regDSISR & 0x40000000 )
				debugConsole << "Exception is a page fault\n";
			if( regDSISR & 0x08000000 )
				debugConsole << "Page priviledge violation\n";
			if( regDSISR & 0x02000000 )
				debugConsole << "Exception was caused by a store\n";
			else
				debugConsole << "Exception was caused by a load\n";
			if( regDSISR & 0x00400000 )
				debugConsole << "Exception was caused by a DABR match\n";
		break;
		case kInstructionFetchCause:
			debugConsole << "\nBus Error while trying to fetch code at " << _getSRR0() << "\n";
			debugConsole << "Useful info:";
			
			UInt32 srr1 = _getSRR1();
			if(srr1 & 0x40000000)
				debugConsole << "Exception is a page fault\n";
			if(srr1 & 0x10000000)
				debugConsole << "Fetch is from no-execute/direct store/guarded memory\n";
			if(srr1 & 0x08000000)
				debugConsole << "Page priviledge violation\n";
		break;
		case kTrapCause:
			debugConsole << "\nUnknown trap!!  This should never happen\n";
		break;
		case kFloatingPointCause:
			debugConsole << "\nFloating point exception\n";
		break;
		case kPriviledgedInstructionCause:
			debugConsole << "\nPriviledged instruction violation\n";
		break;
		case kIllegalInstructionCause:
			debugConsole << "\nIllegal instruction\n";
		break;
		case kDebugStrCause:
			debugConsole << "\n" << (ASCII8Str)regs->r[3] << "\n";
		break;
	}
	
	printCause();
	printRegisters();
	printDisassemblyWindow( regs->srr0 );
	
	ASCII8				command[100];
	
	shouldExit = false;
	while( !shouldExit )
	{
		if( !parse_line( getCommand( command ) ) )
			debugConsole << "Syntax error\n";
	}
	
end:
	ADBToggleInterrupt( saveNoInterruptMode );
	
	inDebugger--;
	closeWindow();
}

void
NKDebugger::disassembleCentered( UInt32 pc )
{
	UInt32			curPC = pc - (kDisassembleLines<<1);
	
	debugConsole << "Disassembling from " << curPC << "\n";
	for( Int32 i = 0; i < kDisassembleLines; i++ )
	{
		ASCII8				disassembly[51];
		
		if( curPC == pc )
			debugConsole << "    *";
		else
			debugConsole << "     ";
		
		debugConsole << Disassemble( *((Int32*)curPC), disassembly ) << "\n";
		
		curPC += 4;
	}
}

void
NKDebugger::disassembleInstructions( UInt32 pc )
{
	UInt32			curPC = pc;
	
	debugConsole << "Disassembling from " << curPC << "\n";
	for( Int32 i = 0; i < kDisassembleLines; i++ )
	{
		ASCII8				disassembly[51];
		
		if( curPC == pc )
			debugConsole << "    *";
		else
			debugConsole << "     ";
		
		debugConsole << Disassemble( *((Int32*)curPC), disassembly ) << "\n";
		
		curPC += 4;
	}
}

static __asm__ void TraceInterrupt( void )
{
	mtsprg(1,r3);
	li		r3, kStepTraceCause;
	stw		r3, entryCause(rtoc);
	mfsprg(r3,1);
	b		ASMEnterDebugger;
}

void
NKDebugger::setupTrace( Boolean stepIn )
{
	saveTraceMSR = regs->srr1;
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		// For 601 machines, the Trace exception handler is actually at 0x00002000
		COPY_TVECTOR(VectorTable[0x20],saveTraceVector);
		COPY_TVECTOR(TraceInterrupt,VectorTable[0x20]);
	}
	else
	{
		COPY_TVECTOR(VectorTable[0x0D],saveTraceVector);
		COPY_TVECTOR(TraceInterrupt,VectorTable[0x0D]);
	}
	
	regs->srr1 |= 0x00000400;
	if( stepIn )
		regs->srr1 |= 0x00000200;
}

void
NKDebugger::desetupTrace( void )
{
	regs->srr1 = saveTraceMSR;
	
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
		// For 601 machines, the Trace exception handler is actually at 0x00002000
		COPY_TVECTOR(saveTraceVector,VectorTable[0x20]);
	else
		COPY_TVECTOR(saveTraceVector,VectorTable[0x0D]);
}

void
NKDebugger::dumpStackTrace( void )
{
	ASCII8		funcName[100], temp[100];
	ASCII8		unmangled[256];
	UInt8*		sp;
	UInt32		lr;
	
	debugConsole << "Stack Trace:\n";
	debugConsole << regs->srr0 << "   " << Unmangle(decodeTraceBack( regs->srr0, funcName ),unmangled) << "\n";
	decodeTraceBack( regs->lr, temp );
	if( strcmp( funcName, temp ) != 0 )
		debugConsole << regs->lr << "   " << Unmangle(temp,unmangled) << "\n";
	sp = (UInt8*)regs->r[1];
	sp = (UInt8*)(*((Int32*)sp));	// Must pop a frame first, since the bottom one holds junk in the lr save area
	while( sp )
	{
		lr = ((UInt32*)sp)[2];
		if(lr)
			debugConsole << lr << "   " << Unmangle(decodeTraceBack( lr, funcName ),unmangled) << "\n";
		else
			break;
		sp = (UInt8*)(*((Int32*)sp));
	}
}

ASCII8Str
NKDebugger::decodeTraceBack( UInt32 pc, ASCII8Str funcName )
{
	UInt16		nameLen = 7;
	ASCII8Str		name = "Unnamed";
	UInt32*		tbStart = (UInt32*)pc;
	
	do
	{
		tbStart++;
	}while(*tbStart && tbStart[-1] != 0x4E800020);
	
	if(!*tbStart)
	{
		TraceBackTable*	table = (TraceBackTable*)tbStart;
		if(table->vers == 0x00092041)
		{
			if((UInt32)table - table->funcLen <= pc)
			{
				name = table->name;
				nameLen = table->nameLen;
			}
		}
		
		if(nameLen > 99)
			nameLen = 99;
	}
	
	MemCopy(name,funcName,nameLen);
	funcName[nameLen] = '\0';
	
	return funcName;
}

void
NKDebugger::printHelp( void )
{
	debugConsole << "Prometheus' Debugger help:\n";
	debugConsole << "g    --- go (continue at current pc)\n";
	debugConsole << "il   --- disassemble 30 lines from current pc\n";
	debugConsole << "ip   --- disassemble 30 lines centered around current pc\n";
	debugConsole << "rs   --- restart\n";
	debugConsole << "s    --- step to next instruction (step in)\n";
	debugConsole << "st   --- do a stack trace\n";
	debugConsole << "t    --- step to next instruction (step out, *doesn't work*)\n";
	debugConsole << "tl   --- list all threads\n";
	debugConsole << "help --- print this help info\n\n";
}

void
NKDebugger::printRegister( UInt32 regNum )
{
	ASCII8		str[2] = { 0, 0 };
	
	switch( regNum )
	{
		case 1:
			registerWindow << "  sp";
			break;
		case 2:
			registerWindow << "rtoc";
			break;
		default:
			if( regNum < 10 )
				registerWindow << " ";
			registerWindow << " r";
			
			if( regNum >= 10 )
			{
				str[0] = (regNum/10) + '0';
				registerWindow << str;
			}
			
			str[0] = (regNum%10) + '0';
			registerWindow << str;
			break;
	}
	
	registerWindow << "=" << regs->r[regNum];
}

void
NKDebugger::printCause( void )
{
	causeBar.cursX = causeBar.bounds.x1;
	causeBar.cursY = causeBar.bounds.y1;
	
	NKBlackOutRect( &causeBar.bounds );
	
	causeBar << redMsg << "             Cause: " << whiteMsg;
	if(entryCause < (sizeof(causeStrings)/4))
		causeBar << causeStrings[entryCause];
	else
		causeBar << "Unknown";
}

void
NKDebugger::printRegisters( void )
{
	registerWindow.cursX = registerWindow.bounds.x1;
	registerWindow.cursY = registerWindow.bounds.y1;

	registerWindow << redMsg << "   Registers\n_______________\n" << whiteMsg;
	for( Int32 i = 0; i < 32; i++ )
	{
		registerWindow << "\n";
		printRegister( i );
	}
	registerWindow << "\n\n  pc=" << regs->srr0;
	registerWindow << "\n  lr=" << regs->lr;
	registerWindow << "\n msr=" << regs->srr1;
}

void
NKDebugger::printDisassemblyWindow( UInt32 pc )
{
	UInt32			curPC = pc - (kDisassemblyWindowLines>>1)*4;
	
	disassemblyWindow.cursX = disassemblyWindow.bounds.x1;
	disassemblyWindow.cursY = disassemblyWindow.bounds.y1;
	
	NKBlackOutRect( &disassemblyWindow.bounds );
	
	ASCII8				disassembly[51];
	
	disassemblyWindow << redMsg << "Disassembly: " << whiteMsg;
	disassemblyWindow << "Disassembling from " << curPC;
	for( Int32 i = 0; i < kDisassemblyWindowLines; i++ )
	{
		if( curPC == pc )
			disassemblyWindow << "\n    *";
		else
			disassemblyWindow << "\n     ";
		
		disassemblyWindow << Disassemble( *((Int32*)curPC), disassembly );
		
		curPC += 4;
	}
}

ASCII8Str
NKDebugger::getCommand( ASCII8Str command )
{
	UInt32		cursX, cursY;
	UInt32		lineLen = 0;
	
	MemZero( (void*)command, 100 );
	
	commandLine << "\n";
	
	for( ;; )
	{
		ASCII8 str[2] = {0x7F,0};
		cursX = commandLine.cursX;
		cursY = commandLine.cursY;
		if(!windowSwappedOut)
			commandLine << str;
		cin >> str[0];
		commandLine.cursX = cursX;
		commandLine.cursY = cursY;
		if( str[0] == 0x1B || windowSwappedOut )	// Escape - swap window.  If window swapped out, swap it in
		{
			swapWindowMemory();
			windowSwappedOut = !windowSwappedOut;
		}
		if( str[0] == '\r' )		// Keyboard returns \r, not \n
		{
			commandLine << " \n";		// Clear block at end of line
			return command;
		}
		else if( str[0] == 0x08 )	// Delete (note, this doesn't work across lines)
		{
			if(lineLen)
			{
				cursX -= 6;
				commandLine.cursX = cursX;
				commandLine.cursY = cursY;
				commandLine << "  ";		// Clear last character and block
				commandLine.cursX = cursX;
				commandLine.cursY = cursY;
				command[lineLen--] = '\0';
			}
		}
		else if( str[0] != 0x1B )
		{
			if(lineLen < 100)
			{
				command[lineLen++] = str[0];
				command[lineLen] = '\0';
				commandLine << str;
			}
		}
	}
}

UInt32 getRegValue( UInt32 regNum )
{
	if( regNum < 32 )
		return debugger->regs->r[regNum];
	else if( regNum & 0x80000000 )
		return debugger->regs->sr[regNum & 0x0000000F];
	else
	{
		switch( regNum )
		{
			case REG_PC:
				return debugger->regs->srr0;
				break;
			case REG_LR:
				return debugger->regs->lr;
				break;
		}
	}
	
	return 0xDEADBEEF;
}

void setRegValue( UInt32 regNum, UInt32 newValue )
{
	if( regNum < 32 )
		debugger->regs->r[regNum] = newValue;
	else if( regNum & 0x80000000)
		debugger->regs->sr[regNum & 0x0000000F] = newValue;
	else
	{
		switch( regNum )
		{
			case REG_PC:
				debugger->regs->srr0 = newValue;
				// Update disassembly display
				debugger->printDisassemblyWindow( newValue );
				break;
			case REG_LR:
				debugger->regs->lr = newValue;
				break;
		}
	}
	
	// Update display
	debugger->printRegisters();
}

void restart( void )
{
	Restart();
}

void goFrom( UInt32 start )
{
	debugger->regs->srr0 = start;
	debugger->shouldExit = true;
}

void print( ASCII8Str str )
{
	debugger->debugConsole << str;
}

void printNum( UInt32 num )
{
	debugger->debugConsole << num;
}

void disassembleAt( UInt32 addr, Boolean centered )
{
	if( centered )
		debugger->disassembleCentered( addr );
	else
		debugger->disassembleInstructions( addr );
}

void printHelp( void )
{
	debugger->printHelp();
}

void stackTrace( void )
{
	debugger->dumpStackTrace();
}

void step( Boolean over )
{
	debugger->setupTrace( !over );
	debugger->shouldExit = true;
}

static ASCII8 hexToCharTab[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static void numToHexStr(UInt32 n,ASCII8Str str);

void displayMemory( UInt32 addr )
{
	ASCII8	numStr[9];
	addr = ROUND_DOWN(16,addr);
	debugger->debugConsole << "Displaying memory from " << addr << "\n";
	
	numToHexStr(addr,numStr);
	debugger->debugConsole << "  " << numStr << "  ";
	
	UInt16*	mem = (UInt16*)addr;
	numStr[4] = '\0';
	for(Int32 i=0;i<8;i++)
	{
		for(Int32 j=0;j<4;j++)
			numStr[j] = hexToCharTab[(*mem >> (12 - 4*j)) & 0x0000000F];
		debugger->debugConsole << numStr << " ";
		mem++;
	}
	debugger->debugConsole << " ";
	
	ASCII8*	theChar = (ASCII8*) addr;
	for(Int32 i=0;i<16;i++)
	{
		ASCII8	theStr[2] = {*theChar++,'\0'};
		debugger->debugConsole << theStr;
	}
	
	debugger->debugConsole << "\n";
}

static void numToHexStr(UInt32 n,ASCII8Str str)
{
	for(Int32 i=0;i<8;i++)
		str[i] = hexToCharTab[(n >> (28 - 4*i)) & 0x0000000F];
	str[8] = '\0';
}

void _Assert(ConstASCII8Str file,ConstASCII8Str cond,UInt32 line,Boolean fatal)
{
	ASCII8	string[1024] = {0};
	ASCII8	number[11] = {0};
	UInt32	numberLen = 0;
	
	if(fatal)
		strcat(string,"Fatal ");
	strcat(string,"Kernel Assert in ");
	strcat(string,file);
	strcat(string," at line ");
	
	while(line)
	{
		number[numberLen++] = ('0' + (line % 10));
		number[numberLen] = '\0';
		line /= 10;
	}
	if(numberLen)
	{
		for(Int32 i=0;i<numberLen/2;i++)
		{
			ASCII8 temp = number[i];
			number[i] = number[numberLen-i-1];
			number[numberLen-i-1] = temp;
		}
		strcat(string,number);
	}
	else
		strcat(string,"???");
	strcat(string,": ");
	strcat(string,cond);
	strcat(string,"\n");
	if(NKDebuggerInited())
		DebugStr(string);
	else
		nkVideo << redMsg << string << whiteMsg;
	if(fatal)
	{
		for(;;)
			;
	}
}
