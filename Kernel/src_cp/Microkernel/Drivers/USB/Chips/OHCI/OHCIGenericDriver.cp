/*
	OHCIGenericDriver.cp
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

#include "OHCIGenericDriver.h"

RegisterDescriptor			ohciRegisterDescriptor[] = {
			CHIP_REGISTER( OHCIRegs, Revision, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, Control, REG_LE ),
			CHIP_REGISTER( OHCIRegs, CommandStatus, REG_LE ),
			CHIP_REGISTER( OHCIRegs, IntStatus, REG_LE ),
			CHIP_REGISTER( OHCIRegs, IntEnable, REG_LE | REG_SIDE_EFFECTS ),
			CHIP_REGISTER( OHCIRegs, IntDisable, REG_LE | REG_SIDE_EFFECTS ),
			
			CHIP_REGISTER( OHCIRegs, HCCA, REG_LE | REG_READ_ONLY ),				// I make these read-only
			CHIP_REGISTER( OHCIRegs, PeriodCurrentED, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, ControlHeadED, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, ControlCurrentED, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, BulkHeadED, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, BulkCurrentED, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, DoneHead, REG_LE | REG_READ_ONLY ),
			
			CHIP_REGISTER( OHCIRegs, FmInterval, REG_LE ),
			CHIP_REGISTER( OHCIRegs, FmRemaining, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, FmNumber, REG_LE | REG_READ_ONLY ),
			CHIP_REGISTER( OHCIRegs, PeriodicStart, REG_LE ),
			CHIP_REGISTER( OHCIRegs, LSThreshold, REG_LE ),
			
			CHIP_REGISTER( OHCIRegs, RhDescriptorA, REG_LE | REG_SIDE_EFFECTS ),
			CHIP_REGISTER( OHCIRegs, RhDescriptorB, REG_LE | REG_SIDE_EFFECTS ),
			CHIP_REGISTER( OHCIRegs, RhStatus, REG_LE | REG_SIDE_EFFECTS ),
			CHIP_REGISTER( OHCIRegs, RhPortStatus[0], REG_LE ),
			CHIP_REGISTER( OHCIRegs, RhPortStatus[1], REG_LE ),
			// Only shows first two root hub ports
			
			LAST_REGISTER };

OHCIGenericDriver::OHCIGenericDriver()
{
}

OHCIGenericDriver::~OHCIGenericDriver()
{
}

// * HC register readers
// All readers do basically the same stuff
#define GENERIC_READER(type,func,reg,arg)\
type \
OHCIGenericDriver::func(arg)\
{\
	type				val;\
	val = (type)ReadUReg32LE( &regs->reg );\
	_eieio();\
	return val;\
}
#define GENERIC_INT_READER(func,reg,arg)	GENERIC_READER(UInt32,func,reg,arg)
#define GENERIC_PTR_READER(func,reg,arg)	GENERIC_READER(void*,func,reg,arg)
#define INT_READER(reg)					GENERIC_INT_READER(get ## reg, reg,)
#define PTR_READER(reg)					GENERIC_PTR_READER(get ## reg, reg,)

// Control and Status
INT_READER(Revision)
INT_READER(Control)
INT_READER(CommandStatus)
INT_READER(IntStatus)
GENERIC_INT_READER(getIntMask,IntEnable,)

// Memory Pointers
PTR_READER(HCCA)
PTR_READER(PeriodCurrentED)
PTR_READER(ControlHeadED)
PTR_READER(ControlCurrentED)
PTR_READER(BulkHeadED)
PTR_READER(BulkCurrentED)
GENERIC_PTR_READER(getHcDoneHead,DoneHead,)

// Frame Counter
INT_READER(FmInterval)
INT_READER(FmRemaining)
INT_READER(FmNumber)
INT_READER(PeriodicStart)
INT_READER(LSThreshold)

// Root Hub
INT_READER(RhDescriptorA)
INT_READER(RhDescriptorB)
INT_READER(RhStatus)
GENERIC_INT_READER(getRhPortStatus,RhPortStatus[port],UInt8 port)
	
// * HC register writers
#define GENERIC_WRITER(type,func,reg)\
void \
OHCIGenericDriver::func( type val )\
{\
	WriteUReg32LE( (UInt32)val, &regs->reg );\
	_eieio();\
}
#define GENERIC_ARG_WRITER(type,func,reg,arg)\
void \
OHCIGenericDriver::func( arg, type val )\
{\
	WriteUReg32LE( (UInt32)val, &regs->reg );\
	_eieio();\
}
#define GENERIC_INT_WRITER(func,reg)		GENERIC_WRITER(UInt32,func,reg)
#define GENERIC_PTR_WRITER(func,reg)		GENERIC_WRITER(void*,func,reg)
#define INT_WRITER(reg)					GENERIC_INT_WRITER(set ## reg, reg)
#define PTR_WRITER(reg)					GENERIC_PTR_WRITER(set ## reg, reg)

// Control and Status
INT_WRITER(Control)
INT_WRITER(CommandStatus)
GENERIC_INT_WRITER(enableInt,IntEnable)
GENERIC_INT_WRITER(disableInt,IntDisable)
GENERIC_INT_WRITER(clearInt,IntStatus)

// Memory Pointers
PTR_WRITER(HCCA)
PTR_WRITER(ControlHeadED)
PTR_WRITER(ControlCurrentED)
PTR_WRITER(BulkHeadED)
PTR_WRITER(BulkCurrentED)

void
OHCIGenericDriver::clearHcDoneHead()
{
	WriteUReg32LE( 0, &regs->DoneHead );
	_eieio();
}

// Frame Counter
INT_WRITER(PeriodicStart)
INT_WRITER(FmInterval)
INT_WRITER(LSThreshold)

// Root Hub
INT_WRITER(RhDescriptorA)
INT_WRITER(RhDescriptorB)
INT_WRITER(RhStatus)
GENERIC_ARG_WRITER(UInt32,setRhPortStatus,RhPortStatus[port],UInt8 port)

// Legacy
void
OHCIGenericDriver::clearHceControl()
{
	WriteUReg32LE( 0, &regs->HceControl );
	_eieio();
}