/*
	IBM IDE Driver.cp
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
	Terry Greeniaus	-	Tuesday, 23 June 98		-	Original creation of file
	Terry Greeniaus	-	Wednesday, 26 Oct 99	-	Added preliminary ATAPI support
	Patrick Varilly		-	Tuesday, 2 Nov 99		-	Fixed silly bug with returns at the beginning of
												start and initialize
*/
#include "NKThreads.h"
#include "NKVirtualMemory.h"
#include "IBM IDE Driver.h"
#include "Kernel Console.h"
#include "ANSI.h"
#include "ATA Command Factory.h"
#include "IDE Commands.h"
#include "Chip Debugger.h"

static RegisterDescriptor	ideRegisterDescriptor[]	=	{	CHIP_REGISTER(IDERegsIBM,data_2,REG_SIDE_EFFECTS),
												CHIP_REGISTER(IDERegsIBM,data_4,REG_SIDE_EFFECTS),
												CHIP_REGISTER(IDERegsIBM,error_read,REG_READ_ONLY),
												CHIP_REGISTER(IDERegsIBM,features_write,REG_WRITE_ONLY),
												CHIP_REGISTER(IDERegsIBM,sectorCount,REG_NOFLAGS),
												CHIP_REGISTER(IDERegsIBM,sector,REG_NOFLAGS),
												CHIP_REGISTER(IDERegsIBM,cylinderLow,REG_NOFLAGS),
												CHIP_REGISTER(IDERegsIBM,cylinderHigh,REG_NOFLAGS),
												CHIP_REGISTER(IDERegsIBM,head,REG_NOFLAGS),
												CHIP_REGISTER(IDERegsIBM,status_read,REG_READ_ONLY),
												CHIP_REGISTER(IDERegsIBM,command_write,REG_WRITE_ONLY),
												CHIP_REGISTER(IDERegsIBM,alternateStatus_read,REG_READ_ONLY),
												CHIP_REGISTER(IDERegsIBM,deviceControl_write,REG_WRITE_ONLY),
												LAST_REGISTER
											};

IBMIDEDriver::IBMIDEDriver(ConstASCII8Str name,MachineDevice<struct IDERegsIBM>* deviceRegs):
	IDEDriver(name,deviceRegs->interrupts[0])
{
	regs = deviceRegs->logicalAddr = (IDERegsIBM*)NKIOMap(deviceRegs->physicalAddr,deviceRegs->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// For the Chip Debugger
	new Chip("IBM ide",ideRegisterDescriptor,regs);
}

IBMIDEDriver::~IBMIDEDriver()
{
}

UInt8 IBMIDEDriver::readError()
{
	return ReadUReg8(&regs->error_read);
}

UInt8 IBMIDEDriver::readSectorCount()
{
	return ReadUReg8(&regs->sectorCount);
}

UInt8 IBMIDEDriver::readSector()
{
	return ReadUReg8(&regs->sector);
}

UInt8 IBMIDEDriver::readCylinderLow()
{
	return ReadUReg8(&regs->cylinderLow);
}

UInt8 IBMIDEDriver::readCylinderHigh()
{
	return ReadUReg8(&regs->cylinderHigh);
}

UInt8 IBMIDEDriver::readHead()
{
	return ReadUReg8(&regs->head);
}

UInt8 IBMIDEDriver::readStatus()
{
	return ReadUReg8(&regs->status_read);
}

void IBMIDEDriver::writeData16(UInt16 data)
{
	WriteUReg16BE(data,&regs->data_2);
}

void IBMIDEDriver::writeFeatures(UInt8 data)
{
	WriteUReg8(data,&regs->features_write);
}

void IBMIDEDriver::writeSectorCount(UInt8 data)
{
	WriteUReg8(data,&regs->sectorCount);
}

void IBMIDEDriver::writeSector(UInt8 data)
{
	WriteUReg8(data,&regs->sector);
}

void IBMIDEDriver::writeCylinderLow(UInt8 data)
{
	WriteUReg8(data,&regs->cylinderLow);
}

void IBMIDEDriver::writeCylinderHigh(UInt8 data)
{
	WriteUReg8(data,&regs->cylinderHigh);
}

void IBMIDEDriver::writeHead(UInt8 data)
{
	WriteUReg8(data,&regs->head);
}

void IBMIDEDriver::writeCommand(UInt8 data)
{
	WriteUReg8(data,&regs->command_write);
}

void IBMIDEDriver::writeDeviceControl(UInt8 data)
{
	WriteUReg8(data,&regs->deviceControl_write);
}

void IBMIDEDriver::slamRead512(Ptr destLogical)
{
	SlamSector16((UReg16BE*)&regs->data_2,destLogical);
}

void IBMIDEDriver::slamRead512LE(Ptr destLogical)
{
	SlamSector16LE((UReg16LE*)&regs->data_2,destLogical);
}
