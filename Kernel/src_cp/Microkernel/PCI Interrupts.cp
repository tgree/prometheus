/*
	PCI Interrupts.cp
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
	Terry Greeniaus		December 1997			Original creation of file
	Patrick Varilly			January, 1998			Added support for VIA interrupts
	Patrick Varilly			Wed, 21 Jan 98			Original history tagging of file. Total restructing of interrupt
		---> system. Note that this change is completely transparent to outside software (except for perhaps a minor
		---> performance improvement).
	Terry Greeniaus		Sun, 26 Jan 98			Made it so that (de)registerPCIInterrupt searched all 32 (not 7!) entries in the table.
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 26 June 98	-	Added support for any length of interrupt controller (G3's have 64 bits, not 32!)
											Changed interrupt numbers to OpenFirmware ones.
*/
#include "Assembly.h"
#include "NKMachineInit.h"
#include "NKInterruptVectors.h"
#include "External Interrupt.h"
#include "External Interrupt Internals.h"
#include "PCI Interrupts.h"
#include "NKVideo.h"
#include "Streams.h"
#include "Machine Registers.h"
#include "NKVirtualMemory.h"

static InterruptHandler*			PCIInterruptHandlers[64];
static InterruptHandler*			VIAInterruptHandlers[7];
static UInt8					endianSwap[32] = { 24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23, 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7	};
static Boolean PCIHandleInterrupt( PPCRegisters* theUserRegs );
static void PCIVIAHandler( void );

static PCI_Interrupt_Controller*	pci_int_control;

NKExceptionHandler InitPCIInterrupts(void)
{
	// Initialize the PCI Interrupt and VIA controllers
	Ptr				interruptBase;
	if( machine.kernelMachineType == machineNewWorld )
		interruptBase = (Ptr)0x80800010;
	else
		interruptBase = (Ptr)PCI_IO_PHYS_ADDR(PCI_INTERRUPT_BASE);
	pci_int_control = (PCI_Interrupt_Controller*)NKIOMap((void*)interruptBase,sizeof(PCI_Interrupt_Controller)*4,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	machine.viaDevice0.logicalAddr = (VIA_Chip*)NKIOMap(machine.viaDevice0.physicalAddr,machine.viaDevice0.len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	for(Int32 i=0;i<machine.interruptControllerWidth/32;i++)
	{
		pci_int_control[i].mask = 0; _eieio();					// Disable all interrupts
		pci_int_control[i].clear = 0xFFFFFFFF; _eieio();			// Clear pending interrupts
		pci_int_control[i].mask = 0; _eieio();					// Disable all interrupts
	}
	
	machine.viaDevice0.logicalAddr->peripheralControl = 0x00; _eieio();	// Set up peripheral control (whatever that means)
	machine.viaDevice0.logicalAddr->interruptEnable = 0x00; _eieio();		// Disable all VIA-style registers
	machine.viaDevice0.logicalAddr->interruptFlag = 0x7F; _eieio();		// Flag all pending VIA-style interrupts (e.g. mouse activity)
	
	// Set the PCI VIA Handler to its correct initial value
	PCIInterruptHandlers[PMAC_DEV_VIA] = new WrapperInterruptHandler(PCIVIAHandler);
	
	// Disable all interrupts
	for(Int32 i=0;i<machine.interruptControllerWidth/32;i++)
	{
		pci_int_control[i].mask = 0; _eieio();
	}
	
	return PCIHandleInterrupt;
}

Boolean registerPCIInterrupt( InterruptHandler *handler, Int32 interruptType )
{
	// First search the main interrupt tables
	if( interruptType >= 0 )
	{
		// Enable the interrupt
		PCIInterruptHandlers[interruptType] = handler;
		pci_int_control[(machine.interruptControllerWidth - interruptType - 1)/32].mask |= (1 << endianSwap[interruptType % 32]);
		_eieio();
		_eieio();
		
		return true;
	}
	
	// If no match is found, search the VIA tables
	if( interruptType >= -6 )
	{
		// Enable the VIA interrupt
		interruptType = -interruptType;
		VIAInterruptHandlers[interruptType] = handler;
		machine.viaDevice0.logicalAddr->interruptEnable |= (1 << interruptType);
		_eieio();
		
		// Enable VIA interrupts if they were disabled
		pci_int_control[(machine.interruptControllerWidth - PMAC_DEV_VIA - 1)/32].mask |= (1 << endianSwap[PMAC_DEV_VIA % 32]);
		_eieio();
		
		return true;
	}
	
	// What the...?!?!?!?
	cout << "Warning: Interrupt of type " << getInterruptName(interruptType) << " could not be registered\n";
	return false;
}

Boolean deregisterPCIInterrupt( Int32 interruptType )
{
	// First search the main interrupt tables
	if( interruptType >= 0)
	{
		// Disable the interrupt
		PCIInterruptHandlers[interruptType] = nil;
		pci_int_control[(machine.interruptControllerWidth - interruptType - 1)/32].mask &= ~(1 << endianSwap[interruptType % 32]);
		_eieio();
		
		return true;
	}
	
	// If no match is found, search the VIA tables
	if( interruptType >= -6 )
	{
		// Disable the VIA interrupt
		VIAInterruptHandlers[interruptType] = nil;
		machine.viaDevice0.logicalAddr->interruptEnable &= ~(1 << interruptType);
		_eieio();
		
		// If there are no more VIA-style interrupts left, disable VIA interrupts
		Int32 i;
		for( i=0; i<7; i++ )
			if( VIAInterruptHandlers[i] != nil )
				break;
		
		if( i==7 )			// If the table was empty i should now be 7
		{
			pci_int_control[(machine.interruptControllerWidth - PMAC_DEV_VIA - 1)/32].mask &= ~(1 << endianSwap[PMAC_DEV_VIA]);
			_eieio();
		}
		
		return true;
	}
	
	// What the...?!?!?!?
	cout << "Warning: Interrupt of type " << getInterruptName(interruptType) << " could not be deregistered\n";
	return false;
}

void pendingInterrupts(UInt32* intHigh,UInt32* intLow)
{
	*intHigh = _l4le(&pci_int_control[0].levels);
	_eieio();
	*intLow = _l4le(&pci_int_control[1].levels);
	_eieio();
}

void enableInterrupts(UInt32 intHigh,UInt32 intLow)
{
	UInt32 mask = _l4le(&pci_int_control[0].mask);
	_st4le(intHigh | mask,&pci_int_control[0].mask);
	_eieio();
	mask = _l4le(&pci_int_control[1].mask);
	_st4le(intLow | mask,&pci_int_control[1].mask);
	_eieio();
}

static Boolean PCIHandleInterrupt( PPCRegisters* )
{
	UInt32	interrupts;
	
	for(Int32 i=0;i<machine.interruptControllerWidth/32;i++)
	{
		interrupts = _l4le(&pci_int_control[machine.interruptControllerWidth/32 - i - 1].events); _eieio();
		_st4le(interrupts,&pci_int_control[machine.interruptControllerWidth/32 - i - 1].clear); _eieio();
		
		UInt32	bit;
		
		for (bit = 0; bit < 32 ; bit++)
		{
			if (interrupts & (1<<bit))
			{
				if (PCIInterruptHandlers[bit + i*32] != nil)
					PCIInterruptHandlers[bit + i*32]->handleInterrupt();
				else
					nkVideo << "Unhandled External Interrupt of type "
						<< getInterruptName(bit + i*32) << " " << (UInt32)(bit + i*32) << "\n";
			}
		}
	}
	
	return true;
}

static void PCIVIAHandler( void )
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
			if (VIAInterruptHandlers[bit] != nil)  
			{
				//nkVideo << "External VIA Interrupt: " << getInterruptName(-bit) << "\n";
				VIAInterruptHandlers[bit]->handleInterrupt();
			}
			else
				nkVideo << "Unhandled External Interrupt of type "
					<< getInterruptName(-bit) << "\n";
		}
	}
}