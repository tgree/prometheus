/*
	NKDisassembly.cp
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
	???
	
	Version History
	============
	Patrick Varilly			Thur, 15 Jan 98		Original creation of file
	Patrick Varilly			Mon, 19 Jan 98			Initial version history tagging
											Added header file for consistency with rest of nanokernel
											Tidied file up and added a few explanatory comments
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file - is
*/
/*
	NKDisassembly.cp
	
	A fast and safe disassembler for PowerPC code. (Interrupt safe)
	
	Copyright © 1998 by The Pandora Team. All rights reserved worldwide.
	Permission to use and modify this file is given solely to the Pandora Team until further notice
*/

#include "NKDisassembly.h"

/*
	Typedefs
*/
#pragma mark Typedefs
typedef void (*assemblyHandler)( UInt32 opcode, ASCII8Str string );
	// A pointer to a primary or secondary assembly opcode handler

/*
	Static globals
*/
#pragma mark Static Globals
static ASCII8 Num2HexTab[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	// A table used to convert nibbles to hex characters

/*
	Prototypes
*/
#pragma mark Prototypes

#pragma mark ÊÊÊÊHandlers
// The disassembler uses a set of tables with pointers to "handler" routines to speed up disassembly
// Following is a list of all possible "handler" routines.
#define HANDLER_DECLARE	static void
#define HANDLER_PARAMS	UInt32 opcode, ASCII8Str string
#define HANDLER(name)		HANDLER_DECLARE name( HANDLER_PARAMS )
HANDLER(Invalid);
HANDLER(OpcodeThirtyOne);
HANDLER(AddFamily);
HANDLER(AddImmediateFamily);
HANDLER(AddExtendedFamily);
HANDLER(AndFamily);
HANDLER(AndImmediateFamily);
HANDLER(Branch);
HANDLER(BranchConditional);
HANDLER(NineTeenFamily);
HANDLER(BranchConditionalFamily);
HANDLER(CompareFamily);
HANDLER(CompareImmediateFamily);
HANDLER(CountLeadingZeros);
HANDLER(ConditionRegisterFamily);
HANDLER(DataCacheBlockFamily);
HANDLER(DivideWordFamily);
HANDLER(ExternalControlFamily);
HANDLER(Equivalent);
HANDLER(ExtendFamily);
HANDLER(FloatingPointFamily);
HANDLER(FEightFamily);
HANDLER(FAddSubtractFamily);
HANDLER(FCmpFamily);
HANDLER(FConvertIntegerFamily);
HANDLER(FDivideFamily);
HANDLER(FMultiplyMultipleFamily);
HANDLER(FMultiplyFamily);
HANDLER(FEstimateFamily);
HANDLER(FRoundToSingle);
HANDLER(FSelect);
HANDLER(FSqrtFamily);
HANDLER(InstructionCacheBlockInvalidate);
HANDLER(InstructionSynchronize);
HANDLER(LoadIntegerFamily);
HANDLER(LoadIntegerIndexFamily);
HANDLER(LoadFPFamily);
HANDLER(LoadFPIndexFamily);
HANDLER(LoadStringWordImmediate);
HANDLER(MoveConditionRegisterFamily);
HANDLER(MoveFromFPSCR);
HANDLER(MoveToFromMSR);
HANDLER(MoveToFromSPR);
HANDLER(MoveToFromSR);
HANDLER(MoveToFromSRIndirect);
HANDLER(MoveFromTimeBase);
HANDLER(MoveToCRFields);
HANDLER(MoveToFPSCRBitFamily);
HANDLER(MoveToFPSCR);
HANDLER(MoveToFPSCRImmediate);
HANDLER(MultiplyHighWordFamily);
HANDLER(MultiplyLowImmediate);
HANDLER(MultiplyLowWordFamily);
HANDLER(NandNorOrOrcFamily);
HANDLER(NegFamily);
HANDLER(OrImmediateFamily);
HANDLER(ReturnFromInterrupt);
HANDLER(RotateLeftWithMaskInsert);
HANDLER(RotateLeftImmediateAndMask);
HANDLER(RotateLeftAndMask);
HANDLER(SystemCall);
HANDLER(ShiftWordFamily);
HANDLER(ShiftRightWordImmediate);
HANDLER(StoreIntegerFamily);
HANDLER(StoreIntegerIndexedFamily);
HANDLER(StoreFPFamily);
HANDLER(StoreFPIndexedFamily);
HANDLER(StoreStringWordImmediate);
HANDLER(SubtractFamily);
HANDLER(SubtractFromImmediate);
HANDLER(SubtractExtendedFamily);
HANDLER(Synchronize);
HANDLER(TransitionLookasideBufferFamily);
HANDLER(Xor);
HANDLER(XorImmediateFamily);
HANDLER(TrapWord);
HANDLER(TrapWordImmediate);

#pragma mark ÊÊÊÊUtilities
// These are prototypes to useful procedures used in disassembling common parts of opcodes.
// For example, these routines handle the writing of general purpose registers and floating point
// register.
static Boolean decodeTrapOperation( Int8 TO, ASCII8Str opcodeName, UInt8* opcodeNameLength );
static Boolean decodeSPR( Int16 spr, ASCII8Str sprName, UInt8* sprNameLength );
static UInt8 writeFRegDReg( ASCII8Str string, UInt8 offset, UInt8 fpOne, Int16  d, UInt8 rTwo );
static UInt8 writeFRegTwoRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 rTwo, UInt8 rThree );
static UInt8 writeFourFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo, UInt8 fpThree, UInt8 fpFour );
static UInt8 writeThreeFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo, UInt8 fpThree );
static UInt8 writeTwoFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo );
static UInt8 writeFPReg( ASCII8Str string, UInt8 offset, UInt8 reg );
static UInt8 writeRegDReg( ASCII8Str string, UInt8 offset, UInt8 rOne, Int16 d, UInt8 rTwo );
static UInt8 writeThreeRegs( ASCII8Str string, UInt8 offset, UInt8 rOne, UInt8 rTwo, UInt8 rThree );
static UInt8 writeTwoRegs( ASCII8Str string, UInt8 offset, UInt8 rOne, UInt8 rTwo );
static UInt8 writeReg( ASCII8Str string, UInt8 offset, UInt8 reg );
static UInt8 writeIMM( ASCII8Str string, UInt8 offset, Int16 IMM );
static UInt8 writeAddress( ASCII8Str string, UInt8 offset, Int32 addr );

// This is an OS/Hardware-independent memory copying routine. When possible, this should be replaced by the
// OS' native copying routine, to speed up disassembly.
static void memCopy( Int8* dst, Int8* src, UInt32 len );

/*
	Tables
*/
#pragma mark Tables

#pragma mark ÊÊÊÊPrimary Opcode Table
// This table is used to call an appropiate "handler" routine, based on bits 0 to 5 (the primary ocpode) of the instruction
// opcode. For each possible primary opcode, there is an entry in this table. Any primary opcode not defined in the PowerPC
// has an Invalid entry.
//
// Note that some handlers are actually second-level dispatchers (e.g. OpcodeThirtyOne), since some primary opcodes
// are used as escape codes to signal the use of a secondary opcode.
static assemblyHandler	primaryOpcodeTable[64] = {
	Invalid,					Invalid,				Invalid,				TrapWordImmediate,		// 0
	Invalid,					Invalid,				Invalid,				MultiplyLowImmediate,	// 4
	SubtractFromImmediate,		Invalid,				CompareImmediateFamily,	CompareImmediateFamily,	// 8
	AddImmediateFamily,		AddImmediateFamily,	AddImmediateFamily,	AddImmediateFamily,	// 12
	BranchConditional,			SystemCall,			Branch,				NineTeenFamily,		// 16
	RotateLeftWithMaskInsert,	RotateLeftImmediateAndMask,	Invalid,			RotateLeftAndMask,		// 20
	OrImmediateFamily,			OrImmediateFamily,		XorImmediateFamily,	XorImmediateFamily,	// 24
	AndImmediateFamily,		AndImmediateFamily,	Invalid,				OpcodeThirtyOne,		// 28
	LoadIntegerFamily,			LoadIntegerFamily,		LoadIntegerFamily,		LoadIntegerFamily,		// 32
	StoreIntegerFamily,			StoreIntegerFamily,		StoreIntegerFamily,		StoreIntegerFamily,		// 36
	LoadIntegerFamily,			LoadIntegerFamily,		LoadIntegerFamily,		LoadIntegerFamily,		// 40
	StoreIntegerFamily,			StoreIntegerFamily,		LoadIntegerFamily,		StoreIntegerFamily,		// 44
	LoadFPFamily,				LoadFPFamily,			LoadFPFamily,			LoadFPFamily,			// 48
	StoreFPFamily,				StoreFPFamily,			StoreFPFamily,			StoreFPFamily,			// 52
	Invalid,					Invalid,				Invalid,				FloatingPointFamily,		// 56
	Invalid,					Invalid,				Invalid,				FloatingPointFamily };	// 60

#pragma mark ÊÊÊÊThityOne Table
// This is the second-level dispatching table used by OpcodeThirtyOne. Any PowerPC opcode which has bits 0 to 5
// equal to thirty-one has a 9/10-bit secondary opcode in bits 21/22 to 30. Any secondary opcode not defined
// in the PowerPC architecture has an Invalid entry in this table.
//
// To compensate for the 9/10 bit difference, this opcode uses 10-bit secondary opcodes. Any instructions which use
// 9-bit opcodes actually have two distinct entries in this table: one with bit 0 off and one with bit 0 on (i.e., the secondary
// opcode plus 512).
//
// I think this should be changed into a two-level table, for easier management: a primary table contains pointers to
// small secondary tables. Since about 30-40% of the second-level tables would simply contain 8 Invalid entries
// one can use this same secondary table, instead of using distinct but equal secondary tables. The primary table
// would contain 128 entries, which are dispatched via bits 0-6 of the secondary opcode, while the remaining
// 3 bits would be used as an index into the second-level table.
//
// The speed lost would be negligible compared to how manageable this table would become

// DELETE THIS LATER :-) --> The table is formatted very messily right now. Have to correct that!
static assemblyHandler	thirtyOneTable[1024] = {
	CompareFamily,	Invalid, Invalid, Invalid, TrapWord, Invalid, Invalid, Invalid,	// 0
	SubtractFamily,	Invalid, AddFamily, MultiplyHighWordFamily, Invalid, Invalid, Invalid, Invalid,		// 8
	Invalid,			Invalid, Invalid, MoveConditionRegisterFamily, LoadIntegerIndexFamily, Invalid, Invalid, LoadIntegerIndexFamily,	// 16
	ShiftWordFamily,	Invalid, CountLeadingZeros, Invalid, AndFamily, Invalid, Invalid, Invalid,	// 24
	CompareFamily,	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 32
	SubtractFamily,	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 40
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, DataCacheBlockFamily, LoadIntegerIndexFamily,	// 48
	Invalid,			Invalid, Invalid, Invalid, AndFamily, Invalid, Invalid, Invalid,	// 56
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 64
	Invalid,			Invalid, Invalid, MultiplyHighWordFamily, Invalid, Invalid, Invalid, Invalid,	// 72
	Invalid,			Invalid, Invalid, MoveToFromMSR, Invalid, Invalid, DataCacheBlockFamily, LoadIntegerIndexFamily,	// 80
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 88
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 96
	NegFamily,		Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 104
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, LoadIntegerIndexFamily,	// 112
	Invalid,			Invalid, Invalid, Invalid, NandNorOrOrcFamily, Invalid, Invalid, Invalid,	// 120
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 128
	SubtractFamily,	Invalid, AddFamily, Invalid, Invalid, Invalid, Invalid, Invalid,	// 136
	MoveToCRFields,	Invalid, MoveToFromMSR, Invalid, Invalid, Invalid, StoreIntegerIndexedFamily, StoreIntegerIndexedFamily,	// 144
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 152 (Well, you get the idea!!!)
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 160
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 168
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, StoreIntegerIndexedFamily,	// 176
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 184
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 192
	SubtractExtendedFamily, Invalid, AddExtendedFamily, Invalid, Invalid, Invalid, Invalid, Invalid,	// 200
	Invalid,			Invalid, MoveToFromSR, Invalid, Invalid, Invalid, Invalid, StoreIntegerIndexedFamily,	// 208
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 216
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 224
	SubtractExtendedFamily, Invalid, AddExtendedFamily, MultiplyLowWordFamily, Invalid, Invalid, Invalid, Invalid,	// 232
	Invalid,			Invalid, MoveToFromSRIndirect, Invalid, Invalid, Invalid, DataCacheBlockFamily, StoreIntegerIndexedFamily,	// 240
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 248
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 256
	Invalid,			Invalid, AddFamily, Invalid, Invalid, Invalid, Invalid, Invalid,		// 264
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, DataCacheBlockFamily, LoadIntegerIndexFamily,	// 272
	Invalid,			Invalid, Invalid, Invalid, Equivalent, Invalid, Invalid, Invalid,	// 280
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 288
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 296
	Invalid,			Invalid, TransitionLookasideBufferFamily, Invalid, Invalid, Invalid, ExternalControlFamily, LoadIntegerIndexFamily,	// 304
	Invalid,			Invalid, Invalid, Invalid, Xor, Invalid, Invalid, Invalid,	// 312
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 320
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 328
	Invalid,			Invalid, Invalid, MoveToFromSPR, Invalid, Invalid, Invalid, LoadIntegerIndexFamily,	// 336
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 344
	Invalid,			Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 352
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 360
	Invalid, Invalid, TransitionLookasideBufferFamily, MoveFromTimeBase, Invalid, Invalid, Invalid, LoadIntegerIndexFamily,	// 368
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 376
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 384
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 392
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, StoreIntegerIndexedFamily,	// 400
	Invalid, Invalid, Invalid, Invalid, NandNorOrOrcFamily, Invalid, Invalid, Invalid,	// 408
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 416
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 424
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, ExternalControlFamily, StoreIntegerIndexedFamily,	// 432
	Invalid, Invalid, Invalid, Invalid, NandNorOrOrcFamily, Invalid, Invalid, Invalid,	// 440
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 448
	Invalid, Invalid, Invalid, DivideWordFamily, Invalid, Invalid, Invalid, Invalid,	// 456
	Invalid, Invalid, Invalid, MoveToFromSPR, Invalid, Invalid, DataCacheBlockFamily, Invalid,	// 464
	Invalid, Invalid, Invalid, Invalid, NandNorOrOrcFamily, Invalid, Invalid, Invalid,	// 472
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 480
	Invalid, Invalid, Invalid, DivideWordFamily, Invalid, Invalid, Invalid, Invalid,	// 488
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 496
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 504
	MoveConditionRegisterFamily, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 512
	SubtractFamily, Invalid, AddFamily, Invalid, Invalid, Invalid, Invalid, Invalid,		// 520
	Invalid, Invalid, Invalid, Invalid, Invalid, LoadIntegerIndexFamily, LoadIntegerIndexFamily, LoadFPIndexFamily,	// 528
	ShiftWordFamily, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 536
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 544
	SubtractFamily, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 552
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, TransitionLookasideBufferFamily, LoadFPIndexFamily,	// 560
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 568
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 576
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 584
	Invalid, Invalid, Invalid, MoveToFromSR, Invalid, Invalid, Synchronize, LoadFPIndexFamily,	// 592
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 600
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 608
	NegFamily, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 616
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, LoadFPIndexFamily,	// 624
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 632
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 640
	SubtractFamily, Invalid, AddFamily, Invalid, Invalid, Invalid, Invalid, Invalid,		// 648
	Invalid, Invalid, Invalid, MoveToFromSRIndirect, Invalid, StoreIntegerIndexedFamily, StoreIntegerIndexedFamily, StoreFPIndexedFamily,	// 656
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 664
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 672
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 680
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, StoreFPIndexedFamily,	// 688
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 696
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 704
	SubtractExtendedFamily, Invalid, AddExtendedFamily, Invalid, Invalid, Invalid, Invalid, Invalid,	// 712
	Invalid, Invalid, Invalid, Invalid, Invalid, StoreStringWordImmediate, Invalid, StoreFPIndexedFamily,	// 720
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 728
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 736
	SubtractExtendedFamily, Invalid, AddExtendedFamily, MultiplyLowWordFamily, Invalid, Invalid, Invalid, Invalid,	// 744
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, DataCacheBlockFamily, StoreFPIndexedFamily,	// 752
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 760
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 768
	Invalid, Invalid, AddFamily, Invalid, Invalid, Invalid, Invalid, Invalid,
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, LoadIntegerIndexFamily, Invalid,
	ShiftWordFamily, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 792
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 800
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 808
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 816
	ShiftRightWordImmediate, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 824
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 832
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 840
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, ExternalControlFamily, Invalid,	// 848
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 856
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 864
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 872
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 880
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 888
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 896
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 904
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, StoreIntegerIndexedFamily, Invalid,	// 912
	Invalid, Invalid, ExtendFamily, Invalid, Invalid, Invalid, Invalid, Invalid,	// 920
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 928
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 936
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 944
	Invalid, Invalid, ExtendFamily, Invalid, Invalid, Invalid, Invalid, Invalid,	// 952
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 960
	Invalid, Invalid, Invalid, DivideWordFamily, Invalid, Invalid, Invalid, Invalid,	// 968
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, InstructionCacheBlockInvalidate, StoreFPIndexedFamily,	// 976
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 984
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid,	// 992
	Invalid, Invalid, Invalid, DivideWordFamily, Invalid, Invalid, Invalid, Invalid,	// 1000
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, DataCacheBlockFamily, Invalid,	// 1008
	Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid, Invalid	// 1016
};	// WOW!!! Wasn't that a big table

#pragma mark ÊÊÊÊFloating Point Table
// This table is used to dispatch
static assemblyHandler		floatingPointTable[64] = {
	FCmpFamily, Invalid, Invalid, Invalid,		// 0
	Invalid, Invalid, MoveToFPSCRBitFamily, MoveFromFPSCR,	// 4
	FEightFamily, Invalid, Invalid, Invalid,	// 8
	FRoundToSingle, Invalid, FConvertIntegerFamily, FConvertIntegerFamily,	// 12
	Invalid, Invalid, FDivideFamily, Invalid,	// 16
	FAddSubtractFamily, FAddSubtractFamily, FSqrtFamily, FSelect,		// 20
	FEstimateFamily, FMultiplyFamily, FEstimateFamily, Invalid,	// 24
	FMultiplyMultipleFamily, FMultiplyMultipleFamily, FMultiplyMultipleFamily, FMultiplyMultipleFamily,	// 28
	FCmpFamily, Invalid, Invalid, Invalid,		// 32
	Invalid, Invalid, Invalid, Invalid,	// 36
	Invalid, Invalid, Invalid, Invalid,	// 40
	Invalid, Invalid, Invalid, Invalid,	// 44
	Invalid, Invalid, Invalid, Invalid,	// 48
	Invalid, Invalid, Invalid, Invalid,	// 52
	Invalid, Invalid, Invalid, Invalid,	// 56
	Invalid, Invalid, Invalid, Invalid	// 60
};

#pragma mark Macros

// A memory copying macro. Although this file is completely OS-independent, it's preferable to have the memory copying
// function of the native OS here
#define MEMCOPY(dst,src,len)		memCopy( ((Int8*)(dst)), ((Int8*)(src)), ((UInt32)(len)) )

// Some macros to extract bits from opcodes
#define GET_FLD(bit1,bit2,op)		(((UInt32)((UInt32)op) >> (31UL - (bit2))) & ( (UInt32)((1UL << (((UInt32)bit2) - ((UInt32)bit1)) + 1UL) - 1UL) ))
#define GET_OP_FLD(bit1,bit2)		(GET_FLD(bit1,bit2,opcode))
#define GET_BIT(bit,op)			(GET_FLD(bit,bit,op))
#define GET_OP_BIT(bit)			(GET_BIT(bit,opcode))

// Some constants
#define opcodeStart				0		// Must be less than paramStart by at least 10 and greater than 0
#define paramStart				16		// Must be 10 or more, greater than opcodeStart
#define hexStart				36		// Must be 36 or less and greater than paramStart

// Some useful macros
#define WRITE_CHAR(dest,off,theChar) ((dest)[(off)] = (theChar))
#define WRITE_DIGIT(dest,off,dig)	WRITE_CHAR(dest,off,((dig)+0x30))
#define WRITE_STR(dest,off,str,len) MEMCOPY( ((Int8*)(dest)+(off)), (str), (len) )

#pragma mark -

/*
	Disassemble: A routine to disassemble a PowerPC opcode
	Parameters:
		opcode		a 32-bit PowerPC opcode.
		string		a pointer to a string at least 51-characters wide (counting the terminating null) on which the
					disassembled output will be place.
	Returns:
		A pointer to the output string.
*/

ASCII8Str Disassemble( UInt32 opcode, ASCII8Str string )
{
	// Clear string
	for( Int32 i=0; i<50; i++ )
		string[i] = ' ';
	string[50] = 0;
	
	// Dispatch via primary opcode (first 6 bits of opcode)
	UInt8		primaryOpcode;
	
	primaryOpcode = GET_OP_FLD(0,5);
	(*primaryOpcodeTable[primaryOpcode])( opcode, string );
	
	// Write the opcode in hex
	ASCII8				hexOpcode[13] = { '|', ' ', '0', 'x',
			Num2HexTab[(opcode>>28)&0xF], Num2HexTab[(opcode>>24)&0xF],
			Num2HexTab[(opcode>>20)&0xF], Num2HexTab[(opcode>>16)&0xF],
			Num2HexTab[(opcode>>12)&0xF], Num2HexTab[(opcode>>8)&0xF],
			Num2HexTab[(opcode>>4)&0xF], Num2HexTab[opcode&0xF] };
	
	WRITE_STR( string, hexStart, hexOpcode, 13 );
	
	return string;
}

/* Handlers: Routines to handle individual mnemonics or mnemonic families
	Parameters:
		opcode		The 32-bit opcode to disassemble
		string		The string in which the disassembled output must be put in. (note: you musn't end this string with a
						terminating null character; the string's entire contents are 50 spaces, so you can write
						at any position in the string and get the expected results).
*/

HANDLER_DECLARE Invalid(HANDLER_PARAMS)
{
	#pragma unused(opcode)
	
	ASCII8Str			str = "Invalid Instruction";
	WRITE_STR( string, opcodeStart, "Invalid Instruction", 19 );		// Copy without the trailing null
}

HANDLER_DECLARE OpcodeThirtyOne (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	
	// Dispatch via secondary opcode
	// Secondary opcodes which use the msb of secondaryOpcode as the OE bit (e.g. add)
	// must have two entries in the thrityOneTable: one with that bit on and one with the bit
	// off. The handler can be the same or different.
	(*thirtyOneTable[secondaryOpcode])( opcode, string );
}

HANDLER_DECLARE AddFamily (HANDLER_PARAMS)
{
	UInt8		rD, rA, rB;
	UInt8		OE, Rc;
	UInt16		secondaryOpcode;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	secondaryOpcode = GET_OP_FLD(22,30);
	
	UInt8		offset = opcodeStart;
	WRITE_STR(string,offset,"add",3);
	offset += 3;
	
	UInt8		toAdd;
	switch( secondaryOpcode )
	{
		case 266:	toAdd = 0;	break;
		case 10:	toAdd = 'c';	break;
		case 138:	toAdd = 'e';	break;
	}
	if( toAdd != 0 ) WRITE_CHAR(string,offset++,toAdd);
	
	if( OE == 1 )
		WRITE_CHAR(string,offset++,'o');
	if( Rc == 1 )
		WRITE_CHAR(string,offset++,'.');
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE AddImmediateFamily (HANDLER_PARAMS)
{
	UInt8		rD,rA;
	Int16		SIMM;
	UInt8		primaryOpcode;
	
	primaryOpcode = GET_OP_FLD(0,5);
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SIMM = GET_OP_FLD(16,31);
	
	// Handle simplified mnemonics
	UInt8		offset = opcodeStart;
	
	if( (rA == 0) && ((primaryOpcode == 14) || (primaryOpcode == 15)) )
	{				// li or lis
		WRITE_STR( string, offset, "li", 2 );
		offset += 2;
		if( primaryOpcode == 15 )
			WRITE_CHAR( string, offset, 's' );
		offset = writeReg( string, paramStart, rD );
		WRITE_CHAR( string, offset++, ',' );
		writeIMM( string, offset, SIMM );
		return;
	}
	else if( SIMM < 0 )	// subi
	{
		WRITE_STR( string, offset, "subi", 4 );
		SIMM = -SIMM;
	}
	else				// addi
		WRITE_STR( string, offset, "addi", 4 );
	
	offset += 4;
	if( primaryOpcode == 12 )
		WRITE_CHAR( string, offset, 'c' );
	else if( primaryOpcode == 13 )
		WRITE_STR( string, offset, "c.", 2 );
	else if( primaryOpcode == 15 )
		WRITE_CHAR( string, offset, 's' );
	
	offset = writeTwoRegs( string, paramStart, rD, rA );
	WRITE_CHAR(string,offset++,',');
	writeIMM(string,offset,SIMM);
}

HANDLER_DECLARE AddExtendedFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			OE,Rc;
	UInt8			rA,rD;
	
	secondaryOpcode = GET_OP_FLD(22,30);
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	if( secondaryOpcode == 234 )
		WRITE_STR(string,opcodeStart,"addme",5);
	else
		WRITE_STR(string,opcodeStart,"addze",5);
	
	UInt8			offset = opcodeStart+5;
	
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoRegs( string, paramStart, rD, rA );
}

HANDLER_DECLARE AndFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			rS,rA,rB;
	UInt8			Rc;
	
	secondaryOpcode = GET_OP_FLD(21,30);
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "and", 3 );
	offset += 3;
	
	if( secondaryOpcode == 60 )
		WRITE_CHAR( string, offset++, 'c' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rA, rS, rB );
}

HANDLER_DECLARE AndImmediateFamily (HANDLER_PARAMS)
{
	UInt8			primaryOpcode;
	UInt8			rS, rA;
	UInt16			UIMM;
	
	primaryOpcode = GET_OP_FLD(0,5);
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	UIMM = GET_OP_FLD(16,31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "andi", 4 );
	offset += 4;
	
	if( primaryOpcode == 29 )
		WRITE_CHAR( string, offset++, 's' );
	WRITE_CHAR( string, offset++, '.' );
	
	offset = writeTwoRegs( string, paramStart, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	writeIMM( string, offset, UIMM );
}

HANDLER_DECLARE Branch (HANDLER_PARAMS)
{
	UInt32			LI;
	UInt8			AA,LK;
	
	LI = GET_OP_FLD(6,29) << 2;
	AA = GET_OP_BIT(30);
	LK = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_CHAR( string, offset++, 'b' );
	if( LK == 1 )
		WRITE_CHAR( string, offset++, 'l' );
	if( AA == 1 )
		WRITE_CHAR( string, offset++, 'a' );
	
	offset = paramStart;
	if( !AA )
	{
		WRITE_CHAR( string, offset++, '*' );
		if( LI > 0x7FFFFF )	// Actually negative
		{
			LI = 0x1000000 - LI;
			WRITE_CHAR( string, offset++, '-' );
		}
		else
			WRITE_CHAR( string, offset++, '+' );
	}
	
	writeAddress( string, offset, LI );
}

HANDLER_DECLARE BranchConditional (HANDLER_PARAMS)
{
	Int16			BD;
	UInt8			BO,BI;
	UInt8			AA,LK;
	
	BO = GET_OP_FLD(6,10);
	BI = GET_OP_FLD(11,15);
	BD = GET_OP_FLD(16,29) << 2;
	AA = GET_OP_BIT(30);
	LK = GET_OP_BIT(31);
	
	// Handle simplified mnemonics (WA!!!!!!!! The simplified mnemonics for bc are just too many to implement just now)
	// We handle atm blt, bne and bdnz
	UInt8			offset = opcodeStart;
	
	if( (AA == 0) && (LK == 0) )
	{
		if( (BO == 12) && (BI == 0) )
		{		// blt
			WRITE_STR( string, offset, "blt", 3 );
			offset = paramStart;
			WRITE_CHAR( string, offset++, '*' );
			if( BD < 0 )
			{
				BD = -BD;
				WRITE_CHAR( string, offset++, '-' );
			}
			else
				WRITE_CHAR( string, offset++, '+' );
			writeIMM( string, offset, BD );
			return;
		}
		else if( (BO == 4) && (((BI-2) % 4) == 0) )
		{		// bne	cr2,xxxx
			WRITE_STR( string, offset, "bne", 3 );
			offset = paramStart;
			if( BI == 2 )
				WRITE_CHAR( string, offset++, '*' );
			else
			{
				WRITE_STR( string, paramStart, "cr", 2 );
				offset += 2;
				WRITE_DIGIT( string, offset++, ((BI-2) / 4) );
				WRITE_STR( string, paramStart, ",*", 2 );
				offset += 2;
			}
			
			if( BD < 0 )
			{
				BD = -BD;
				WRITE_CHAR( string, offset++, '-' );
			}
			else
				WRITE_CHAR( string, offset++, '+' );
			writeIMM( string, offset, BD );
			return;
		}
		else if( (BO == 16) && (BI == 0) )
		{		// bdnz
			WRITE_STR( string, offset, "bdnz", 4 );
			offset = paramStart;
			WRITE_CHAR( string, offset++, '*' );
			if( BD < 0 )
			{
				BD = -BD;
				WRITE_CHAR( string, offset++, '-' );
			}
			else
				WRITE_CHAR( string, offset++, '+' );
			writeIMM( string, offset, BD );
			return;
		}
	}
	
	// Normal bc
	WRITE_STR( string, offset, "bc", 2 );
	offset += 2;
	if( LK == 1 )
		WRITE_CHAR( string, offset++, 'l' );
	if( AA == 1 )
		WRITE_CHAR( string, offset++, 'a' );
	
	offset = paramStart;
	if( BO > 9 )	WRITE_DIGIT( string, offset++, BO/10 );
	WRITE_DIGIT( string, offset++, BO % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( BI > 9 )	WRITE_DIGIT( string, offset++, BI/10 );
	WRITE_DIGIT( string, offset++, BI % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( !AA )
	{
		WRITE_CHAR( string, offset++, '*' );
		if( BD < 0 )
		{
			BD = -BD;
			WRITE_CHAR( string, offset++, '-' );
		}
		else
			WRITE_CHAR( string, offset++, '+' );
	}
	writeIMM( string, offset, BD );
}

HANDLER_DECLARE NineTeenFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	
	secondaryOpcode = GET_OP_FLD(21,30);
	
	switch( secondaryOpcode )
	{
		case 0:
			MoveConditionRegisterFamily( opcode, string );
			break;
		case 528:
		case 16:
			BranchConditionalFamily( opcode, string );
			break;
		case 257:
		case 129:
		case 289:
		case 225:
		case 33:
		case 449:
		case 417:
		case 193:
			ConditionRegisterFamily( opcode, string );
			break;
		case 150:
			InstructionSynchronize( opcode, string );
			break;
		case 50:
			ReturnFromInterrupt( opcode, string );
			break;
	}
}

HANDLER_DECLARE BranchConditionalFamily (HANDLER_PARAMS)
{
	UInt8			BO,BI,LK;
	UInt16			secondaryOpcode;
	
	secondaryOpcode = GET_OP_FLD(21,30);
	BO = GET_OP_FLD(6,10);
	BI = GET_OP_FLD(11,15);
	LK = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	if( secondaryOpcode == 528 )
	{		// bcctr
		// Simplified mnemonics are too many for branch instructions. Let's not implement them just yet
		// One notable exception is bctr (bcctr		20,x)
		if( BO == 20 )
		{
			WRITE_STR( string, offset, "bctr", 4 );
			return;
		}
		
		WRITE_STR( string, offset, "bcctr", 5 );
		offset = paramStart;
		if( BO > 9 )	WRITE_DIGIT( string, offset++, BO/10 );
		WRITE_DIGIT( string, offset++, BO % 10 );
		WRITE_CHAR( string, offset++, ',' );
		if( BI > 9 )	WRITE_DIGIT( string, offset++, BI/10 );
		WRITE_DIGIT( string, offset, BI % 10 );
	}
	else if( secondaryOpcode == 16 )
	{		// bclr
		// Simplified mnemonics are too many for branch instructions. Let's not implement them just yet
		// One notable exception is blr (bclr		20,x)
		if( BO == 20 )
		{
			WRITE_STR( string, offset, "blr", 3 );
			return;
		}
		
		WRITE_STR( string, offset, "bclr", 4 );
		offset = paramStart;
		if( BO > 9 )	WRITE_DIGIT( string, offset++, BO/10 );
		WRITE_DIGIT( string, offset++, BO % 10 );
		WRITE_CHAR( string, offset++, ',' );
		if( BI > 9 )	WRITE_DIGIT( string, offset++, BI/10 );
		WRITE_DIGIT( string, offset, BI % 10 );
	}
}

HANDLER_DECLARE CompareFamily (HANDLER_PARAMS)
{
	UInt8			crD, L, rA, rB;
	UInt16			secondaryOpcode;
	
	secondaryOpcode = GET_OP_FLD(21,30);
	crD = GET_OP_FLD(6,8);
	L = GET_OP_BIT(10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 0:
			if( L == 1 )		// cmpd
				WRITE_STR( string, opcodeStart, "cmpd", 4 );
			else
				WRITE_STR( string, opcodeStart, "cmpw", 4 );
			break;
		case 32:
			if( L == 1 )		// cmpld
				WRITE_STR( string, opcodeStart, "cmpld", 4 );
			else
				WRITE_STR( string, opcodeStart, "cmplw", 4 );
			break;
	}
	
	UInt8			offset = paramStart;
	
	if( crD != 0 )
	{
		WRITE_STR( string, offset, "cr", 2 );
		offset += 2;
		WRITE_DIGIT( string, offset++, crD );
		WRITE_CHAR( string, offset++, ',' );
	}
	
	writeTwoRegs( string, offset, rA, rB );
}

HANDLER_DECLARE CompareImmediateFamily (HANDLER_PARAMS)
{
	UInt8		crD,L,rA;
	Int16		SIMM;
	UInt8		primaryOpcode;
	
	primaryOpcode = GET_OP_FLD(0,5);
	crD = GET_OP_FLD(6,8);
	L = GET_OP_BIT(10);
	rA = GET_OP_FLD(11,15);
	SIMM = GET_OP_FLD(16,31);
	
	switch( primaryOpcode )
	{
		case 11:
			if( L == 1 )
				WRITE_STR( string, opcodeStart, "cmpid", 5 );
			else
				WRITE_STR( string, opcodeStart, "cmpiw", 5 );
			break;
		case 10:
			if( L == 1 )
				WRITE_STR( string, opcodeStart, "cmplid", 5 );
			else
				WRITE_STR( string, opcodeStart, "cmpliw", 5 );
			break;
	}
	
	UInt8		offset = paramStart;
	
	if( crD != 0 )
	{
		WRITE_STR( string, offset, "cr", 2 );
		offset += 2;
		WRITE_DIGIT( string, offset++, crD );
		WRITE_CHAR( string, offset++, ',' );
	}
	
	offset = writeReg( string, offset, rA );
	WRITE_CHAR( string, offset++, ',' );
	if( (SIMM < 0) && (primaryOpcode != 10) )
	{
		SIMM = -SIMM;
		WRITE_CHAR( string, offset++, '-' );
	}
	writeIMM( string, offset, SIMM );
}

HANDLER_DECLARE CountLeadingZeros (HANDLER_PARAMS)
{
	UInt8		rS,rA,Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	WRITE_STR(string, offset, "cntlzw", 6 );
	offset += 6;
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoRegs( string, paramStart, rA, rS );
}

HANDLER_DECLARE ConditionRegisterFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			crbD,crbA,crbB;
	
	secondaryOpcode = GET_OP_FLD( 21, 30 );
	crbD = GET_OP_FLD( 6,10 );
	crbA = GET_OP_FLD( 11,15 );
	crbB = GET_OP_FLD( 16,20 );
	
	UInt8			offset = paramStart;
	
	switch( secondaryOpcode )
	{
		case 257:		WRITE_STR( string, opcodeStart, "crand", 5 );		break;
		case 129:		WRITE_STR( string, opcodeStart, "crandc", 6 );	break;
		case 289:
			if( (crbD == crbA) && (crbD == crbB) )
			{
				WRITE_STR( string, opcodeStart, "crset", 5 );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
				WRITE_DIGIT( string, offset++, crbD % 10 );
				return;
			}
			else
				WRITE_STR( string, opcodeStart, "creqv", 5 );
			break;
		case 225:		WRITE_STR( string, opcodeStart, "crnand", 6 );	break;
		case 33:
			if( (crbA == crbB) )
			{
				WRITE_STR( string, opcodeStart, "crnot", 5 );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
				WRITE_DIGIT( string, offset++, crbD % 10 );
				WRITE_CHAR( string, offset++, ',' );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbA > 9 )	WRITE_DIGIT( string, offset++, crbA / 10 );
				WRITE_DIGIT( string, offset++, crbA % 10 );
				return;
			}
			else
				WRITE_STR( string, opcodeStart, "crnor", 5 );
			break;
		case 449:
			if( (crbA == crbB) )
			{
				WRITE_STR( string, opcodeStart, "crmove", 6 );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
				WRITE_DIGIT( string, offset++, crbD % 10 );
				WRITE_CHAR( string, offset++, ',' );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbA > 9 )	WRITE_DIGIT( string, offset++, crbA / 10 );
				WRITE_DIGIT( string, offset++, crbA % 10 );
				return;
			}
			else
				WRITE_STR( string, opcodeStart, "cror", 4 );
			break;
		case 417:		WRITE_STR( string, opcodeStart, "crorc", 5 );		break;
		case 193:
			if( (crbD == crbA) && (crbD == crbB) )
			{
				WRITE_STR( string, opcodeStart, "crclr", 5 );
				WRITE_STR( string, offset, "crb", 3 );
				offset += 3;
				if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
				WRITE_DIGIT( string, offset++, crbD % 10 );
				return;
			}
			else
				WRITE_STR( string, opcodeStart, "crxor", 5 );
			break;
	}
	
	WRITE_STR( string, offset, "crb", 3 );
	offset += 3;
	if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
	WRITE_DIGIT( string, offset++, crbD % 10 );
	WRITE_CHAR( string, offset++, ',' );
	WRITE_STR( string, offset, "crb", 3 );
	offset += 3;
	if( crbA > 9 )	WRITE_DIGIT( string, offset++, crbA / 10 );
	WRITE_DIGIT( string, offset++, crbA % 10 );
	WRITE_CHAR( string, offset++, ',' );
	WRITE_STR( string, offset, "crb", 3 );
	offset += 3;
	if( crbB > 9 )	WRITE_DIGIT( string, offset++, crbB / 10 );
	WRITE_DIGIT( string, offset, crbB % 10 );
}

HANDLER_DECLARE DataCacheBlockFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			rA,rB;
	
	secondaryOpcode = GET_OP_FLD(21,30);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 758:	WRITE_STR( string, opcodeStart, "dcba", 4 );	break;
		case 86:	WRITE_STR( string, opcodeStart, "dcbf", 4 );	break;
		case 470:	WRITE_STR( string, opcodeStart, "dcbi", 4 );	break;
		case 54:	WRITE_STR( string, opcodeStart, "dcbst", 5 );	break;
		case 278:	WRITE_STR( string, opcodeStart, "dcbt", 4 );	break;
		case 246:	WRITE_STR( string, opcodeStart, "dcbtst", 6 );break;
		case 1014: WRITE_STR( string, opcodeStart, "dcbz", 4 );	break;
	}
	
	writeTwoRegs( string, paramStart, rA, rB );
}

HANDLER_DECLARE DivideWordFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			rD,rA,rB;
	UInt8			OE,Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	OE = GET_OP_BIT(21);
	secondaryOpcode = GET_OP_FLD(22,30);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "divw", 4 );
	offset += 4;
	if( secondaryOpcode == 459 )	// divwu
		WRITE_CHAR( string, offset++, 'u' );
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE ExternalControlFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode;
	UInt8			rD,rA,rB;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	secondaryOpcode = GET_OP_FLD(21,30);
	
	if( secondaryOpcode == 310 )		// eciwx
		WRITE_STR( string, opcodeStart, "eciwx", 5 );
	else if( secondaryOpcode == 438 )	// ecowx
		WRITE_STR( string, opcodeStart, "ecowx", 5 );
	else if( secondaryOpcode == 854 )	// eieio
	{
		WRITE_STR( string, opcodeStart, "eieio", 5 );
		return;		// eieio has no parameters
	}
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE Equivalent (HANDLER_PARAMS)
{
	UInt8			rS,rA,rB;
	UInt8			Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	WRITE_STR( string, opcodeStart, "eqv", 3 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+3, '.' );
	
	writeThreeRegs( string, paramStart, rA, rS, rB );
}

HANDLER_DECLARE ExtendFamily (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode;
	UInt8		rS,rA;
	UInt8		Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	secondaryOpcode = GET_OP_FLD(21,30);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	if( secondaryOpcode == 954 )
	{		// extsb
		WRITE_STR( string, offset, "extsb", 5 );
		offset += 5;
	}
	else if( secondaryOpcode == 922 )
	{		// extsh
		WRITE_STR( string, offset, "extsh", 5 );
		offset += 5;
	}
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoRegs( string, paramStart, rA, rS );
}

HANDLER_DECLARE FloatingPointFamily (HANDLER_PARAMS)
{
	// Floating point instructions are really wierd: They use two different primary opcodes and variable-sized
	// secondary opcodes. Fortunately, all instruction can be seperated by their primary opcode and the last 5
	// bits of their secondary opcode. Therefore, we use a small 64-entry table for dispatching.
	//
	// Individual secondary-level handlers are responsible of distinguishing between two equal secondary opcodes
	// and different primary opcodes (usually, these kinds of instructions fit into the same family)
	
	UInt8		secondaryOpcode = GET_OP_FLD(26,30);
	
	(*floatingPointTable[secondaryOpcode])( opcode, string );
}

HANDLER_DECLARE FEightFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	
	UInt8		fpD, fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	if( (primaryOpcode != 63) || ((secondaryOpcode != 264) && (secondaryOpcode != 72)
		&& (secondaryOpcode != 136) && (secondaryOpcode != 40)) )
	{
		Invalid( opcode, string );
		return;
	}
	
	UInt8			offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 264:	WRITE_STR( string, offset, "fabs", 4 );	offset += 4;	break;
		case 40:	WRITE_STR( string, offset, "fneg", 4 );	offset += 4;	break;
		case 136:	WRITE_STR( string, offset, "fnabs", 5 );	offset += 5;	break;
		case 72:	WRITE_STR( string, offset, "fmr", 3 );	offset += 3;	break;
	}
	if( Rc == 1 )
		WRITE_CHAR(string, offset, '.' );
	
	writeTwoFPRegs( string, paramStart, fpD, fpB );
}

HANDLER_DECLARE FAddSubtractFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		secondaryOpcode = GET_OP_FLD(26,30);
	UInt8		fpD, fpA, fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpA = GET_OP_FLD(11,15);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 20:	WRITE_STR( string, offset, "fsub", 4 );	offset += 4;	break;
		case 21:	WRITE_STR( string, offset, "fadd", 4 );	offset += 4;	break;
	}
	
	if( primaryOpcode == 59 )
		WRITE_CHAR( string, offset++, 's' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeFPRegs( string, paramStart, fpD, fpA, fpB );
}

HANDLER_DECLARE FCmpFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	
	UInt8		fpA, fpB;
	UInt8		crD;
	
	crD = GET_OP_FLD(6,8);
	fpA = GET_OP_FLD(11,15);
	fpB = GET_OP_FLD(16,20);
	
	if( (primaryOpcode == 63) && (secondaryOpcode == 64) )
	{
		MoveConditionRegisterFamily( opcode, string );
		return;
	}
	if( (primaryOpcode != 63) || ((secondaryOpcode != 32) && (secondaryOpcode != 0)) )
	{
		Invalid( opcode, string );
		return;
	}
	
	if( secondaryOpcode == 0 )
		WRITE_STR( string, opcodeStart, "fcmpu", 5 );
	else
		WRITE_STR( string, opcodeStart, "fcmpo", 5 );
	
	UInt8		offset = paramStart;
	WRITE_STR( string, offset, "cr", 2 );
	offset += 2;
	WRITE_DIGIT( string, offset++, crD );
	WRITE_CHAR( string, offset++, ',' );
	writeTwoFPRegs( string, offset, fpA, fpB );
}

HANDLER_DECLARE FConvertIntegerFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	
	UInt8		fpD, fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	if( (primaryOpcode != 63) || ((secondaryOpcode != 14) && (secondaryOpcode != 15)) )
	{
		Invalid( opcode, string );
		return;
	}
	
	UInt8		offset = opcodeStart;
	
	if( secondaryOpcode == 14 )
	{
		WRITE_STR( string, opcodeStart, "fctiw", 5 );
		offset += 5;
	}
	else
	{
		WRITE_STR( string, opcodeStart, "fctiwz", 6 );
		offset += 6;
	}
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoFPRegs( string, paramStart, fpD, fpB );
}

HANDLER_DECLARE FDivideFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		fpD,fpA,fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpA = GET_OP_FLD(11,15);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	if( primaryOpcode == 63 )
	{
		WRITE_STR( string, offset, "fdiv", 4 );
		offset += 4;
	}
	else if( primaryOpcode == 59 )
	{
		WRITE_STR( string, offset, "fdivs", 5 );
		offset += 5;
	}
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeFPRegs( string, paramStart, fpD, fpA, fpB );
}

HANDLER_DECLARE FMultiplyMultipleFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		secondaryOpcode = GET_OP_FLD(26,30);
	
	UInt8		fpD,fpA,fpB,fpC;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpA = GET_OP_FLD(11,15);
	fpB = GET_OP_FLD(16,20);
	fpC = GET_OP_FLD(21,25);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 28:
			WRITE_STR( string, offset, "fmsub", 5 );
			offset += 5;
			break;
		case 29:
			WRITE_STR( string, offset, "fmadd", 5 );
			offset += 5;
			break;
		case 30:
			WRITE_STR( string, offset, "fnmsub", 6 );
			offset += 6;
			break;
		case 31:
			WRITE_STR( string, offset, "fnmadd", 6 );
			offset += 6;
			break;
	}
	
	if( primaryOpcode == 59 )
		WRITE_CHAR( string, offset++, 's' );
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeFourFPRegs( string, paramStart, fpD, fpA, fpB, fpC );
}

HANDLER_DECLARE FMultiplyFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		fpD,fpA,fpC;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpA = GET_OP_FLD(11,15);
	fpC = GET_OP_FLD(21,25);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	WRITE_STR( string, offset, "fmul", 4 );
	offset += 4;
	if( primaryOpcode == 59 )
		WRITE_CHAR( string, offset++, 's' );
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeFPRegs( string, paramStart, fpD, fpA, fpC );
}

HANDLER_DECLARE FEstimateFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		secondaryOpcode = GET_OP_FLD(26,30);
	UInt8		fpD,fpB;
	UInt8		Rc;
	
	if( ((primaryOpcode == 63) && (secondaryOpcode == 24))
		|| ((primaryOpcode == 59) && (secondaryOpcode == 26)) )
	{
		Invalid( opcode, string );
		return;
	}
	
	fpD = GET_OP_FLD(6,10);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 24:	WRITE_STR( string, offset, "fres", 4 );		offset += 4;	break;
		case 26:	WRITE_STR( string, offset, "frsqrte", 7 );	offset += 7;	break;
	}
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoFPRegs( string, paramStart, fpD, fpB );
}

HANDLER_DECLARE FRoundToSingle (HANDLER_PARAMS)
{
	UInt8		fpD,fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	WRITE_STR( string, opcodeStart, "frsp", 4 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+4, '.' );
	
	writeTwoFPRegs( string, paramStart, fpD, fpB );
}

HANDLER_DECLARE FSelect (HANDLER_PARAMS)
{
	UInt8		fpD,fpA,fpB,fpC;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpA = GET_OP_FLD(11,15);
	fpB = GET_OP_FLD(16,20);
	fpC = GET_OP_FLD(21,25);
	Rc = GET_OP_BIT(31);
	
	WRITE_STR( string, opcodeStart, "fsel", 4 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+4, '.' );
	
	writeFourFPRegs( string, paramStart, fpD, fpA, fpB, fpC );
}

HANDLER_DECLARE FSqrtFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		fpD,fpB;
	UInt8		Rc;
	
	fpD = GET_OP_FLD(6,10);
	fpB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	
	switch( primaryOpcode )
	{
		case 63:	WRITE_STR( string, offset, "fsqrt", 5 );	offset += 5;	break;
		case 59:	WRITE_STR( string, offset, "fsqrts", 6 );	offset += 6;	break;
	}
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoFPRegs( string, paramStart, fpD, fpB );
}

HANDLER_DECLARE InstructionCacheBlockInvalidate (HANDLER_PARAMS)
{
	UInt8		rA,rB;
	
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	WRITE_STR( string, opcodeStart, "icbi", 4 );
	writeTwoRegs( string, paramStart, rA, rB );
}

HANDLER_DECLARE InstructionSynchronize (HANDLER_PARAMS)
{
	#pragma unused(opcode)
	WRITE_STR( string, opcodeStart, "isync", 5 );
}

HANDLER_DECLARE LoadIntegerFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		rD, rA;
	Int16		d;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	d = GET_OP_FLD(16,31);
	
	switch( primaryOpcode )
	{
		case 32:	WRITE_STR( string, opcodeStart, "lwz", 3 );	break;
		case 33:	WRITE_STR( string, opcodeStart, "lwzu", 3 );	break;
		case 34:	WRITE_STR( string, opcodeStart, "lbz", 3 );	break;
		case 35:	WRITE_STR( string, opcodeStart, "lbzu", 4 );	break;
		case 40:	WRITE_STR( string, opcodeStart, "lhz", 3 );	break;
		case 41:	WRITE_STR( string, opcodeStart, "lhzu", 4 );	break;
		case 42:	WRITE_STR( string, opcodeStart, "lha", 3 );	break;
		case 43:	WRITE_STR( string, opcodeStart, "lhau", 4 );	break;
		case 46:	WRITE_STR( string, opcodeStart, "lmw", 3 );	break;
	}
	
	writeRegDReg( string, paramStart, rD, d, rA );
}

HANDLER_DECLARE LoadIntegerIndexFamily (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		rD,rA,rB;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 20:	WRITE_STR( string, opcodeStart, "lwarx", 5 );	break;
		case 23:	WRITE_STR( string, opcodeStart, "lwzx", 4 );	break;
		case 55:	WRITE_STR( string, opcodeStart, "lwzux", 5 );	break;
		case 87:	WRITE_STR( string, opcodeStart, "lbzx", 4 );	break;
		case 119:	WRITE_STR( string, opcodeStart, "lbzux", 5 );	break;
		case 279:	WRITE_STR( string, opcodeStart, "lhzx", 4 );	break;
		case 311:	WRITE_STR( string, opcodeStart, "lhzux", 5 );	break;
		case 343:	WRITE_STR( string, opcodeStart, "lhax", 4 );	break;
		case 375:	WRITE_STR( string, opcodeStart, "lhaux", 5 );	break;
		case 533:	WRITE_STR( string, opcodeStart, "lswx", 4 );	break;
		case 534:	WRITE_STR( string, opcodeStart, "lwbrx", 5 );	break;
		case 790:	WRITE_STR( string, opcodeStart, "lhbrx", 5 );	break;
	}
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE LoadFPFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		fpD, rA;
	Int16		d;
	
	fpD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	d = GET_OP_FLD(16,31);
	
	switch( primaryOpcode )
	{
		case 48:	WRITE_STR( string, opcodeStart, "lfs", 3 );	break;
		case 49:	WRITE_STR( string, opcodeStart, "lfsu", 4 );	break;
		case 50:	WRITE_STR( string, opcodeStart, "lfd", 3 );	break;
		case 51:	WRITE_STR( string, opcodeStart, "lfdu", 4 );	break;
	}
	
	writeFRegDReg( string, paramStart, fpD, d, rA );
}

HANDLER_DECLARE LoadFPIndexFamily (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		fpD,rA,rB;
	
	fpD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 535:	WRITE_STR( string, opcodeStart, "lfsx", 4 );	break;
		case 567:	WRITE_STR( string, opcodeStart, "lfsux", 5 );	break;
		case 599:	WRITE_STR( string, opcodeStart, "lfdx", 4 );	break;
		case 631:	WRITE_STR( string, opcodeStart, "lfdux", 5 );	break;
	}
	
	writeFRegTwoRegs( string, paramStart, fpD, rA, rB );
}

HANDLER_DECLARE LoadStringWordImmediate (HANDLER_PARAMS)
{
	UInt8		rD, rA, NB;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	NB = GET_OP_FLD(16,20);
	
	WRITE_STR( string, opcodeStart, "lswi", 4 );
	UInt8		offset;
	offset = writeTwoRegs( string, paramStart, rD, rA );
	WRITE_CHAR( string, offset++, ',' );
	WRITE_CHAR( string, offset++, '0' );
	WRITE_CHAR( string, offset++, 'x' );
	WRITE_CHAR( string, offset++, Num2HexTab[(NB>>4)&0xF] );
	WRITE_CHAR( string, offset, Num2HexTab[NB&0xF] );
}

HANDLER_DECLARE MoveConditionRegisterFamily (HANDLER_PARAMS)
{
	// This gets called from various different handlers, but always at secondary level
	// (specifically, this gets called from NineTeenFamily, FCmpFamily, )
	
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		crbD,crbS;
	
	crbD = GET_OP_FLD(6,8);
	crbS = GET_OP_FLD(11,13);
	
	UInt8		offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 0:	WRITE_STR( string, opcodeStart, "mcrf", 4 );	break;
		case 64:	WRITE_STR( string, opcodeStart, "mcrfs", 5 );	break;
		case 512:	WRITE_STR( string, opcodeStart, "mcrxr", 5);
			offset = paramStart;
			WRITE_STR( string, offset, "cr", 2 );
			offset += 2;
			WRITE_DIGIT( string, offset++, crbD );
			return;
		case 19:	WRITE_STR( string, opcodeStart, "mfcr", 4 );
			writeReg( string, paramStart, GET_OP_FLD(6,10) );
			return;
	}
	
	offset = paramStart;
	WRITE_STR( string, offset, "cr", 2 );
	offset += 2;
	WRITE_DIGIT( string, offset++, crbD );
	WRITE_STR( string, offset, ",cr", 3 );
	offset += 3;
	WRITE_DIGIT( string, offset, crbS );
}

HANDLER_DECLARE MoveFromFPSCR (HANDLER_PARAMS)
{
	UInt16				secondaryOpcode = GET_OP_FLD(21,30);
	
	if( secondaryOpcode == 711 )
	{					// Do some in-the-way tertiary dispatching
		MoveToFPSCR( opcode, string );
		return;
	}
	
	UInt8			rA;
	UInt8			Rc;
	
	rA = GET_OP_FLD(6,10);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "mffs", 4 );
	offset += 4;
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeFPReg( string, paramStart, rA );
}

HANDLER_DECLARE MoveToFromMSR (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			rD;
	rD = GET_OP_FLD(6,10);
	
	if( secondaryOpcode == 146 )
		WRITE_STR( string, opcodeStart, "mtmsr", 5 );
	else
		WRITE_STR( string, opcodeStart, "mfmsr", 5 );
	writeReg( string, paramStart, rD );
}

HANDLER_DECLARE MoveToFromSPR (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		rD;
	UInt16		spr;
	
	rD = GET_OP_FLD(6,10);
	spr = GET_OP_FLD(11,15) | (GET_OP_FLD(16,20)<<5);	// The SPR is encoded in a wierd order
	
	ASCII8		sprName[6];
	UInt8		sprNameLength = 0;
	Boolean		identified;
	
	identified = decodeSPR( spr, sprName, &sprNameLength );
	
	UInt8		offset = opcodeStart;
	if( secondaryOpcode == 467 )
		WRITE_STR( string, offset, "mt", 2 );
	else
		WRITE_STR( string, offset, "mf", 2 );
	offset += 2;
	WRITE_STR( string, offset, sprName, sprNameLength );
	
	#define WRITE_SPR_NUM	do {\
			if( spr > 999 )\
			{\
				WRITE_DIGIT( string, offset++, spr / 1000 );\
				spr %= 1000;\
				WRITE_DIGIT( string, offset++, spr / 100 );\
				spr %= 100;\
				WRITE_DIGIT( string, offset++, spr / 10 );\
			}\
			else if( spr > 99 )\
			{\
				WRITE_DIGIT( string, offset++, spr / 100 );\
				spr %= 100;\
				WRITE_DIGIT( string, offset++, spr / 10 );\
			}\
			else if( spr > 9 )	WRITE_DIGIT( string, offset++, spr / 10 );\
			WRITE_DIGIT( string, offset++, spr % 10 );\
		} while(0)
	
	offset = paramStart;
	if( (secondaryOpcode == 467) && (!identified) )		// mtspr has spr number before register
	{
		WRITE_SPR_NUM;
		WRITE_CHAR( string, offset++, ',' );
	}
	offset = writeReg( string, offset, rD );
	if( (secondaryOpcode != 467) && (!identified) )			// but mfspr has it the other way round!
	{
		WRITE_CHAR( string, offset++, ',' );
		WRITE_SPR_NUM;
	}
	
	#undef WRITE_SPR_NUM
}

HANDLER_DECLARE MoveToFromSR (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		rD;
	UInt8		SR;
	
	rD = GET_OP_FLD(6,10);
	SR = GET_OP_FLD(12,15);
	
	if( secondaryOpcode == 210 )
		WRITE_STR( string, opcodeStart, "mtsr", 4 );
	else
		WRITE_STR( string, opcodeStart, "mfsr", 4 );
	
	UInt8		offset = paramStart;
	
	if( secondaryOpcode == 210 )
	{
		if( SR > 9 )	WRITE_DIGIT( string, offset++, SR / 10 );
		WRITE_DIGIT( string, offset++, SR % 10 );
		WRITE_CHAR( string, offset++, ',' );
	}
	offset = writeReg( string, offset, rD );
	if( secondaryOpcode != 210 )
	{
		WRITE_CHAR( string, offset++, ',' );
		if( SR > 9 )	WRITE_DIGIT( string, offset++, SR / 10 );
		WRITE_DIGIT( string, offset++, SR % 10 );
	}
}

HANDLER_DECLARE MoveToFromSRIndirect (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(21,30);
	UInt8		rD,rB;
	
	rD = GET_OP_FLD(6,10);
	rB = GET_OP_FLD(16,20);
	
	if( secondaryOpcode == 242 )
		WRITE_STR( string, opcodeStart, "mtsrin", 6 );
	else
		WRITE_STR( string, opcodeStart, "mfsrin", 6 );
	writeTwoRegs( string, paramStart, rD, rB );
}

HANDLER_DECLARE MoveFromTimeBase (HANDLER_PARAMS)
{
	UInt8		rD;
	UInt16		tbr;
	
	rD = GET_OP_FLD(6,10);
	tbr = GET_OP_FLD(11,15) | (GET_OP_FLD(16,20)<<5);	// The TBR is encoded in a wierd order
	
	WRITE_STR( string, opcodeStart, "mftb", 4 );
	if( tbr == 269 )
		WRITE_CHAR( string, opcodeStart+4, 'u' );
	
	writeReg( string, paramStart, rD );
}

HANDLER_DECLARE MoveToCRFields (HANDLER_PARAMS)
{
	UInt8		rS, CRM;
	
	rS = GET_OP_FLD(6,10);
	CRM = GET_OP_FLD(12,19);
	
	// Handle simplified mnemonics
	if( CRM == 0xFF )
	{
		WRITE_STR( string, opcodeStart, "mtcr", 4 );
		writeReg( string, paramStart, rS );
		return;
	}
	
	WRITE_STR( string, opcodeStart, "mtcrf", 5 );
	
	UInt8		offset = paramStart;
	WRITE_CHAR( string, offset++, '0' );
	WRITE_CHAR( string, offset++, 'x' );
	WRITE_CHAR( string, offset++, Num2HexTab[(CRM>>4)&0xF] );
	WRITE_CHAR( string, offset++, Num2HexTab[CRM&0xF] );
	WRITE_CHAR( string, offset++, ',' );
	writeReg( string, offset, rS );
}

HANDLER_DECLARE MoveToFPSCRBitFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			crbD,Rc;
	
	crbD = GET_OP_FLD(6,10);
	Rc = GET_OP_BIT(31);
	
	if( secondaryOpcode == 134 )
	{					// Do some in-the-way tertiary dispatching
		MoveToFPSCRImmediate( opcode, string );
		return;
	}
	else if( secondaryOpcode == 70 )
		WRITE_STR( string, opcodeStart, "mtfsb0", 6 );
	else
		WRITE_STR( string, opcodeStart, "mtfsb1", 6 );
	
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+6, '.' );
	
	UInt8			offset = paramStart;
	WRITE_STR( string, offset, "crb", 3 );
	offset += 3;
	if( crbD > 9 )	WRITE_DIGIT( string, offset++, crbD / 10 );
	WRITE_DIGIT( string, offset, crbD % 10 );
}

HANDLER_DECLARE MoveToFPSCR (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			Rc = GET_OP_BIT(31);
	UInt8			FMR,fpB;
	
	FMR = GET_OP_FLD(7,14);
	fpB = GET_OP_FLD(16,20);
	
	if( secondaryOpcode != 711 )
	{
		Invalid( opcode, string );
		return;
	}
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "mtfsf", 5 );
	offset += 5;
	if( Rc == 1 )
		WRITE_CHAR( string, offset, '.' );
	
	offset = paramStart;
	WRITE_CHAR( string, offset++, '0' );
	WRITE_CHAR( string, offset++, 'x' );
	WRITE_CHAR( string, offset++, Num2HexTab[(FMR>>4)&0xF] );
	WRITE_CHAR( string, offset++, Num2HexTab[FMR&0xF] );
	WRITE_CHAR( string, offset++, ',' );
	writeFPReg( string, offset, fpB );
}

HANDLER_DECLARE MoveToFPSCRImmediate (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			Rc = GET_OP_BIT(31);
	UInt8			crD,IMM;
	
	crD = GET_OP_FLD(6,8);
	IMM = GET_OP_FLD(16,19);
	
	if( secondaryOpcode != 134 )
	{
		Invalid( opcode, string );
		return;
	}
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "mtfsfi", 6 );
	offset += 6;
	if( Rc == 1 )
		WRITE_CHAR( string, offset, '.' );
	
	offset = paramStart;
	WRITE_STR( string, offset, "cr", 2 );
	offset += 2;
	WRITE_DIGIT( string, offset++, crD );
	WRITE_CHAR( string, offset++, ',' );
	WRITE_CHAR( string, offset++, '0' );
	WRITE_CHAR( string, offset++, 'x' );
	WRITE_CHAR( string, offset++, Num2HexTab[IMM] );
}

HANDLER_DECLARE MultiplyHighWordFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(22,30);
	UInt8			rD,rA,rB;
	UInt8			Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "mulhw", 5 );
	offset += 5;
	if( secondaryOpcode == 11 )
		WRITE_CHAR( string, offset++, 'u' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE MultiplyLowImmediate (HANDLER_PARAMS)
{
	UInt8		rD,rA;
	Int16		SIMM;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SIMM = GET_OP_FLD(16,31);
	
	WRITE_STR( string, opcodeStart, "mulli", 5 );
	
	UInt8		offset = paramStart;
	offset = writeTwoRegs( string, offset, rD, rA );
	WRITE_CHAR( string, offset++, ',' );
	if( SIMM < 0 )
	{
		SIMM = -SIMM;
		WRITE_CHAR( string, offset++, '-' );
	}
	writeIMM( string, offset, SIMM );
}

HANDLER_DECLARE MultiplyLowWordFamily (HANDLER_PARAMS)
{
	UInt8			rD,rA,rB;
	UInt8			OE,Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	
	WRITE_STR( string, offset, "mullw", 5 );
	offset += 5;
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rD, rA, rB );
}

HANDLER_DECLARE NandNorOrOrcFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			rS,rA,rB;
	UInt8			Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 476:	WRITE_STR( string, offset, "nand", 4 );	offset += 4;	break;
		case 412:	WRITE_STR( string, offset, "orc", 3 );	offset += 3;	break;
		case 124:
			if( rS == rB )
			{
				WRITE_STR( string, offset, "not", 3 );
				offset += 3;
				if( Rc == 1 )
					WRITE_CHAR( string, offset++, '.' );
				writeTwoRegs( string, paramStart, rA, rS );
				return;
			}
			
			WRITE_STR( string, offset, "nor", 3 );	offset += 3;	break;
		case 444:
			if( rS == rB )
			{
				WRITE_STR( string, offset, "mr", 2 );
				offset += 2;
				if( Rc == 1 )
					WRITE_CHAR( string, offset++, '.' );
				writeTwoRegs( string, paramStart, rA, rS );
				return;
			}
			
			WRITE_STR( string, offset, "or", 2 );	offset += 2;	break;
	}
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rA, rS, rB );
}

HANDLER_DECLARE NegFamily (HANDLER_PARAMS)
{
	UInt8			rD,rA;
	UInt8			OE,Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	WRITE_STR( string, offset, "neg", 3 );
	offset += 3;
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoRegs( string, paramStart, rD, rA );
}

HANDLER_DECLARE OrImmediateFamily (HANDLER_PARAMS)
{
	UInt8			primaryOpcode = GET_OP_FLD(0,5);
	UInt8			rS,rA;
	UInt16			UIMM;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	UIMM = GET_OP_FLD(16,31);
	
	// Handle simplified mnemonics
	if( (rS == 0) && (rA == 0) && (UIMM == 0) && (primaryOpcode == 24) )
	{		// Hehe, I like this mnemonic
		WRITE_STR( string, opcodeStart, "nop", 3 );
		return;
	}
	
	switch( primaryOpcode )
	{
		case 24:	WRITE_STR( string, opcodeStart, "ori", 3 );	break;
		case 25:	WRITE_STR( string, opcodeStart, "oris", 4 );	break;
	}
	
	UInt8		offset = paramStart;
	offset = writeTwoRegs( string, offset, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	writeIMM( string, offset, UIMM );
}

HANDLER_DECLARE ReturnFromInterrupt (HANDLER_PARAMS)
{
	#pragma unused(opcode)
	WRITE_STR( string, opcodeStart, "rfi", 3 );
}

HANDLER_DECLARE RotateLeftWithMaskInsert (HANDLER_PARAMS)
{
	UInt8		rS,rA;
	UInt8		SH,MB,ME;
	UInt8		Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SH = GET_OP_FLD(16,20);
	MB = GET_OP_FLD(21,25);
	ME = GET_OP_FLD(26,30);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = paramStart;

	// Handle simplified mnemonics (ugh... rotate and mask instructions have ugly simplified mnemonics)
	UInt8		b = MB;
	UInt8		n = ME + 1 - b;
	
	if( (MB == b) && (ME == ((b+n)-1)) )
	{
		if( SH == (32 - b) )					// inslwi
			WRITE_STR( string, opcodeStart, "inslwi", 6 );
		else if( (SH == 32 - (b+n)) && (n>0) )	// insrwi
			WRITE_STR( string, opcodeStart, "insrwi", 6 );
		else
			goto noSimple;
		
		if( Rc == 1 )
			WRITE_CHAR( string, opcodeStart+6, '.' );
		
		offset = writeTwoRegs( string, offset, rA, rS );
		WRITE_CHAR( string, offset++, ',' );
		if( n > 9 )	WRITE_DIGIT( string, offset++, n / 10 );
		WRITE_DIGIT( string, offset++, n % 10 );
		WRITE_CHAR( string, offset++, ',' );
		if( b > 9 )	WRITE_DIGIT( string, offset++, b / 10 );
		WRITE_DIGIT( string, offset++, b % 10 );
		return;
	}
	
noSimple:
	WRITE_STR( string, opcodeStart, "rlwimi", 6 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+6, '.' );
	
	offset = writeTwoRegs( string, offset, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	if( SH > 9 )	WRITE_DIGIT( string, offset++, SH / 10 );
	WRITE_DIGIT( string, offset++, SH % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( MB > 9 )	WRITE_DIGIT( string, offset++, MB / 10 );
	WRITE_DIGIT( string, offset++, MB % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( ME > 9 )	WRITE_DIGIT( string, offset++, ME / 10 );
	WRITE_DIGIT( string, offset++, ME % 10 );
}

HANDLER_DECLARE RotateLeftImmediateAndMask (HANDLER_PARAMS)
{
	UInt8		rS,rA;
	UInt8		SH,MB,ME;
	UInt8		Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SH = GET_OP_FLD(16,20);
	MB = GET_OP_FLD(21,25);
	ME = GET_OP_FLD(26,30);
	Rc = GET_OP_BIT(31);
	
	UInt8		offset = opcodeStart;
	UInt8		firstParam = -1, secondParam = 255;

	// Handle simplified mnemonics (ugh... rotate and mask instructions have ugly simplified mnemonics)
	if( MB == 0 )
	{		// Many simplified mnemonics have MB = 0
		if( ME == 31 )
		{		// rotlwi and rotrwi. We use the form which gives the smallest n (i.e. a left shift 17 bits is simplified
				// as a right shift 15 bits)
			UInt8		n = SH;
			if( n == 0 )
			{	// Actually, this is a single use of clrrwi, but I can't make it fit where it should be
				WRITE_STR( string, offset, "clrrwi", 6 );
				firstParam = 0;
			}
			else if( n < (32-n) )
			{	// rotlwi
				WRITE_STR( string, offset, "rotlwi", 6 );
				firstParam = n;
			}
			else
			{	// rotrwi
				WRITE_STR( string, offset, "rotrwi", 6 );
				firstParam = 32-n;
			}
			offset += 6;
		}
		else if( SH == 0 )
		{	// clrrwi
			WRITE_STR( string, offset, "clrrwi", 6 );
			offset += 6;
			firstParam = 31-ME;
		}
		else if( ME == (31-SH) )
		{	// slwi
			WRITE_STR( string, offset, "slwi", 4 );
			offset += 4;
			firstParam = SH;
		}
		else
		{	// extlwi
			WRITE_STR( string, offset, "extlwi", 6 );
			offset += 6;
			firstParam = ME+1;
			secondParam = SH;
		}
	}
	else if( ME == 31 )
	{		// Most other simplified mnemonics have ME = 31
		if( SH == 0 )
		{	// clrlwi
			WRITE_STR( string, offset, "clrlwi", 6 );
			offset += 6;
			firstParam = MB;
		}
		else if( SH == (32-MB) )
		{	// srwi
			WRITE_STR( string, offset, "srwi", 4 );
			offset += 4;
			firstParam = MB;
		}
		else if( (32-MB) <= SH )
		{	// extrwi
			WRITE_STR( string, offset, "extrwi", 6 );
			offset += 6;
			firstParam = 32-MB;
			secondParam = SH - (32-MB);
		}
		else
		{	// clrlslwi	b>1,0
			WRITE_STR( string, offset, "clrlslwi", 8 );
			offset += 8;
			firstParam = MB;
			secondParam = 0;
		}
	}
	else if( ME == (31-SH) )
	{	// clrlslwi
		WRITE_STR( string, offset, "clrlslwi", 8 );
		offset += 8;
		firstParam = MB+SH;
		secondParam = SH;
	}
	else
		goto noSimple;
	
	if( firstParam == -1 )
		goto noSimple;
	
	// Output simplified mnemonic
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	offset = writeTwoRegs( string, paramStart, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	if( firstParam > 9 )	WRITE_DIGIT( string, offset++, firstParam / 10 );
	WRITE_DIGIT( string, offset++, firstParam % 10 );
	if( secondParam != 255 )
	{
		WRITE_CHAR( string, offset++, ',' );
		if( secondParam > 9 )	WRITE_DIGIT( string, offset++, secondParam / 10 );
		WRITE_DIGIT( string, offset++, secondParam % 10 );
	}
	return;
	
noSimple:
	// Output normal mnemonic
	WRITE_STR( string, opcodeStart, "rlwinm", 6 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+6, '.' );
	
	offset = writeTwoRegs( string, paramStart, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	if( SH > 9 )	WRITE_DIGIT( string, offset++, SH / 10 );
	WRITE_DIGIT( string, offset++, SH % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( MB > 9 )	WRITE_DIGIT( string, offset++, MB / 10 );
	WRITE_DIGIT( string, offset++, MB % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( ME > 9 )	WRITE_DIGIT( string, offset++, ME / 10 );
	WRITE_DIGIT( string, offset++, ME % 10 );
}

HANDLER_DECLARE RotateLeftAndMask (HANDLER_PARAMS)
{
	UInt8		rS,rA,rB;
	UInt8		MB,ME;
	UInt8		Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	MB = GET_OP_FLD(21,25);
	ME = GET_OP_FLD(26,30);
	Rc = GET_OP_BIT(31);
	
	// Handle simplified mnemonics (this one's simple)
	if( (MB == 0) && (ME == 31) )
	{
		WRITE_STR( string, opcodeStart, "rotlw", 5 );
		if( Rc == 1 )
			WRITE_CHAR( string, opcodeStart+5, '.' );
		writeThreeRegs( string, paramStart, rA, rS, rB );
		return;
	}
	
	WRITE_STR( string, opcodeStart, "rlwnm", 5 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+5, '.' );
	
	UInt8		offset = paramStart;
	offset = writeThreeRegs( string, offset, rA, rS, rB );
	WRITE_CHAR( string, offset++, ',' );
	if( MB > 9 )	WRITE_DIGIT( string, offset++, MB / 10 );
	WRITE_DIGIT( string, offset++, MB % 10 );
	WRITE_CHAR( string, offset++, ',' );
	if( ME > 9 )	WRITE_DIGIT( string, offset++, ME / 10 );
	WRITE_DIGIT( string, offset++, ME % 10 );
}

HANDLER_DECLARE SystemCall (HANDLER_PARAMS)
{
	if( GET_OP_FLD(6,31) != 2 )
	{
		Invalid( opcode, string );
		return;
	}
	
	WRITE_STR( string, opcodeStart, "sc", 2 );
}

HANDLER_DECLARE ShiftWordFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			rS,rA,rB;
	UInt8			Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 24:	WRITE_STR( string, offset, "slw", 3 );	offset += 3;	break;
		case 536:	WRITE_STR( string, offset, "srw", 3 );	offset += 3;	break;
		case 792:	WRITE_STR( string, offset, "sraw", 4 );	offset += 4;	break;
	}
	
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rA, rS, rB );
}

HANDLER_DECLARE ShiftRightWordImmediate (HANDLER_PARAMS)
{
	UInt8			rS,rA,SH;
	UInt8			Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SH = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	UInt8			offset = opcodeStart;
	
	WRITE_STR( string, offset, "srawi", 5 );
	offset += 5;
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	offset = writeTwoRegs( string, paramStart, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	if( SH > 9 )	WRITE_DIGIT( string, offset++, SH / 10 );
	WRITE_DIGIT( string, offset, SH % 10 );
}

HANDLER_DECLARE StoreIntegerFamily (HANDLER_PARAMS)
{
	UInt8			primaryOpcode = GET_OP_FLD(0,5);
	UInt8			rS,rA;
	Int16			d;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	d = GET_OP_FLD(16,31);
	
	switch( primaryOpcode )
	{
		case 36:	WRITE_STR( string, opcodeStart, "stw", 3 );	break;
		case 37:	WRITE_STR( string, opcodeStart, "stwu", 4 );	break;
		case 38:	WRITE_STR( string, opcodeStart, "stb", 3 );	break;
		case 39:	WRITE_STR( string, opcodeStart, "stbu", 4 );	break;
		case 44:	WRITE_STR( string, opcodeStart, "sth", 3 );	break;
		case 45:	WRITE_STR( string, opcodeStart, "sthu", 4 );	break;
		case 47:	WRITE_STR( string, opcodeStart, "stmw", 4 );	break;
	}
	
	writeRegDReg( string, paramStart, rS, d, rA );
}

HANDLER_DECLARE StoreIntegerIndexedFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			rS,rA,rB;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 150:	WRITE_STR( string, opcodeStart, "stwcx.", 6 );	break;
		case 151:	WRITE_STR( string, opcodeStart, "stwx", 4 );		break;
		case 183:	WRITE_STR( string, opcodeStart, "stwux", 5 );	break;
		case 215:	WRITE_STR( string, opcodeStart, "stbx", 4 );		break;
		case 247:	WRITE_STR( string, opcodeStart, "stbux", 5 );		break;
		case 407:	WRITE_STR( string, opcodeStart, "sthx", 4 );		break;
		case 439:	WRITE_STR( string, opcodeStart, "sthux", 5 );		break;
		case 661:	WRITE_STR( string, opcodeStart, "stswx", 5 );	break;
		case 662:	WRITE_STR( string, opcodeStart, "stwbrx", 6 );	break;
		case 918:	WRITE_STR( string, opcodeStart, "sthbrx", 6 );	break;
	}
	
	writeThreeRegs( string, paramStart, rS, rA, rB );
}

HANDLER_DECLARE StoreFPFamily (HANDLER_PARAMS)
{
	UInt8			primaryOpcode = GET_OP_FLD(0,5);
	UInt8			fpS, rA;
	Int16			d;
	
	fpS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	d = GET_OP_FLD(16,31);
	
	switch( primaryOpcode )
	{
		case 52:	WRITE_STR( string, opcodeStart, "stfs", 4 );	break;
		case 53:	WRITE_STR( string, opcodeStart, "stfsu", 5 );	break;
		case 54:	WRITE_STR( string, opcodeStart, "stfd", 4 );	break;
		case 55:	WRITE_STR( string, opcodeStart, "stfdu", 5 );	break;
	}
	
	writeFRegDReg( string, paramStart, fpS, d, rA );
}

HANDLER_DECLARE StoreFPIndexedFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			fpS,rA,rB;
	
	fpS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	switch( secondaryOpcode )
	{
		case 663:	WRITE_STR( string, opcodeStart, "stfsx", 5 );		break;
		case 695:	WRITE_STR( string, opcodeStart, "stfsux", 6 );	break;
		case 727:	WRITE_STR( string, opcodeStart, "stfdx", 5 );		break;
		case 759:	WRITE_STR( string, opcodeStart, "stfdux", 6 );	break;
		case 983:	WRITE_STR( string, opcodeStart, "stfiwx", 6 );	break;
	}
	
	writeFRegTwoRegs( string, paramStart, fpS, rA, rB );
}

HANDLER_DECLARE StoreStringWordImmediate (HANDLER_PARAMS)
{
	UInt8			rS,rA,NB;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	NB = GET_OP_FLD(16,20);
	
	WRITE_STR( string, opcodeStart, "stswi", 5 );
	
	UInt8			offset = paramStart;
	offset = writeTwoRegs( string, offset, rS, rA );
	WRITE_CHAR( string, offset++, ',' );
	if( NB > 9 )	WRITE_DIGIT( string, offset++, NB / 10 );
	WRITE_DIGIT( string, offset++, NB % 10 );
}

HANDLER_DECLARE SubtractFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(22,30);
	UInt8			rD,rA,rB;
	UInt8			OE,Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	// Subtract is always simplified!!!
	UInt8		offset = opcodeStart;
	
	switch( secondaryOpcode )
	{
		case 8:	WRITE_STR( string, offset, "subc", 4 );	offset += 4;	break;
		case 40:	WRITE_STR( string, offset, "sub", 3 );	offset += 3;	break;
		case 136:	WRITE_STR( string, offset, "sube", 4 );	offset += 4;	break;
			// Note that sube doesn't seem to appear as a simplified form of subfe, but it follows, so I use it here
	}
	
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeThreeRegs( string, paramStart, rD, rB, rA );
			// Notice how rA and rB switch places when using simplified mnemonics
}

HANDLER_DECLARE SubtractFromImmediate (HANDLER_PARAMS)
{
	UInt8		rD,rA;
	Int16		SIMM;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SIMM = GET_OP_FLD(16,31);
	
	WRITE_STR( string, opcodeStart, "subfic", 6 );
	
	UInt8		offset = paramStart;
	offset = writeTwoRegs( string, offset, rD, rA );
	WRITE_CHAR( string, offset++, ',' );
	if( SIMM < 0 )
	{
		SIMM = -SIMM;
		WRITE_CHAR( string, offset++, '-' );
	}
	writeIMM( string, offset, SIMM );
}

HANDLER_DECLARE SubtractExtendedFamily (HANDLER_PARAMS)
{
	UInt16		secondaryOpcode = GET_OP_FLD(22,30);
	UInt8		rD,rA;
	UInt8		OE,Rc;
	
	rD = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	OE = GET_OP_BIT(21);
	Rc = GET_OP_BIT(31);
	
	switch( secondaryOpcode )
	{
		case 200:	WRITE_STR( string, opcodeStart, "subfze", 6 );	break;
		case 232:	WRITE_STR( string, opcodeStart, "subfme", 6 );	break;
	}
	
	UInt8		offset = opcodeStart+6;
	if( OE == 1 )
		WRITE_CHAR( string, offset++, 'o' );
	if( Rc == 1 )
		WRITE_CHAR( string, offset++, '.' );
	
	writeTwoRegs( string, paramStart, rD, rA );
}

HANDLER_DECLARE Synchronize (HANDLER_PARAMS)
{
	#pragma unused(opcode)
	WRITE_STR( string, opcodeStart, "sync", 4 );
}

HANDLER_DECLARE TransitionLookasideBufferFamily (HANDLER_PARAMS)
{
	UInt16			secondaryOpcode = GET_OP_FLD(21,30);
	UInt8			rB = 255;
	
	switch( secondaryOpcode )
	{
		case 306:	WRITE_STR( string, opcodeStart, "tlbie", 5 );	rB = GET_OP_FLD(16,20);	break;
		case 370:	WRITE_STR( string, opcodeStart, "tlbia", 5 );		break;
		case 566:	WRITE_STR( string, opcodeStart, "tlbsync", 7 );	break;
	}
	
	if( rB != 255 )
		writeReg( string, paramStart, rB );
}

HANDLER_DECLARE TrapWord (HANDLER_PARAMS)
{
	UInt8		TO;
	UInt8		rA,rB;
	
	TO = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	
	ASCII8		trapWord[5];
	UInt8		trapWordLength, identified;
	
	identified = decodeTrapOperation( TO, trapWord, &trapWordLength );
	
	if( (!identified) && (TO == 31) && (rA == 0) && (rB == 0) )
	{
		// Special simplified mnemonic
		WRITE_STR( string, opcodeStart, "trap", 4 );
		return;
	}
	
	WRITE_STR( string, opcodeStart, trapWord, trapWordLength );
	
	UInt8		offset = paramStart;
	if( !identified )
	{
		if( TO > 9 )	WRITE_DIGIT( string, offset++, TO / 10 );
		WRITE_DIGIT( string, offset++, TO % 10 );
		WRITE_CHAR( string, offset++, ',' );
	}
	writeTwoRegs( string, offset, rA, rB );
}

HANDLER_DECLARE TrapWordImmediate (HANDLER_PARAMS)
{
	UInt8		TO;
	UInt8		rA;
	Int16		SIMM;
	
	TO = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	SIMM = GET_OP_FLD(16,31);
	
	ASCII8		trapWord[5];
	Boolean		identified;
	UInt8		trapWordLength;
	
	identified = decodeTrapOperation( TO, trapWord, &trapWordLength );
	
	WRITE_STR( string, opcodeStart, trapWord, trapWordLength );
	WRITE_CHAR( string, opcodeStart+trapWordLength, 'i' );
	
	UInt8		offset = paramStart;
	if( !identified )
	{
		if( TO > 9 )	WRITE_DIGIT( string, offset++, TO / 10 );
		WRITE_DIGIT( string, offset++, TO % 10 );
		WRITE_CHAR( string, offset++, ',' );
	}
	offset = writeReg( string, offset, rA );
	WRITE_CHAR( string, offset++, ',' );
	if( SIMM < 0 )
	{
		SIMM = -SIMM;
		WRITE_CHAR( string, offset++, '-' );
	}
	writeIMM( string, offset, SIMM );
}

HANDLER_DECLARE Xor (HANDLER_PARAMS)
{
	UInt8		rS,rA,rB;
	UInt8		Rc;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	rB = GET_OP_FLD(16,20);
	Rc = GET_OP_BIT(31);
	
	WRITE_STR( string, opcodeStart, "xor", 3 );
	if( Rc == 1 )
		WRITE_CHAR( string, opcodeStart+3, '.' );
	
	writeThreeRegs( string, paramStart, rA, rS, rB );
}

HANDLER_DECLARE XorImmediateFamily (HANDLER_PARAMS)
{
	UInt8		primaryOpcode = GET_OP_FLD(0,5);
	UInt8		rS,rA;
	UInt16		UIMM;
	
	rS = GET_OP_FLD(6,10);
	rA = GET_OP_FLD(11,15);
	UIMM = GET_OP_FLD(16,31);
	
	if( primaryOpcode == 27 )
		WRITE_STR( string, opcodeStart, "xoris", 5 );
	else
		WRITE_STR( string, opcodeStart, "xori", 4 );
	
	UInt8		offset = paramStart;
	offset = writeTwoRegs( string, offset, rA, rS );
	WRITE_CHAR( string, offset++, ',' );
	writeIMM( string, offset, UIMM );
}

#pragma mark -
#pragma mark Utility Functions
#pragma mark =============
#pragma mark Ê

static Boolean decodeTrapOperation( Int8 TO, ASCII8Str opcodeName, UInt8* opcodeNameLength )
{
	Boolean				identified = 1;
	
	switch( TO )
	{
		case 1:	WRITE_STR( opcodeName, 0, "twlgt", *opcodeNameLength = 5 );	break;
		case 2:	WRITE_STR( opcodeName, 0, "twllt", *opcodeNameLength = 5 );	break;
		case 4:	WRITE_STR( opcodeName, 0, "tweq", *opcodeNameLength = 4 );	break;
		case 5:	WRITE_STR( opcodeName, 0, "twlge", *opcodeNameLength = 5 );	break;
		case 6:	WRITE_STR( opcodeName, 0, "twlle", *opcodeNameLength = 5 );	break;
		case 8:	WRITE_STR( opcodeName, 0, "twgt", *opcodeNameLength = 4 );	break;
		case 12:	WRITE_STR( opcodeName, 0, "twge", *opcodeNameLength = 4 );	break;
		case 16:	WRITE_STR( opcodeName, 0, "twlt", *opcodeNameLength = 4 );	break;
		case 20:	WRITE_STR( opcodeName, 0, "twle", *opcodeNameLength = 4 );	break;
		case 24:	WRITE_STR( opcodeName, 0, "twne", *opcodeNameLength = 4 );	break;
		default:	WRITE_STR( opcodeName, 0, "tw", *opcodeNameLength = 2 );	identified = 0; break;
	}
	
	return identified;
}

static Boolean decodeSPR( Int16 spr, ASCII8Str sprName, UInt8* sprNameLength )
{
	Boolean				identified = 1;
	
	switch( spr )
	{
		case 1:	WRITE_STR( sprName, 0, "xer", *sprNameLength = 3 );	break;
		case 8:	WRITE_STR( sprName, 0, "lr", *sprNameLength = 2 );	break;
		case 9:	WRITE_STR( sprName, 0, "ctr", *sprNameLength = 3 );	break;
		case 18:	WRITE_STR( sprName, 0, "dsisr", *sprNameLength = 5 );	break;
		case 19:	WRITE_STR( sprName, 0, "dar", *sprNameLength = 3 );	break;
		case 22:	WRITE_STR( sprName, 0, "dec", *sprNameLength = 3 );	break;
		case 25:	WRITE_STR( sprName, 0, "sdr1", *sprNameLength = 4 );	break;
		case 26:	WRITE_STR( sprName, 0, "srr0", *sprNameLength = 4 );	break;
		case 27:	WRITE_STR( sprName, 0, "srr1", *sprNameLength = 4 );	break;
		case 272:	WRITE_STR( sprName, 0, "sprg0", *sprNameLength = 5 );	break;
		case 273:	WRITE_STR( sprName, 0, "sprg1", *sprNameLength = 5 );	break;
		case 274:	WRITE_STR( sprName, 0, "sprg2", *sprNameLength = 5 );	break;
		case 275:	WRITE_STR( sprName, 0, "sprg3", *sprNameLength = 5 );	break;
		case 280:	WRITE_STR( sprName, 0, "asr", *sprNameLength = 3 );	break;
		case 282:	WRITE_STR( sprName, 0, "ear", *sprNameLength = 3 );	break;
		case 287:	WRITE_STR( sprName, 0, "pvr", *sprNameLength = 3 );	break;
		case 528:	WRITE_STR( sprName, 0, "ibat0u", *sprNameLength = 6 );	break;
		case 529:	WRITE_STR( sprName, 0, "ibat0l", *sprNameLength = 6 );	break;
		case 530:	WRITE_STR( sprName, 0, "ibat1u", *sprNameLength = 6 );	break;
		case 531:	WRITE_STR( sprName, 0, "ibat1l", *sprNameLength = 6 );	break;
		case 532:	WRITE_STR( sprName, 0, "ibat2u", *sprNameLength = 6 );	break;
		case 533:	WRITE_STR( sprName, 0, "ibat2l", *sprNameLength = 6 );	break;
		case 534:	WRITE_STR( sprName, 0, "ibat3u", *sprNameLength = 6 );	break;
		case 535:	WRITE_STR( sprName, 0, "ibat3l", *sprNameLength = 6 );	break;
		case 536:	WRITE_STR( sprName, 0, "dbat0u", *sprNameLength = 6 );	break;
		case 537:	WRITE_STR( sprName, 0, "dbat0l", *sprNameLength = 6 );	break;
		case 538:	WRITE_STR( sprName, 0, "dbat1u", *sprNameLength = 6 );	break;
		case 539:	WRITE_STR( sprName, 0, "dbat1l", *sprNameLength = 6 );	break;
		case 540:	WRITE_STR( sprName, 0, "dbat2u", *sprNameLength = 6 );	break;
		case 541:	WRITE_STR( sprName, 0, "dbat2l", *sprNameLength = 6 );	break;
		case 542:	WRITE_STR( sprName, 0, "dbat3u", *sprNameLength = 6 );	break;
		case 543:	WRITE_STR( sprName, 0, "dbat3l", *sprNameLength = 6 );	break;
		case 1013:	WRITE_STR( sprName, 0, "dabr", *sprNameLength = 4 );	break;
		default:	WRITE_STR( sprName, 0, "spr", *sprNameLength = 3 );	identified = 0; break;
	}
	
	return identified;
}

static UInt8 writeFRegDReg( ASCII8Str string, UInt8 offset, UInt8 fpOne, Int16 d, UInt8 rTwo )
{
	offset = writeFPReg( string, offset, fpOne );
	WRITE_CHAR( string, offset++, ',' );
	if( d < 0 )
	{
		WRITE_CHAR( string, offset++, '-' );
		d = -d;
	}
	offset = writeIMM( string, offset, d );
	WRITE_CHAR( string, offset++, '(' );
	offset = writeReg( string, offset, rTwo );
	WRITE_CHAR( string, offset++, ')' );
	return offset;
}

static UInt8 writeFRegTwoRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 rTwo, UInt8 rThree )
{
	offset = writeFPReg( string, offset, fpOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeReg( string, offset, rTwo );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeReg( string, offset, rThree );
	return offset;
}

static UInt8 writeFourFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo, UInt8 fpThree, UInt8 fpFour )
{
	offset = writeFPReg( string, offset, fpOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpTwo );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpThree );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpFour );
	return offset;
}

static UInt8 writeThreeFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo, UInt8 fpThree )
{
	offset = writeFPReg( string, offset, fpOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpTwo );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpThree );
	return offset;
}

static UInt8 writeTwoFPRegs( ASCII8Str string, UInt8 offset, UInt8 fpOne, UInt8 fpTwo )
{
	offset = writeFPReg( string, offset, fpOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeFPReg( string, offset, fpTwo );
	return offset;
}

static UInt8 writeFPReg( ASCII8Str string, UInt8 offset, UInt8 reg )
{
	switch( reg )
	{
		default:		// I don't know of any special purpose fp registers (like r2 = RTOC, etc.). Do you?			// Nope
			WRITE_STR( string, offset, "fp", 2 );
			offset += 2;
			if( reg > 9 )
				WRITE_DIGIT( string, offset++, reg/10 );
			WRITE_DIGIT( string, offset++, reg % 10 );
			break;
	}
	
	return offset;
}

static UInt8 writeRegDReg( ASCII8Str string, UInt8 offset, UInt8 rOne, Int16 d, UInt8 rTwo )
{
	offset = writeReg( string, offset, rOne );
	WRITE_CHAR( string, offset++, ',' );
	if( d < 0 )
	{
		WRITE_CHAR( string, offset++, '-' );
		d = -d;
	}
	offset = writeIMM( string, offset, d );
	WRITE_CHAR( string, offset++, '(' );
	offset = writeReg( string, offset, rTwo );
	WRITE_CHAR( string, offset++, ')' );
	return offset;
}

static UInt8 writeThreeRegs( ASCII8Str string, UInt8 offset, UInt8 rOne, UInt8 rTwo, UInt8 rThree )
{
	offset = writeReg( string, offset, rOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeReg( string, offset, rTwo );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeReg( string, offset, rThree );
	return offset;
}

static UInt8 writeTwoRegs( ASCII8Str string, UInt8 offset, UInt8 rOne, UInt8 rTwo )
{
	offset = writeReg( string, offset, rOne );
	WRITE_CHAR( string, offset++, ',' );
	offset = writeReg( string, offset, rTwo );
	return offset;
}

static UInt8 writeReg( ASCII8Str string, UInt8 offset, UInt8 reg )
{
	switch( reg )
	{
		case 1:
			WRITE_STR( string, offset, "sp", 2 );
			offset += 2;
			break;
		case 2:
			WRITE_STR( string, offset, "rtoc", 4 );
			offset += 4;
			break;
		default:
			WRITE_CHAR( string, offset++, 'r' );
			if( reg > 9 )
				WRITE_DIGIT( string, offset++, reg/10 );
			WRITE_DIGIT( string, offset++, reg % 10 );
			break;
	}
	
	return offset;
}

static UInt8 writeIMM( ASCII8Str string, UInt8 offset, Int16 IMM )
{
	WRITE_STR(string,offset,"0x",2);
	offset += 2;
	WRITE_CHAR(string,offset++,Num2HexTab[(IMM>>12)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(IMM>>8)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(IMM>>4)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[IMM&0xF]);
	return offset;
}

static UInt8 writeAddress( ASCII8Str string, UInt8 offset, Int32 addr )
{
	WRITE_STR(string,offset,"0x",2);
	offset += 2;
	WRITE_CHAR(string,offset++,Num2HexTab[(addr>>20)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(addr>>16)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(addr>>12)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(addr>>8)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[(addr>>4)&0xF]);
	WRITE_CHAR(string,offset++,Num2HexTab[addr&0xF]);
	return offset;
}

static void memCopy( Int8* dst, Int8* src, UInt32 len )
{
	// This routine is absolutely inefficient, but optimizations are hardware-dependent
	while( len-- > 0 )
		*dst++ = *src++;
}