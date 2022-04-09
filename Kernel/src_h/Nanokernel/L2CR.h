/*
	L2CR.h
	Copyright © 1997-1998 by PowerLogix R & D, Inc.
	
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
	Author:	Terry Greeniaus (tgree@phys.ualberta.ca,tmg@gpu.srv.ualberta.ca)
	Please e-mail updates to this file to me, thanks!
	
	Best viewed in Geneva 9-pt.  Sorry!
	
	History
	***********
		Thur, Dec. 12, 1998.
		- Terry: First public release, contributed by PowerLogix.
	***********
		Sat, Aug. 7, 1999.
		- Terry: Added some more L2 bits.
	***********
	
*/
#ifndef __L2CR__
#define __L2CR__

enum
{
	// L2 Enable
	L2E			=	0x80000000,
	
	// L2 Parity Enable
	L2PE			=	0x40000000,
	
	// L2 Size
	L2SIZ_MASK	=	0x30000000,
	L2SIZ_256K	=	0x10000000,
	L2SIZ_512K	=	0x20000000,
	L2SIZ_1024K	=	0x30000000,
	L2SIZ_2048K	=	0x00000000,
	
	// L2 Clock
	L2CLK_MASK	=	0x0E000000,
	L2CLK_1_0	=	0x02000000,
	L2CLK_1_5	=	0x04000000,
	L2CLK_2_0	=	0x08000000,
	L2CLK_2_5	=	0x0A000000,
	L2CLK_3_0	=	0x0C000000,
	L2CLK_3_5	=	0x06000000,
	L2CLK_4_0	=	0x0E000000,
	
	// L2 Ram
	L2RAM_MASK	=	0x01800000,
	L2RAM_FLOW	=	0x00000000,
	L2RAM_SYNC	=	0x01000000,
	L2RAM_LATE	=	0x01800000,
	
	// L2 Data only
	L2DO			=	0x00400000,
	
	// L2 Invalidate
	L2I			=	0x00200000,
	
	// L2 ZZ Control
	L2CTL		=	0x00100000,
	
	// L2 Write-thru
	L2WT		=	0x00080000,
	
	// L2 Test support
	L2TS			=	0x00040000,
	
	// L2 Output Hold
	L2OH_MASK	=	0x00030000,
	L2OH_0_5		=	0x00000000,
	L2OH_1_0		=	0x00010000,
	L2OH_1_5		=	0x00020000,
	L2OH_2_0		=	0x00030000,
	
	// L2 Slow
	L2SL			=	0x00008000,
	
	// L2 Differential
	L2DF			=	0x00004000,
	
	// L2 Bypass
	L2BYP		=	0x00002000,
	
	// L2 Instruction only
	L2IO			=	0x00000400,
	
	// L2 Configuration mask (everything but size, speed. Includes: parity, SDRAM type, output hold, slow, differential)
	L2CONFIG_MASK	=	0x4183C000
};

#ifdef __cplusplus
extern "C" {
#endif

UInt32 __setL2CR(register UInt32 newL2CR);	/* Sets L2CR to newL2CR, returns L2CR contents.  Returns -1 on a non-750 chip */
UInt32 __getL2CR(void);					/* Returns L2CR if this is a 750 chip, 0 otherwise */

#ifdef __cplusplus
}
#endif

#endif
