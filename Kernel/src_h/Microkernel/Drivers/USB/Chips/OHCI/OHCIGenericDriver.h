/*
	OHCIGenericDriver.h
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
	Patrick Varilly		-	Fri, 26 Nov 99		-	Original creation of file
*/

#ifndef __OHCI_GENERIC_DRIVER__
#define __OHCI_GENERIC_DRIVER__

#include "OHCIDriver.h"
#include "Chip Debugger.h"

// OHCI on-chip registers
typedef struct OHCIRegs
{
	// Control and status
	UReg32LE					Revision;
	UReg32LE					Control;
	UReg32LE					CommandStatus;
	UReg32LE					IntStatus;
	UReg32LE					IntEnable;
	UReg32LE					IntDisable;
	
	// Memory pointers
	UReg32LE					HCCA;
	UReg32LE					PeriodCurrentED;
	UReg32LE					ControlHeadED;
	UReg32LE					ControlCurrentED;
	UReg32LE					BulkHeadED;
	UReg32LE					BulkCurrentED;
	UReg32LE					DoneHead;
	
	// Frame counter
	UReg32LE					FmInterval;
	UReg32LE					FmRemaining;
	UReg32LE					FmNumber;
	UReg32LE					PeriodicStart;
	UReg32LE					LSThreshold;
	
	// Root hub
	UReg32LE					RhDescriptorA;
	UReg32LE					RhDescriptorB;
	UReg32LE					RhStatus;
	UReg32LE					RhPortStatus[15];		// Up to 15 ports in root hub (iMacs have 2)
	
	// Legacy support
	UReg32LE					LegacyPad[28];			// Legacy support registers at offset 100h from base addr
	UReg32LE					HceControl;
	// I don't care about the rest...
} OHCIRegs;

class OHCIGenericDriver :
	public OHCIDriver
{
public:
							OHCIGenericDriver();
	virtual					~OHCIGenericDriver();
	
protected:
	// The generic driver provides accessors for a compliant memory-mapped OHCI device (all OHCI chips should
	// be memory mapped, but we can be on the safe side by doing this)
	
	// * HC register readers
	// Control and Status
	virtual UInt32				getRevision();
	virtual UInt32				getControl();
	virtual UInt32				getCommandStatus();
	virtual UInt32				getIntStatus();
	virtual UInt32				getIntMask();
	// Memory Pointers
	virtual void*				getHCCA();
	virtual void*				getPeriodCurrentED();
	virtual void*				getControlHeadED();
	virtual void*				getControlCurrentED();
	virtual void*				getBulkHeadED();
	virtual void*				getBulkCurrentED();
	virtual void*				getHcDoneHead();
	// Frame Counter
	virtual UInt32				getFmInterval();
	virtual UInt32				getFmRemaining();
	virtual UInt32				getFmNumber();
	virtual UInt32				getPeriodicStart();
	virtual UInt32				getLSThreshold();
	// Root Hub
	virtual UInt32				getRhDescriptorA();
	virtual UInt32				getRhDescriptorB();
	virtual UInt32				getRhStatus();
	virtual UInt32				getRhPortStatus( UInt8 port );
	
	// * HC register writers
	// Control and Status
	virtual void				setControl( UInt32 val );
	virtual void				setCommandStatus( UInt32 val );
	virtual void				enableInt( UInt32 val );
	virtual void				disableInt( UInt32 val );
	virtual void				clearInt( UInt32 val );
	// Memory Pointers
	virtual void				setHCCA( void* val );
	virtual void				setControlHeadED( void* val );
	virtual void				setControlCurrentED( void* val );
	virtual void				setBulkHeadED( void* val );
	virtual void				setBulkCurrentED( void* val );
	virtual void				clearHcDoneHead();
	// Frame Counter
	virtual void				setPeriodicStart( UInt32 val );
	virtual void				setFmInterval( UInt32 val );
	virtual void				setLSThreshold( UInt32 val );
	// Root Hub
	virtual void				setRhDescriptorA( UInt32 val );
	virtual void				setRhDescriptorB( UInt32 val );
	virtual void				setRhStatus( UInt32 val );
	virtual void				setRhPortStatus( UInt8 port, UInt32 val );
	// Legacy
	virtual void				clearHceControl();
	
	// Actual memory-mapped registers
	OHCIRegs					*regs;
};

extern RegisterDescriptor			ohciRegisterDescriptor[];

#endif /* __OHCI_GENERIC_DRIVER__ */