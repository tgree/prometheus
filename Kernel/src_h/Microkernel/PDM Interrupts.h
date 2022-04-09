/*
	Driver.h
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
#ifndef __PDM__INTERRUPTS__
#define __PDM__INTERRUPTS__

#include "External Interrupt Internals.h"
#include "VIA Chip.h"

typedef struct VIA2_Chip
{
	const UInt8	reserved1[0x02];		// Offset 0x00
	UReg8		slotInterruptFlag;		// Offset 0x02
	UReg8		interruptFlag;			// Offset 0x03
	const UInt8	reserved2[0x0E];		// Offset 0x04
	UReg8		slotInterruptEnable;		// Offset 0x12
	UReg8		interruptEnable;		// Offset 0x13
} VIA2_Chip;

NKExceptionHandler InitPDMInterrupts(void);
Boolean registerPDMInterrupt( InterruptHandler *handler, Int32 interruptType );
Boolean deregisterPDMInterrupt( Int32 interruptType );

#endif /* !__PDM__INTERRUPTS__ */