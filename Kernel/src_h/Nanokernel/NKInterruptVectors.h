/*
	NKInterruptVectors.h
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
	Patrick Varilly		-	Tuesday, 11 Jan 00	-	Removed saving of msr of interrupt handling code, the actual interrupt handlers should restore
											the MSR themselves if they change it
*/
#ifndef __NK_INTERRUPT_VECTORS__
#define __NK_INTERRUPT_VECTORS__

extern UInt32 VectorTable[48][2];	// Vector table is a table of PowerPC transition vectors.  When one of the 48 PPC native interrupt handlers gets executed,
							// it immediately reads the appropriate PPC transition vector from this table and then branches to it after having enabled
							// data/instruction relocation.  This allows one to easily replace the PPC interrupt handlers, and is modeled somewhat
							// after what MacOS does.

// This structure is used by NKDebugger.cp and NKInterruptVectors.cp
typedef struct PPCRegisters
{
	Float64		fp[32];	// Floating point registers 0-12 should be saved in an interrupt.
	UInt32		r[32];	// General purpose registers 0-12 should be saved in an interrupt.
	
	// The following SPR registers should be saved in an interrupt
	UInt32		lr;
	UInt32		ctr;
	UInt32		xer;
	UInt32		fpscr;
	UInt32		cr;
	
	// The following SPR registers are used by the interrupt handler
	UInt32		srr0;	// pc of interrupted code
	UInt32		srr1;	// msr of interrupted code
	UInt32		dar;		// dar of interrupt handling code
	UInt32		dsisr;	// dsisr of interrupt handling code
	
	// The following SR registers should be saved by the interrupt handler
	UInt32		sr[16];
}PPCRegisters;

// NKExceptionHandler type.  This is a pointer to a C function.  The registers that were interrupt are pointed to by theUserRegs.
// Return true if the processor should continue execution, false if the processor should halt due to unrecoverable error.
typedef Boolean (*NKExceptionHandler)(PPCRegisters* theUserRegs);

void	NKInitInterruptVectors(void);
void	NKInitInterruptVectorsOnThisProcessor();	// Call this ONCE per processor to initialize interrupt vectors on that processor

// Allows you to install a handler for an exception.  Returns the previous handler.
NKExceptionHandler	NKInstallExceptionHandler(NKExceptionHandler proc,UInt32 handlerID);

extern PPCRegisters savedRegs[13];

enum
{
	// Indexes into savedRegs[] for various exception types and also ID's for NKInstallExceptionHandler
	unimplementedException		=	0,
	systemResetException		=	1,
	machineCheckException		=	2,
	dataStorageException		=	3,
	instructionStorageException	=	4,
	externalException			=	5,
	alignmentException			=	6,
	programException			=	7,
	fpuUnavailableException		=	8,
	decrementerException		=	9,
	systemCallException			=	10,
	traceException				=	11,
	fpuAssistException			=	12,
	ioControllerException		=	13
};

#endif /* __NK_INTERRUPT_VECTORS__ */