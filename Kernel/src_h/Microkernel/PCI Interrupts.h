/*
	PCI Interrupts.h
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
#ifndef __PCI__INTERRUPTS__
#define __PCI__INTERRUPTS__

#include "External Interrupt Internals.h"
#include "VIA Chip.h"

typedef struct PCI_Interrupt_Controller
{
	UReg32LE	events;
	UReg32LE	mask;
	UReg32LE	clear;
	UReg32LE	levels;
}PCI_Interrupt_Controller;

NKExceptionHandler InitPCIInterrupts(void);
Boolean registerPCIInterrupt( InterruptHandler *handler, Int32 interruptType );
Boolean deregisterPCIInterrupt( Int32 interruptType );
void pendingInterrupts(UInt32* intHIgh,UInt32* intLow);
void enableInterrupts(UInt32 intHigh,UInt32 intLow);

#endif /* !__PCI__INTERRUPTS__ */