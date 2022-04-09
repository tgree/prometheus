/*
	External Interrupts.cp
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
	Patrick Varilly			January, 1998			Ported code to PDM machines
	Patrick Varilly			Tue, 20 Jan 98			Original history tagging of file
	Patrick Varilly			Wed, 21 Jan 98			Total restructing of interrupt system. Note that this change is
		---> completely transparent to outside software (except for perhaps a minor performance improvement).
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Wed, 10 Nov 98	-	Fixed getInterruptName so it actually returns the right name
*/
/*
	External Interrupt.h
	
	A set of classes and functions that take care of initializing and making external interrupts work
	
	Copyright © 1997-1998 by The Pandora Team. All rights reserved worldwide.
*/
#include "Kernel Types.h"
#include "Assembly.h"
#include "NKMachineInit.h"
#include "NKInterruptVectors.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "Streams.h"
#include "External Interrupt.h"
#include "External Interrupt Internals.h"
#include "PDM Interrupts.h"
#include "PCI Interrupts.h"

void InitExternalInterrupt(void)
{
	NKExceptionHandler	externalInterruptProc = nil;
	
	// I think we should get rid of the HardwareInterruptHandler class and put two functions, InitPDMInterrupts() and InitPCIInterrupts() in
	// the respective files.  That also gets lots of junk out of the constructors for PCIInterruptHandler and PDMInterruptHandler.  These two
	// init routines will return a value for use in externalInterruptProc - that means that there will be a seperate C style function for handling
	// PCI and PDM interrupts.  From there, it can be dispatched to the appropriate static global PCIInterruptHandler or PDMInterruptHandler.
	switch(machine.machineClass)
	{
		case classPCI:
		case classPowerBookPCI:
		case classG3:
		case classNewWorld:
			externalInterruptProc = InitPCIInterrupts();
		break;
		case classPDM:
		case classPerforma:
		case classPowerBook:
			externalInterruptProc = InitPDMInterrupts();
		break;
		default:
			cout << "Don't know enough about this type of machine to enable interrupts\n\n";
		break;
	}
	
	if(externalInterruptProc)
	{
		// Enable external interrupts
		NKInstallExceptionHandler(externalInterruptProc,externalException);
		_setDEC(0x7FFFFFFF);	// Pat: what's this here for?  Decs should just be ignored before the preempter is up
		_setDEC(0x80000000);
		SetMSR(GetMSR() | 0x00008000);	// Enable Decrementer and External interrupts
	}
}

ASCII8Str			_interruptNames[PMAC_INT_MAX] = {
	
	// Miscellaneous
	"VBL", "NMI", "PDS", 
	
	// Clock interrupt names
	"VIA - Timer 1", "VIA - Timer 2", "VIA - PMU", "VIA - Reserved", "VIA - CUDA", "VIA - Hz Tick",
	
	// DMA Interrupt names
	"DMA SCSI 0","DMA Floppy",
	"DMA Ethernet Transmit / IDE 0", "DMA Ethernet Receive / IDE 1",
	"DMA SCC A Transmit", "DMA SCC A Receive",
	"DMA SCC B Transmit", "DMA SCC B Receive",
	"DMA Audio Out", "DMA Audio In",
	"DMA SCSI 1", "Reserved - 11",
	
	// Device interrupt names
	"SCSI 0", "SCSI 1 / IDE 0", "Ethernet / IDE 1", "SCC A", "SCC B", "Audio",
	"VIA", "Floppy",
	
	// Card interrupt names (not very informative)
	"Card 0", "Card 1", "Card 2", "Card 3", "Card 4", "Card 5",
	"Card 6", "Card 7", "Card 8", "Card 9", "Card 10"
	
};
ASCII8Str*			interruptNames = &_interruptNames[9];

ASCII8Str getInterruptName( Int32 interruptType )
{
	if( (interruptType > PMAC_DEV_CARD10) || (interruptType < PMAC_DEV_VBL) )
		return "Out of Range";
	
	return interruptNames[interruptType];
}

InterruptHandler::InterruptHandler(UInt32 type0,UInt32 type1)
{
	interruptType[0] = type0;
	interruptType[1] = type1;
}

InterruptHandler::~InterruptHandler()
{
}

void InterruptHandler::enable()
{
	Boolean			result = false;
	
	for(UInt32 i=0;i<2;i++)
	{
		if(interruptType[i] == PMAC_DEV_NO_INT)
			continue;
		
		switch( machine.machineClass )
		{
			case classPCI:
			case classPowerBookPCI:
			case classG3:
			case classNewWorld:
				result = registerPCIInterrupt( this, this->interruptType[i] );
			break;
			case classPDM:
			case classPerforma:
			case classPowerBook:
				result = registerPDMInterrupt( this, this->interruptType[i] );
			break;
			default:
				cout << "Error in InterruptHandler::enable() - don't know interrupt type!\n";
			break;
		}
		
		if( result == false )
			cout << "Could not enable interrupt of type " << getInterruptName( this->interruptType[i] ) << "\n";
	}
}

void InterruptHandler::disable()
{
	Boolean			result = false;
	
	for(UInt32 i=0;i<2;i++)
	{
		if(interruptType[i] == PMAC_DEV_NO_INT)
			continue;
		
		switch( machine.machineClass )
		{
			case classPCI:
			case classPowerBookPCI:
			case classG3:
			case classNewWorld:
				result = deregisterPCIInterrupt( this->interruptType[i] );
			break;
			case classPDM:
			case classPowerBook:
			case classPerforma:
				result = deregisterPDMInterrupt( this->interruptType[i] );
			break;
			default:
				cout << "Error in InterruptHandler::disable() - don't know interrupt type!\n";
			break;
		}
		
		if( result == false )
			cout << "Could not disable interrupt of type " << getInterruptName( this->interruptType[i] ) << "\n";
	}
}

Boolean searchHandlerTable( HandlerTable *table, Int32 type, UInt32 numEntries, UInt32* entryNum )
{
	UInt32			i;
	
	for( i=0; i<numEntries; i++ )
	{
		if( type == table[i].type )
		{
			*entryNum = i;
			return true;
		}
	}
	
	return false;
}

Boolean addEntryToHandlerTable( HandlerTable *table, Int32 interruptType, UInt32 entryNum, InterruptHandler *handler )
{
	if( interruptType > PMAC_INT_MAX )
	{
		cout << "Tried to add out of range interrupt to a handler table\n";
		return false;
	}
	
	if( table[entryNum].handler != nil )
		cout << "Warning: interrupt of type " << getInterruptName( interruptType ) << " has been replaced\n";
	
	table[entryNum].handler = handler;
	
	return true;
}