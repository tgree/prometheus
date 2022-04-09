/*
	DBDMA.h
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
	Terry Greeniaus	-	Sunday, 11 Oct. 98	-	Original creation of file
*/
#ifndef __DBDMA__
#define __DBDMA__

typedef struct DBDMARegs
{
	UReg32LE		channelControl;		// Writing port to the channelStatus register
	UReg32LE		channelStatus;		// Read-only status register
	UReg32LE		rsrv1;			// Reserved
	UReg32LE		commandPtrLo;		// Address of next command entry to be fetched.  Must be a 16-byte aligned address.
	UReg32LE		interruptSelect;	// Optional - used to select which CS_S7 - CS_S0 bits generate an interrupt
	UReg32LE		branchSelect;		// Optional - used to select which CS_S7 - CS_S0 bits generate a branch
	UReg32LE		waitSelect;		// Optional - used to select which CS_S7 - CS_S0 bits generate a pause
}DBDMARegs;

enum
{
	// DBDMARegs.channelStatus bits.  (1 << n) gets you the bit in the register
	CS_RUN		=	15,	// Starts execution on the channel
	CS_PAUSE	=	14,	// Set to pause execution, clear to resume execution
	CS_FLUSH		=	13,	// Causes any buffered data in the channel to be immediately written to memory
	CS_WAKE		=	12,	// Starts an idle channel at the address pointed to by commandPtr.  A channel goes idle after a STOP
	CS_DEAD		=	11,	// Set to 1 by hardware when a catastrophic (bus error, etc.) error occurs.  Cleared by clearing the CS_RUN bit.
	CS_ACTIVE	=	10,	// Set to 1 by hardware if currently executing instructions.
	CS_BT		=	8,	// Set by hardware at end of NOP, INPUT or OUTPUT commands to indicate if a branch was taken.
	CS_S7		=	7,	// Device-definable bits (unused so far as I know)
	CS_S6		=	6,
	CS_S5		=	5,
	CS_S4		=	4,
	CS_S3		=	3,
	CS_S2		=	2,
	CS_S1		=	1,
	CS_S0		=	0
};

enum
{
	// The cmd field for the DBDMA command
	CMD_OUTPUT_MORE	=	0,	// Transfer more memory to stream
	CMD_OUTPUT_LAST	=	1,	// Transfer last memory to stream
	CMD_INPUT_MORE	=	2,	// Transfer more stream to memory
	CMD_INPUT_LAST	=	3,	// Transfer last stream to memory
	CMD_STORE_QUAD	=	4,	// Store immediate 4-byte value
	CMD_LOAD_QUAD	=	5,	// Load immediate 4-byte value
	CMD_NOP			=	6,	// No data transfer
	CMD_STOP		=	7	// Suspend command processing
};

enum
{
	// The key field for the DBDMA command
	KEY_STREAM0	=	0,	// Default device stream
	KEY_STREAM1	=	1,	// Device-dependant stream
	KEY_STREAM2	=	2,	// Device-dependant stream
	KEY_STREAM3	=	3,	// Device-dependant stream
	KEY_REGS		=	5,	// Channel-state register space
	KEY_SYSTEM	=	6,	// System memory-mapped space
	KEY_DEVICE	=	7	// Device memory-mapped space
};

enum
{
	// The i (interrupt) field for the DBDMA command.  The Interrupt Condition bit is determined by:
	//	c = (ChannelStatus.s7..s0 & InterruptSelect.mask) == (InterruptSelect.value & InterruptSelect.mask)
	INT_NEVER	=	0,	// Never interrupt
	INT_IC_SET	=	1,	// Interrupt if the Interrupt Condition bit is set
	INT_IC_CLEAR	=	2,	// Interrupt if the Interrupt Condition bit is cleared
	INT_ALWAYS	=	3	// Always interrupt
};

enum
{
	// The b (branch) field for the DBDMA command.  The Branch Condition bit is determined by:
	//	c = (ChannelStatus.s7..s0 & BranchSelect.mask) == (BranchSelect.value & BranchSelect.mask)
	BRANCH_NEVER		=	0,	// Never branch
	BRANCH_BC_SET	=	1,	// Branch if the Branch Condition bit is set
	BRANCH_BC_CLEAR	=	2,	// Branch if the Branch Condition bit is cleared
	BRANCH_ALWAYS	=	3	// Always branch
};

enum
{
	// The w (wait) field for the DBDMA command.  The Wait Condition bit is determined by:
	//	c = (ChannelStatus.s7..s0 & WaitSelect.mask) == (WaitSelect.value & WaitSelect.mask)
	WAIT_NEVER		=	0,	// Never wait
	WAIT_WC_SET		=	1,	// Wait if the Wait Condition bit is set
	WAIT_WC_CLEAR	=	2,	// Wait fi the Wait Condition bit is cleared
	WAIT_ALWAYS		=	3	// Always wait
};

typedef struct DBDMACommand
{
	// This structure is formatted for big-endian access.  Make sure to use byte-reversing instructions
	// for the little-endian fields.
	UInt16	reqCount;			// This is a little-endian field!!!
	UInt16	rsrv1	:	2;
	UInt16	i		:	2;
	UInt16	b		:	2;
	UInt16	w		:	2;
	UInt16	cmd		:	4;
	UInt16	rsrv2	:	1;
	UInt16	key		:	3;
	UInt32	address;			// This is a little-endian field!!!
	UInt32	cmdDep;			// This is a little-endian field!!!
	UInt16	resCount;			// This is a little-endian field!!!
	UInt16	xferStatus;		// This is a little-endian field!!!
}DBDMACommand;

#endif /* __DBDMA__ */