/*
	Benchmarks.cp
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
	Terry Greeniaus	-	Monday, 3 August 98	-	Original creation of file
*/
#include "Assembly.h"
#include "BenchMarks.h"
#include "VIA Chip.h"

__asm__ void	BenchMarkDecrementor(register VIA_Chip* via,register UInt32* deltaTime,register UInt16 numViaClocks)
{
	// Set up the auxiliary control register
	lbz	r0,VIA_Chip.auxillaryControl(r3);
	eieio;
	li	r7,0x0020;
	stb	r7,VIA_Chip.auxillaryControl(r3);
	eieio;
	
	// Disable all interrupts
	lbz	r7,VIA_Chip.interruptEnable(r3);
	eieio;
	slwi(r0,r0,8)
	or	r0,r7,r0;
	li	r7,0x7F;
	stb	r7,VIA_Chip.interruptEnable(r3);
	eieio;
	
	// r0 contains:	(VIA_Chip.auxillaryControl << 8) | (VIA_Chip.interruptEnable)
	
	// Load the VIA T2 timer with a countdown value
	stb	r5,VIA_Chip.timer2CounterLow(r3);
	eieio;
	lbz	r7,VIA_Chip.timer2CounterLow(r3);
	eieio;
	srwi(r7,r5,8);
	stb	r7,VIA_Chip.timer2CounterHigh(r3);
	eieio;
	lbz	r7,VIA_Chip.timer2CounterHigh(r3);
	eieio;
	
	// Start the decrementor
	li		r6,-1;
	mtdec(r6);
	
	@waitForInterruptNon601:
		lbz		r8,VIA_Chip.interruptFlag(r3);
		eieio;
		andi.		r8,r8,0x0020;
		beq+		@waitForInterruptNon601;
	
	// Stop the decrementor
	mfdec(r6);
	
	// The one's complement of the decrementor contains the elapsed time!
	not		r6,r6;
	stw		r6,0(r4);
	
	stb		r0,VIA_Chip.interruptEnable(r3);
	eieio;
	srwi(r0,r0,8);
	stb		r0,VIA_Chip.auxillaryControl(r3);
	eieio;
	
	blr;
}

__asm__ void	BenchMarkBDNZ(register UInt32* deltaTime,register UInt32 numBranches)
{
	mtctr	r4;
	
	// Start the decrementor
	li		r0,-1;
	mtdec(r0);
	
@loop:
	bdnz+	@loop;
	
	// Stop the decrementor
	mfdec(r4);
	
	// The one's complement of the decrementor contains the elapsed time!
	not		r4,r4;
	stw		r4,0(r3);
	
	blr;
}

__asm__ void BenchMarkSystemBus(register UInt32* deltaTime,register void* cacheInhibitedWordAddr,register UInt32 numLoads)
{
	srwi(r5,r5,2);
	mtctr	r5;
	
	// Start the decrementor
	li		r0,-1;
	mtdec(r0);
	
@loop:
	eieio;
	lwz		r0,0(r4);
	eieio;
	lwz		r8,12(r4);
	eieio;
	lwz		r9,4(r4);
	eieio;
	lwz		r10,8(r4);
	eieio;
	bdnz+	@loop;
	
	// Stop the decrementor
	mfdec(r4);
	
	// The one's complement of the decrementor contains the elapsed time!
	not		r4,r4;
	stw		r4,0(r3);
	
	blr;
}
