/*
	NKOurInterruptVectors.cp
	Copyright © 1999 by Patrick Varilly

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
	Patrick Varilly		-	Sunday, 28 March 99	-	Created file
*/

#include "Kernel Types.h"
#include "Assembly.h"
#include "NKOurInterruptVectors.h"
#include "NKVirtualMemory.h"
#include "NKMachineInit.h"

extern UInt32						VectorTable[48][2];

void NKOurInterruptVectorsInit(void)
{
	// Map first few K with BAT2; this is just to write to the area. It will never be accessed again, so it doesn't
	// matter if NKInitVirtualMemory zaps the mapping later (which it will).
	// We can be sure BAT2 will not be in the kernel code because BAT2 is used to map everything that is taken out
	// of kernel end.  However, we must remap it to what it was
	//UInt32					bat2up, bat2down;
	//NKSaveBATMap( 2, bat2up, bat2down );
	//NKBATMap( 1, nil, nil, 48*0x100, WIMG_COHERENT, PP_READ_WRITE );
	
	// For each interrupt handler, we build a pseudo-handler which dispatches the interrupt to what's in the VectorTable
	UInt32	pvr = (_getPVR() >> 16);
	if(pvr == cpu750 || pvr == cpu7400)	// G3 or G4, special handlers for L2 test support
	{
		for(UInt32 i = 0; i < 48; i++ )
		{
			// Each interrupt handler is called with the following state:
			//	sprg0	=	pointer to processor's current Thread
			//	sprg1-3	=	scratch
			// We must change this to:
			//	sprg0	=	pointer to processor's current Thread (remains the same)
			//	sprg1	=	original RTOC
			//	sprg2	=	original lr
			//	sprg3	=	scratch
			//	rtoc		=	pointer to handler's handlerScratch area (stored in VectorTable)
			//	lr		=	return address to here
			// Everything else *must* be preserved.  The code is a slight adaptation of that found in
			// NKROMInterruptVectors.cp (because the machine state is different and we have three scratch sprgs)
			UInt32*		instructions = (UInt32*)(i*0x0100);
			*instructions++ = MTSPR(SPRG1,2);		// mtspr		sprg1,r2
			*instructions++ = MTSPR(SPRG2,3);		// mtspr		sprg2,r3
			*instructions++ = MFSPR(2,L2CR);			// mfspr		r2,l2cr
			*instructions++ = RLWINM(3,2,14,31,31);	// rlwinm		r3,r2,14,31,31	-> L2TS bit
			*instructions++ = RLWINM(2,2,1,31,31);		// rlwinm		r2,r2,1,31,31		-> L2E bit
			*instructions++ = AND(2,2,3);				// and		r2,r2,r3
			*instructions++ = MULLI(2,2,-1);			// mulli		r2,r2,-1		-> 0 if L2TS == 0, 0xFFFFFFFF is L2TS == 1
			*instructions++ = MFSPR(3,L2CR);			// mfspr		r3,l2cr
			*instructions++ = ANDC(2,3,2);			// andc		r2,r3,r2
			*instructions++ = MTSPR(L2CR,2);			// mtspr		l2cr,r2
			*instructions++ = MFSPR(3,SPRG2);		// mfspr		r3,sprg2
			*instructions++ = MFLR(2);
			*instructions++ = MTSPR(SPRG2,2);
			*instructions++ = MFSPR(2,SPRG3);
			*instructions++ = LIS(2,(((UInt32)VectorTable >> 16) & 0xFFFF));
			*instructions++ = ORI(2,2,((UInt32)VectorTable & 0xFFFF));
			*instructions++ = LWZ(2,i*8,2);
			*instructions++ = MTLR(2);
			*instructions++ = LIS(2,(((UInt32)VectorTable >> 16) & 0xFFFF));
			*instructions++ = ORI(2,2,((UInt32)VectorTable & 0xFFFF));
			*instructions++ = LWZ(2,i*8 + 4,2);
			*instructions++ = BLRL;
			*instructions++ = MFSPR(2,SPRG2);
			*instructions++ = MTLR(2);
			*instructions++ = MFSPR(2,SPRG1);
			*instructions = RFI;
		}
	}
	else
	{
		for(UInt32 i = 0; i < 48; i++ )
		{
			// Each interrupt handler is called with the following state:
			//	sprg0	=	pointer to processor's current Thread
			//	sprg1-3	=	scratch
			// We must change this to:
			//	sprg0	=	pointer to processor's current Thread (remains the same)
			//	sprg1	=	original RTOC
			//	sprg2	=	original lr
			//	sprg3	=	scratch
			//	rtoc		=	pointer to handler's handlerScratch area (stored in VectorTable)
			//	lr		=	return address to here
			// Everything else *must* be preserved.  The code is a slight adaptation of that found in
			// NKROMInterruptVectors.cp (because the machine state is different and we have three scratch sprgs)
			UInt32				*instructions = (UInt32*)(i*0x100);
			*instructions++ = MTSPR(SPRG1,2);
			*instructions++ = MFLR(2);
			*instructions++ = MTSPR(SPRG2,2);
			*instructions++ = MFSPR(2,SPRG3);
			*instructions++ = LIS(2,(((UInt32)VectorTable >> 16) & 0xFFFF));
			*instructions++ = ORI(2,2,((UInt32)VectorTable & 0xFFFF));
			*instructions++ = LWZ(2,i*8,2);
			*instructions++ = MTLR(2);
			*instructions++ = LIS(2,(((UInt32)VectorTable >> 16) & 0xFFFF));
			*instructions++ = ORI(2,2,((UInt32)VectorTable & 0xFFFF));
			*instructions++ = LWZ(2,i*8 + 4,2);
			*instructions++ = BLRL;
			*instructions++ = MFSPR(2,SPRG2);
			*instructions++ = MTLR(2);
			*instructions++ = MFSPR(2,SPRG1);
			*instructions = RFI;
		}
	}
	// Flush the caches to make sure data goes
	NKFlushCaches( nil, 48*0x100 );
	
	// Restore BAT Mapping
	//NKSetBATMap( 2, bat2up, bat2down );
}

void NKOurInterruptVectorsInitOnThisProcessor(void)
{
	// Enable machine check exceptions and also set interrupt prefix to vector through nil.
	SetMSR((GetMSR() | 0x00001000) & ~0x00000040);
}