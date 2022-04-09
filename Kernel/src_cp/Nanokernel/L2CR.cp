/*
	L2CR.c
	Copyright © 1997-1999 by PowerLogix R & D, Inc.
	
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
	Author:	Terry Greeniaus (tgree@phys.ualberta.ca, tmg@gpu.srv.ualberta.ca)
	Please e-mail me updates to this file, thanks!
	
	Best viewed in Geneva 9-pt.  Sorry!
	
	History
	***********
		Thur, Dec. 12, 1998.
		- Terry: First public release, contributed by PowerLogix.
	***********
		Sat, Aug. 7, 1999.
		- Terry: Made sure code disabled interrupts before running. (Previously
				it was assumed interrupts were already disabled).
		- Terry: Updated for tentative G4 support.  4MB of memory is now flushed
				instead of 2MB.  (Prob. only 3 is necessary).
		- Terry: Updated for workaround to HID0[DPM] processor bug
				during global invalidates.
	***********
*/
#include "L2CR.h"

asm UInt32 __setL2CR(register UInt32 applyThis)
{
	/* Usage:
	
		When setting the L2CR register, you must do a few special things.  If you are enabling the
		cache, you must perform a global invalidate.  If you are disabling the cache, you must
		flush the cache contents first.  This routine takes care of doing these things.  When first
		enabling the cache, make sure you pass in the L2CR you want, as well as passing in the
		global invalidate bit set.  A global invalidate will only be performed if the L2I bit is set
		in applyThis.  When enabling the cache, you should also set the L2E bit in applyThis.  If you
		want to modify the L2CR contents after the cache has been enabled, the recommended
		procedure is to first call __setL2CR(0) to disable the cache and then call it again with
		the new values for L2CR.  Examples:
	
			__setL2CR(0)				-	disables the cache
			__setL2CR(0xB3A04000)		-	enables my G3 upgrade card:
										-	L2E set to turn on the cache
										-	L2SIZ set to 1MB
										-	L2CLK set to 1:1
										-	L2RAM set to pipelined syncronous late-write
										-	L2I set to perform a global invalidation
										-	L2OH set to 0.5 nS
										-	L2DF set because this upgrade card requires it
	
		A similar call should work for your card.  You need to know the correct setting for your
		card and then place them in the fields I have outlined above.  Other fields support optional
		features, such as L2DO which caches only data, or L2TS which causes cache pushes from
		the L1 cache to go to the L2 cache instead of to main memory.
	*/
	
	/* Make sure this is has an L2CR register */
	mfspr	r4,287;
	rlwinm	r4,r4,16,16,31;
	cmplwi	r4,0x0008;	/* 750 has L2CR */
	beq		@hasL2CR;
	cmplwi	r4,0x000C;	/* G4 has L2CR */
	beq		@hasL2CR;
	li		r3,-1;
	blr;
	
@hasL2CR:
	/* Turn off interrupts and data relocation. */
	mfmsr	r7;	/* Save MSR in r7 */
	rlwinm	r4,r7,0,17,15;	/* Turn off EE bit - an external exception while we are flushing the cache is fatal (comment this line and see!) */
	rlwinm	r4,r4,0,28,26;	/* Turn off DR bit */
	sync;
	mtmsr	r4;
	sync;
	
	/* Get the current enable bit of the L2CR into r4 */
	mfspr	r4,1017;
	rlwinm	r4,r4,0,0,0;
	
	/* See if we want to perform a global inval this time. */
	rlwinm	r6,r3,0,10,10;	/* r6 contains the new invalidate bit */
	rlwinm.	r5,r3,0,0,0;		/* r5 contains the new enable bit */
	rlwinm	r3,r3,0,11,9;		/* Turn off the invalidate bit */
	rlwinm	r3,r3,0,1,31;		/* Turn off the enable bit */
	or		r3,r3,r4;			/* Keep the enable bit the same as it was for now. */
	bne		@dontDisableCache	/* Only disable the cache if L2CRApply has the enable bit off */

@disableCache:
	/*
		Disable the cache. First, read the first 4MB of memory (physical) to put new data in the cache.  (Actually we only need
		the size of the L2 cache plus the size of the L1 cache, but 4MB will cover everything just to be safe).
		
		*** Might be a good idea to set L2DO here - to prevent instructions from getting into the cache.  But since we invalidate
			the next time we enable the cache it doesn't really matter ***
	*/
	lis		r4,0x0002;
	mtctr	r4;
	li		r4,0;
	@loadLoop:
		lwzx		r0,r0,r4;
		addi		r4,r4,32;	/* Go to start of next cache line */
		bdnz		@loadLoop;
	
	/* Now, flush the first 4MB of memory */
	lis		r4,0x0002;
	mtctr	r4;
	li		r4,0;
	sync;
	@flushLoop:
		dcbf		r0,r4;
		addi		r4,r4,32;	/* Go to start of next cache line */
		bdnz		@flushLoop;
	
	/* Turn off the L2CR enable bit. */
	rlwinm	r3,r3,0,1,31;
	
@dontDisableCache:
	/* Set up the L2CR configuration bits */
	sync;
	mtspr	1017,r3;
	sync;
	cmplwi	r6,0;
	beq		@noInval;
	
	/*
		Before we perform the global invalidation, we must disable dynamic power management via HID0[DPM]
		to work around a processor bug where DPM can possibly interfere with the state machine in the processor
		that invalidates the L2 cache tags.
	*/
	mfspr	r8,1008;			/* Save HID0 in r8 */
	rlwinm	r4,r8,0,12,10;	/* Turn off HID0[DPM] */
	sync;
	mtspr	1008,r4;			/* Disable DPM */
	sync;
	
	/* Perform a global invalidation */
	oris		r3,r3,0x0020;
	sync;
	mtspr	1017,r3;
	isync;	/* For errata #9 on G4 */
	sync;
@invalCompleteLoop:			/* Wait for the invalidation to complete */
		mfspr	r3,1017;
		rlwinm.	r4,r3,0,31,31;
		bne		@invalCompleteLoop;
	
	rlwinm	r3,r3,0,11,9;	/* Turn off the L2I bit */
	sync
	mtspr	1017,r3;
	sync
	
	/* Restore HID0[DPM] to whatever it was before */
	sync;
	mtspr	1008,r8;
	sync;
	
@noInval:
	/* See if we need to enable the cache */
	cmplwi	r5,0;
	beq		@done;
	
@enableCache:
	/* Enable the cache */
	oris		r3,r3,0x8000;
	mtspr	1017,r3;
	sync;

@done:
	/* Restore MSR (restores EE and DR bits to original state) */
	sync;
	mtmsr	r7;
	sync;
	
	blr;
}

asm UInt32 __getL2CR(void)
{
	/* Make sure this has an L2CR */
	mfspr	r3,287;
	rlwinm	r3,r3,16,16,31;
	cmplwi	r3,0x0008;	/* 750 chip */
	beq		@hasL2CR;
	cmplwi	r3,0x000C;	/* G4 chip */
	beq		@hasL2CR;
	li		r3,0;
	bnelr;
	
	/* Return the L2CR contents */
@hasL2CR:
	mfspr	r3,1017;
	blr;
}
