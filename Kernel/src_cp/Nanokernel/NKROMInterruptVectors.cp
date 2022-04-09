/*
	NKROMInterruptVectors.cp
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
	Terry Greeniaus	-	Monday, 24 August 98	-	Re-worked code so that sprg0 is now reserved (contains a current Thread pointer for the processor,
												looking ahead to multi-processing support).  This required some pretty complicated stuff - now we have NO
												scratch sprg registers at interrupt time!!!!!
*/
#include "Assembly.h"
#include "Macros.h"
#include "NKROMInterruptVectors.h"
#include "NKInterruptVectors.h"

static void NKROMHandler0(void);
static void NKROMHandler1(void);
static void NKROMHandler2(void);
static void NKROMHandler3(void);
static void NKROMHandler4(void);
static void NKROMHandler5(void);
static void NKROMHandler6(void);
static void NKROMHandler7(void);
static void NKROMHandler8(void);
static void NKROMHandler9(void);
static void NKROMHandler10(void);
static void NKROMHandler11(void);
static void NKROMHandler12(void);
static void NKROMHandler13(void);
static void NKROMHandler14(void);
static void NKROMHandler15(void);
static void NKROMHandler16(void);
static void NKROMHandler17(void);
static void NKROMHandler18(void);
static void NKROMHandler19(void);
static void NKROMHandler20(void);
static void NKROMHandler21(void);
static void NKROMHandler22(void);
static void NKROMHandler23(void);
static void NKROMHandler24(void);
static void NKROMHandler25(void);
static void NKROMHandler26(void);
static void NKROMHandler27(void);
static void NKROMHandler28(void);
static void NKROMHandler29(void);
static void NKROMHandler30(void);
static void NKROMHandler31(void);
static void NKROMHandler32(void);
static void NKROMHandler33(void);
static void NKROMHandler34(void);
static void NKROMHandler35(void);
static void NKROMHandler36(void);
static void NKROMHandler37(void);
static void NKROMHandler38(void);
static void NKROMHandler39(void);
static void NKROMHandler40(void);
static void NKROMHandler41(void);
static void NKROMHandler42(void);
static void NKROMHandler43(void);
static void NKROMHandler44(void);
static void NKROMHandler45(void);
static void NKROMHandler46(void);
static void NKROMHandler47(void);

static UInt32	glueAddr[49];

void NKROMInterruptVectorsInit(void)
{
	glueAddr[0] = _getRTOC();					// Vector 0 is actually our RTOC
	glueAddr[1] = FUNC_ADDR(NKROMHandler0);		// The ROM vector table will be &glueAddr[1], which is stored in sprg3
	glueAddr[2] = FUNC_ADDR(NKROMHandler1);
	glueAddr[3] = FUNC_ADDR(NKROMHandler2);
	glueAddr[4] = FUNC_ADDR(NKROMHandler3);
	glueAddr[5] = FUNC_ADDR(NKROMHandler4);
	glueAddr[6] = FUNC_ADDR(NKROMHandler5);
	glueAddr[7] = FUNC_ADDR(NKROMHandler6);
	glueAddr[8] = FUNC_ADDR(NKROMHandler7);
	glueAddr[9] = FUNC_ADDR(NKROMHandler8);
	glueAddr[10] = FUNC_ADDR(NKROMHandler9);
	glueAddr[11] = FUNC_ADDR(NKROMHandler10);
	glueAddr[12] = FUNC_ADDR(NKROMHandler11);
	glueAddr[13] = FUNC_ADDR(NKROMHandler12);
	glueAddr[14] = FUNC_ADDR(NKROMHandler13);
	glueAddr[15] = FUNC_ADDR(NKROMHandler14);
	glueAddr[16] = FUNC_ADDR(NKROMHandler15);
	glueAddr[17] = FUNC_ADDR(NKROMHandler16);
	glueAddr[18] = FUNC_ADDR(NKROMHandler17);
	glueAddr[19] = FUNC_ADDR(NKROMHandler18);
	glueAddr[20] = FUNC_ADDR(NKROMHandler19);
	glueAddr[21] = FUNC_ADDR(NKROMHandler20);
	glueAddr[22] = FUNC_ADDR(NKROMHandler21);
	glueAddr[23] = FUNC_ADDR(NKROMHandler22);
	glueAddr[24] = FUNC_ADDR(NKROMHandler23);
	glueAddr[25] = FUNC_ADDR(NKROMHandler24);
	glueAddr[26] = FUNC_ADDR(NKROMHandler25);
	glueAddr[27] = FUNC_ADDR(NKROMHandler26);
	glueAddr[28] = FUNC_ADDR(NKROMHandler27);
	glueAddr[29] = FUNC_ADDR(NKROMHandler28);
	glueAddr[30] = FUNC_ADDR(NKROMHandler29);
	glueAddr[31] = FUNC_ADDR(NKROMHandler30);
	glueAddr[32] = FUNC_ADDR(NKROMHandler31);
	glueAddr[33] = FUNC_ADDR(NKROMHandler32);
	glueAddr[34] = FUNC_ADDR(NKROMHandler33);
	glueAddr[35] = FUNC_ADDR(NKROMHandler34);
	glueAddr[36] = FUNC_ADDR(NKROMHandler35);
	glueAddr[37] = FUNC_ADDR(NKROMHandler36);
	glueAddr[38] = FUNC_ADDR(NKROMHandler37);
	glueAddr[39] = FUNC_ADDR(NKROMHandler38);
	glueAddr[40] = FUNC_ADDR(NKROMHandler39);
	glueAddr[41] = FUNC_ADDR(NKROMHandler40);
	glueAddr[42] = FUNC_ADDR(NKROMHandler41);
	glueAddr[43] = FUNC_ADDR(NKROMHandler42);
	glueAddr[44] = FUNC_ADDR(NKROMHandler43);
	glueAddr[45] = FUNC_ADDR(NKROMHandler44);
	glueAddr[46] = FUNC_ADDR(NKROMHandler45);
	glueAddr[47] = FUNC_ADDR(NKROMHandler46);
	glueAddr[48] = FUNC_ADDR(NKROMHandler47);
}

void NKROMInterruptVectorsInitOnThisProcessor(void)
{
	// Trick ROM into using our vector table by using an interrupt table pointer stored in SPRG3
	_setSPRG3((UInt32)&glueAddr[1]);
	
	// Enable machine check exceptions and also set interrupt prefix to vector through ROM.
	SetMSR(GetMSR() | 0x00001040);
}

/*
	// This gets called when ROM finds our vector.  Machine state is as follows:
	//	sprg0	=	pointer to processor's current Thread
	//	sprg1	=	original sp
	//	sprg2	=	original lr
	//	sprg3	=	&glueAddr[1]
	//	sp		=	address of this code
	//	lr		=	address of the identifier from ROM (in new ROMs - garbage in old ROMs)
	//	RTOC	=	original RTOC
	//
	// We need to set up the machine state so that it is as follows:
	//	sprg0	=	pointer to processor's current Thread
	//	sprg1	=	original RTOC
	//	sprg2	=	original lr
	//	sprg3	=	reserved
	//	rtoc		=	interrupt handler's RTOC (from VectorTable)
	//	lr		=	a return address to here
	//	sp		=	original sp
*/

#define	NK_ROM_GLUE(vector)	mfsprg(sp,1);						\
							mtsprg(1,RTOC);					\
							mfsprg(RTOC,3);					\
							lwz		RTOC,-4(RTOC)	;			\
							lwz		RTOC,VectorTable(RTOC);		\
							lwz		RTOC,(vector*8)(RTOC);		\
							mtlr		RTOC;					\
							mfsprg(RTOC,3);					\
							lwz		RTOC,-4(RTOC);			\
							lwz		RTOC,VectorTable(RTOC);		\
							lwz		RTOC,(vector*8 + 4)(RTOC);	\
							blrl;								\
							mfsprg(RTOC,2);					\
							mtlr		RTOC;					\
							mfsprg(RTOC,1);					\
							rfi
							
#define	NK_ROM_HANDLER(vector)	__asm__ void NKROMHandler ## vector(void) {NK_ROM_GLUE(vector);}

NK_ROM_HANDLER(0);
NK_ROM_HANDLER(1);
NK_ROM_HANDLER(2);
NK_ROM_HANDLER(3);
NK_ROM_HANDLER(4);
NK_ROM_HANDLER(5);
NK_ROM_HANDLER(6);
NK_ROM_HANDLER(7);
NK_ROM_HANDLER(8);
NK_ROM_HANDLER(9);
NK_ROM_HANDLER(10);
NK_ROM_HANDLER(11);
NK_ROM_HANDLER(12);
NK_ROM_HANDLER(13);
NK_ROM_HANDLER(14);
NK_ROM_HANDLER(15);
NK_ROM_HANDLER(16);
NK_ROM_HANDLER(17);
NK_ROM_HANDLER(18);
NK_ROM_HANDLER(19);
NK_ROM_HANDLER(20);
NK_ROM_HANDLER(21);
NK_ROM_HANDLER(22);
NK_ROM_HANDLER(23);
NK_ROM_HANDLER(24);
NK_ROM_HANDLER(25);
NK_ROM_HANDLER(26);
NK_ROM_HANDLER(27);
NK_ROM_HANDLER(28);
NK_ROM_HANDLER(29);
NK_ROM_HANDLER(30);
NK_ROM_HANDLER(31);
NK_ROM_HANDLER(32);
NK_ROM_HANDLER(33);
NK_ROM_HANDLER(34);
NK_ROM_HANDLER(35);
NK_ROM_HANDLER(36);
NK_ROM_HANDLER(37);
NK_ROM_HANDLER(38);
NK_ROM_HANDLER(39);
NK_ROM_HANDLER(40);
NK_ROM_HANDLER(41);
NK_ROM_HANDLER(42);
NK_ROM_HANDLER(43);
NK_ROM_HANDLER(44);
NK_ROM_HANDLER(45);
NK_ROM_HANDLER(46);
NK_ROM_HANDLER(47);