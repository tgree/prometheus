/*
	PDM Interrupts.cp
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
	Patrick Varilly			January, 1998			Original creation of file
	Patrick Varilly			Wed, 21 Jan 98			Original history tagging of file. Total restructing of interrupt
		---> system. Note that this change is completely transparent to outside software (except for perhaps a minor
		---> performance improvement).
	Terry Greeniaus		March 18, 1998		Fixed PDMVIA2Handler() to search the PDMVIA2Interrupts table
		---> instead of PDMVIA1Interrupts.  SCSI should work now!!!
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 26 June 98	-	Changed for OpenFirmware interrupts (negligible changes)
*/
#include "Kernel Types.h"
#include "Assembly.h"
#include "NKMachineInit.h"
#include "NKInterruptVectors.h"
#include "NKVideo.h"
#include "NKVirtualMemory.h"
#include "Streams.h"
#include "External Interrupt.h"
#include "External Interrupt Internals.h"
#include "PDM Interrupts.h"
#include "Machine Registers.h"

static HandlerTable			PDMVIA1Interrupts[7] = {
	{ nil, PMAC_DEV_NO_INT },				/* Cascade */
	{ nil, PMAC_DEV_HZTICK },
	{ nil, PMAC_DEV_CUDA },		
	{ nil, PMAC_DEV_NO_INT },				/* VIA Data */
	{ nil,PMAC_DEV_NO_INT },				/* VIA CLK Source */
	{ nil, PMAC_DEV_TIMER2 },
	{ nil, PMAC_DEV_TIMER1 }
};

static HandlerTable			PDMVIA2Interrupts[7] = {
	{ nil, PMAC_DMA_SCSI0 },				/* SCSI A DMA IRQ (not used) */
	{ nil, PMAC_DEV_NO_INT },				/* Any slot interrupt */
							// Must be initialized to PDMSlotHandler for slot-caused interrupts to work
	{ nil, PMAC_DMA_SCSI1 },				/* SCSI B DMA IRQ (not used) */
	{ nil, PMAC_DEV_SCSI0 },
	{ nil, PMAC_DEV_NO_INT },				/* Reserved */
	{ nil, PMAC_DEV_FLOPPY },
	{ nil, PMAC_DEV_SCSI1 }
};


static HandlerTable			PDMDMAInterrupts[7] = {
	{ nil, PMAC_DMA_SCC_B_RX },
	{ nil, PMAC_DMA_SCC_B_TX },
	{ nil, PMAC_DMA_SCC_A_RX },
	{ nil, PMAC_DMA_SCC_A_TX },
	{ nil, PMAC_DMA_ETHERNET_RX },
	{ nil, PMAC_DMA_ETHERNET_TX },
	{ nil, PMAC_DMA_FLOPPY }
};

static HandlerTable			PDMSlotInterrupts[7] = {
	{ nil, PMAC_DEV_NO_INT },				/* Unused */
	{ nil, PMAC_DEV_NO_INT },				/* Unused */
	{ nil, PMAC_DEV_NUBUS3 },	/* Slot 3 (stranging ordering here..) */
	{ nil, PMAC_DEV_NUBUS0 },	/* Slot 0 */
	{ nil, PMAC_DEV_NUBUS1 },	/* Slot 1 */
	{ nil, PMAC_DEV_PDS },	/* Slot 2 (PDS)*/
	{ nil, PMAC_DEV_VBL }	/* Video Interrupt */
};

static HandlerTable			PDMICRInterrupts[8] = {
	{ nil, PMAC_DEV_NO_INT },				/* Cascade Interrupts */
							// Must be initialized to PDMVIA1Handler for VIA1 interrupts to work
	{ nil, PMAC_DEV_NO_INT },
							// Must be initialized to PDMVIA2Handler for VIA2 interrupts to work
	{ nil, PMAC_DEV_SCC },
	{ nil, PMAC_DEV_ETHERNET },
	{ nil, PMAC_DEV_NO_INT },				/* PDM DMA Interrupt */
							// Must be initialized to PDMDMAHandler for DMA interrupts to work
	{ nil, PMAC_DEV_NMI },
	{ nil, PMAC_DEV_NO_INT },				/* INT Mode bit */
	{ nil, PMAC_DEV_NO_INT }				/* ACK Bit */
};

static Boolean PDMInterruptHandler( PPCRegisters* theUserRegs );
static void PDMDMAHandler( void );
static void PDMSlotHandler( void );
static void PDMVIA1Handler( void );
static void PDMVIA2Handler( void );

static Reg8*		interruptControl;
static Reg8*		dmaInterruptFlag;

NKExceptionHandler InitPDMInterrupts(void)
{
	// Terry: How come you map byte-sized registers with a size of a page? Most of the time (unless the register address
	// happens to be page-aligned) this will use up *two* pages, not one!!!
	// Terry says: OOOOPS!
	/* Set up register addresses */
	interruptControl = (Reg8*)NKIOMap((void*)PDM_IO_PHYS_ADDR(PDM_ICR),sizeof(Reg8),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	dmaInterruptFlag = (Reg8*)NKIOMap((void*)PDM_IO_PHYS_ADDR(PDM_DMA_IFR),sizeof(Reg8),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	machine.viaDevice0.logicalAddr = (VIA_Chip*)NKIOMap(machine.viaDevice0.physicalAddr,machine.viaDevice0.len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	machine.viaDevice1.logicalAddr = (VIA2_Chip*)NKIOMap(machine.viaDevice1.physicalAddr,machine.viaDevice1.len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	/* Reset all interrupt flags */
	*interruptControl = 0x80; _eieio();		// Set ACK bit
	
	machine.viaDevice0.logicalAddr->peripheralControl = 0x00; _eieio();	// Disable all VIA1 interrupts
	machine.viaDevice0.logicalAddr->interruptEnable = 0x7f;	_eieio();
	machine.viaDevice0.logicalAddr->interruptFlag = 0x7f;	_eieio();
	
	machine.viaDevice1.logicalAddr->interruptEnable = 0x7f; _eieio();		// Disable all VIA2 interrupts
	machine.viaDevice1.logicalAddr->interruptFlag = 0x7f; _eieio();
	
	machine.viaDevice1.logicalAddr->slotInterruptEnable = 0x7f; _eieio();	// Disable all slot interrupts
	machine.viaDevice1.logicalAddr->slotInterruptFlag = 0x7f; _eieio();
	
	/* Initialize special interrupt handlers */
	PDMVIA2Interrupts[1].handler = new WrapperInterruptHandler(PDMSlotHandler);
	PDMICRInterrupts[0].handler = new WrapperInterruptHandler(PDMVIA1Handler);
	PDMICRInterrupts[1].handler = new WrapperInterruptHandler(PDMVIA2Handler);
	PDMICRInterrupts[4].handler = new WrapperInterruptHandler(PDMDMAHandler);
	
	return PDMInterruptHandler;
}

Boolean registerPDMInterrupt( InterruptHandler *handler, Int32 interruptType )
{
	/* This is a bit of a complication, since PDM interrupts are only caused by the ICR */
	/* which can only handle 8 different interrupt types, 2 of which are resreved. The */
	/* way this is solved is by having an ugly, but working, hierarchical dispatching */
	/* system. Therefore, certain special interrupt types in the ICR are actually a class of */
	/* interrupts for which a special dispatcher is used to figure out which of the */
	/* different kinds of interrupts in the class occured. This can further be nested, which */
	/* leads to unnecessary complication. Still, the system as it is in Power Macs can only */
	/* handle 30 different interrupt types, compared to PCIs 38 different types */
	
	/* The way the hierarchical dispatching is done is illustrated by the following diagram:
	  *															*
	  *						ICR Interrupt							*
	  *					/--------*-------\							*
	  *		       /----------	     /   \		-------\					*
	  *                   |                             |      |                         |					*
	  *	VIA1 Interrupt | VIA2 Interrupt | DMA Interrupt | ICR native interrupt		*
	  *					| | |										*
	  *				Slot interrupt									*/

#define ENABLE(inBit)		{ if( enable_int != nil ) { *enable_int = (*enable_int) | 0x80 | (1<<inBit); _eieio(); } }

	volatile UInt8*			enable_int = nil;
	UInt32				entryNum = 0;
	
	/* First search in the VIA1 Interrupts table */
	if( searchHandlerTable( PDMVIA1Interrupts, interruptType, 7, &entryNum ) )
	{
		if( addEntryToHandlerTable( PDMVIA1Interrupts, interruptType, entryNum, handler ) )
		{
			// Enable the VIA interrupt
			enable_int = &machine.viaDevice0.logicalAddr->interruptEnable;
			ENABLE(entryNum);
			return true;
		}
	}
	
	/* If not there, search the VIA2 table */
	if( searchHandlerTable( PDMVIA2Interrupts, interruptType, 7, &entryNum ) )
	{
		if( addEntryToHandlerTable( PDMVIA2Interrupts, interruptType, entryNum, handler ) )
		{
			// Enable the VIA interrupt
			enable_int = &machine.viaDevice1.logicalAddr->interruptEnable;
			ENABLE(entryNum);
			return true;
		}
	}
	
	/* If not there, search the DMA table */
	if( searchHandlerTable( PDMDMAInterrupts, interruptType, 7, &entryNum ) )
		if( addEntryToHandlerTable( PDMDMAInterrupts, interruptType, entryNum, handler ) )
			return true;
	
	/* If not there, search the ICR table */
	if( searchHandlerTable( PDMICRInterrupts, interruptType, 8, &entryNum ) )
		if( addEntryToHandlerTable( PDMICRInterrupts, interruptType, entryNum, handler ) )
			return true;
	
	/* Finally search the Slot table */
	if( searchHandlerTable( PDMSlotInterrupts, interruptType, 7, &entryNum ) )
	{
		if( addEntryToHandlerTable( PDMSlotInterrupts, interruptType, entryNum, handler ) )
		{
			// Enable the Slot interrupts
			enable_int = &machine.viaDevice1.logicalAddr->interruptEnable;
			ENABLE(1);
			enable_int = &machine.viaDevice1.logicalAddr->slotInterruptEnable;
			ENABLE(entryNum);
			return true;
		}
	}
	
	/* What the ...?!?!? We couldn't find a match? */
	cout << "Warning: Interrupt of type " << getInterruptName(interruptType) << " could not be registered\n";
	return false;
	
#undef ENABLE
}

Boolean deregisterPDMInterrupt( Int32 interruptType )
{
#define DISABLE(inBit)		{ if( disable_int != nil ) { *disable_int = 1<<inBit & 0x7F; _eieio(); } }

	volatile UInt8*			disable_int = nil;
	UInt32				entryNum;
	
	/* First search in the VIA1 Interrupts table */
	if( searchHandlerTable( PDMVIA1Interrupts, interruptType, 7, &entryNum ) )
	{
		// Disable the VIA interrupt
		PDMVIA1Interrupts[entryNum].handler = nil;
		disable_int = &machine.viaDevice0.logicalAddr->interruptEnable;
		DISABLE(entryNum);
		return true;
	}
	
	/* If not there, search the VIA2 table */
	if( searchHandlerTable( PDMVIA2Interrupts, interruptType, 7, &entryNum ) )
	{
		// Disable the VIA interrupt
		PDMVIA2Interrupts[entryNum].handler = nil;
		disable_int = &machine.viaDevice1.logicalAddr->interruptEnable;
		DISABLE(entryNum);
		return true;
	}
	
	/* If not there, search the DMA table */
	if( searchHandlerTable( PDMDMAInterrupts, interruptType, 7, &entryNum ) )
	{
		PDMDMAInterrupts[entryNum].handler = nil;
		return true;
	}
	
	/* If not there, search the ICR table */
	if( searchHandlerTable( PDMICRInterrupts, interruptType, 8, &entryNum ) )
	{
		PDMICRInterrupts[entryNum].handler = nil;
		return true;
	}
	
	/* Finally search the Slot table */
	if( searchHandlerTable( PDMSlotInterrupts, interruptType, 7, &entryNum ) )
	{
		// Enable the Slot interrupts
		PDMSlotInterrupts[entryNum].handler = nil;
		disable_int = &machine.viaDevice1.logicalAddr->slotInterruptEnable;
		DISABLE(entryNum);
		
		// If there are no more Slot interrupts left, disable Slot interrupts
		Int32		i;
		for( i=0; i<7; i++ )
			if( PDMSlotInterrupts[i].handler != nil )
				break;
		
		if( i==7 )			// If the table was empty i should now be 7
		{
			disable_int = &machine.viaDevice1.logicalAddr->interruptEnable;
			DISABLE(1);
		}
		
		return true;
	}
	
	/* What the ...?!?!? We couldn't find a match? */
	cout << "Warning: Interrupt of type " << getInterruptName(interruptType) << " could not be deregistered\n";
	return false;
	
#undef DISABLE
}

static Boolean PDMInterruptHandler(PPCRegisters*)
{
	UInt32		interrupts, bit;

	interrupts = *interruptControl; _eieio();
	
	for (bit = 0; bit < 7; bit++) {
		if (interrupts & (1<<bit)) {
			if (PDMICRInterrupts[bit].handler != nil) 
				PDMICRInterrupts[bit].handler->handleInterrupt();
			else
				cout << "Unhandled External Interrupt of type "
					<< getInterruptName(PDMICRInterrupts[bit].type) << "\n";
		}
	}
	
	/* Acknowledge interrupt */
	*interruptControl = 0x80; _eieio();
	
	return true;
}

static void PDMSlotHandler(void)
{
	register UInt8		intbits, bit;
	
	intbits  = ~machine.viaDevice1.logicalAddr->slotInterruptFlag;/* Slot interrupts are reversed of everything else */
	_eieio();
	intbits &= machine.viaDevice1.logicalAddr->slotInterruptEnable; /* only care about enabled ints*/
	_eieio();

	/*
	 * Unflag interrupts we're about to process. No, not really. Only VBL can get unflagged
	 */

	for (bit = 0; bit < 7; bit++) {
		if(intbits & (1<<bit)){
			if (bit == 6) {
		/* Special Case.. only VBL bit is allowed to be cleared */
				machine.viaDevice1.logicalAddr->slotInterruptFlag = 0x40; _eieio();
			}

			if (PDMSlotInterrupts[bit].handler != nil) {
				PDMSlotInterrupts[bit].handler->handleInterrupt();
			} else
				cout << "Unhandled External Interrupt of type "
					<< getInterruptName(PDMSlotInterrupts[bit].type) << "\n";
		}
	}
}

static void PDMDMAHandler( void )
{
	register UInt8		bit, intbits;

	intbits  = (*dmaInterruptFlag); _eieio();

	for (bit = 0; bit < 7; bit++) {
		if(intbits & (1<<bit)){
			if (PDMDMAInterrupts[bit].handler != nil) {
				PDMDMAInterrupts[bit].handler->handleInterrupt();
			} else
				cout << "Unhandled External Interrupt of type "
					<< getInterruptName(PDMDMAInterrupts[bit].type) << "\n";
		}
	}
}

static void PDMVIA1Handler( void )
{
	register UInt8 intbits, bit;

	intbits = machine.viaDevice0.logicalAddr->interruptFlag; _eieio();		/* get interrupts pending */
	intbits &= machine.viaDevice0.logicalAddr->interruptEnable; _eieio();	/* only care about enabled */

	if (intbits == 0)
		return;

	/*
	 * Unflag interrupts we're about to process.
	 */
	machine.viaDevice0.logicalAddr->interruptFlag = intbits; _eieio();

	for (bit = 0; bit < 7 ; bit++) {
		if(intbits & (1<<bit)) {
			if (PDMVIA1Interrupts[bit].handler != nil)  
				PDMVIA1Interrupts[bit].handler->handleInterrupt();
			else
				cout << "Unhandled External Interrupt of type "
					<< getInterruptName(PDMVIA1Interrupts[bit].type) << "\n";
		}
	}
}

static void PDMVIA2Handler( void )
{
	register UInt8 intbits, bit;

	intbits = machine.viaDevice1.logicalAddr->interruptFlag; _eieio();		/* get interrupts pending */
	intbits &= machine.viaDevice1.logicalAddr->interruptEnable; _eieio();	/* only care about enabled */

	if (intbits == 0)
		return;

	/*
	 * Unflag interrupts we're about to process.
	 */
	machine.viaDevice1.logicalAddr->interruptFlag = intbits; _eieio();

	for (bit = 0; bit < 7 ; bit++) {
		if(intbits & (1<<bit)) {
			if (PDMVIA2Interrupts[bit].handler != nil)  
				PDMVIA2Interrupts[bit].handler->handleInterrupt();
			else
				cout << "Unhandled External Interrupt of type "
					<< getInterruptName(PDMVIA2Interrupts[bit].type) << "\n";
		}
	}
}