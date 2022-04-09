/*
	IDE Driver.cp
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
#include "IDE Driver.h"
#include "Kernel Console.h"
#include "ANSI.h"
#include "ATA Command Factory.h"
#include "IDE Commands.h"
#include "Chip Debugger.h"
#include "PCI Interrupts.h"

#define	DISP_STATUS	0
#if		DISP_STATUS
#define	STATUS(str)	nkVideo << str
#else
#define	STATUS(str)	do{}while(0)
#endif

enum
{
	// Bits in the status register
	statusBusy			=	0x80,
	statusDriveReady		=	0x40,
	statusDriveWriteFault	=	0x20,
	statusDriveSeekComplete	=	0x10,
	statusDataRequest		=	0x08,
	statusCorrectedData		=	0x04,
	statusIndex			=	0x02,
	statusError			=	0x01
};

enum
{
	// Bits in the device control register
	deviceControlSoftReset		=	0x04,
	deviceControlDisableInterrupts	=	0x02,
	deviceControlEnableInterrupts	=	0
};

enum
{
	// Bits in the drive/head register
	driveHeadLBAMode		=	0x40,
	driveHeadDriveNumber	=	0x10,
	driveHeadHeadSelectMask	=	0x0F,
	driveHeadLBAMask		=	0x0F
};

enum
{
	// Bits in the error register
	errorBadBlockDetected		=	0x80,
	errorUncorrectablData		=	0x40,
	errorMediaChanged			=	0x20,
	errorIDNotFound			=	0x10,
	errorMediaChangeRequest		=	0x08,
	errorAbortedCommand		=	0x04,
	errorTrack0NotFound		=	0x02,
	errorAddressMarkNotFound	=	0x01
};

IDEDriver::IDEDriver(ConstASCII8Str name,UInt32 int1,UInt32 int2):
	IOCommandDriver(name),
	InterruptHandler(int1,int2)
{
	masterPresent = slavePresent = false;
	justDidDRDYHoser = false;
	isAtapi = false;
	currCommand = nil;
}

IDEDriver::~IDEDriver()
{
}

void IDEDriver::initialize()
{
	// Select drive
	selectDrive(ideMaster);
	
	// See if anyone's there
	writeCylinderLow(0xA5);
	if(readCylinderLow() != 0xA5)
		return;
	
	// Disable interrupts
	writeDeviceControl(deviceControlDisableInterrupts);
}

static UInt32	driverStart = 0;

void IDEDriver::start()
{
	// Select drive
	selectDrive(ideMaster);
	
	// See if anyone's there
	writeCylinderLow(0xA5);
	if(readCylinderLow() != 0xA5)
		return;
	
	// Reset the drive
	STATUS("IDEDriver::resetting device\n");
	writeDeviceControl(deviceControlSoftReset | deviceControlDisableInterrupts);
	justDidDRDYHoser = true;
	Wait_us(30000);
	writeDeviceControl(deviceControlDisableInterrupts);
	
	// Wait for busy to clear
	STATUS("IDEDriver::waiting for busy to clear\n");
	Wait_ns(400);
	if(!waitForBusyClear())
		cout << "\tIDEDriver::start() timed out waiting for busy to clear\n";
	
	// Determine if this is an ATAPI device
	isAtapi = ((readStatus() & statusDriveReady) == 0);
	if(isAtapi)
	{
		if(readSectorCount() != 0x01 || readSector() != 0x01 || readCylinderLow() != 0x14 || readCylinderHigh() != 0xEB)
		{
			isAtapi = false;
			cout << "\tThought I found an ATAPI device, strange...\n";
		}
		else
			cout << "\tFound ATAPI device\n";
	}
	else
		cout << "\tFound ATA device\n";
	
	// Enable interrupts
	enable();
	writeDeviceControl(deviceControlEnableInterrupts);
	
	// Execute drive diagnostics
	STATUS("IDEDriver::executing drive diagnostics\n");
	IDECommand*	theCommand = ATACommandFactory::executeDriveDiagnostic();
	//if(++driverStart == 2)
	//	enableInterrupts(0xFFFFFFFF,0xFFFFFFFF);
	command(theCommand);
	CurrThread::blockForIO(theCommand);
	
	// Check error
	if((theCommand->error & 0x0F) == 1)
	{
		STATUS("IDE Master present\n");
		masterPresent = true;
	}
	else
		nkVideo << "IDE Diagnostics Master Error = " << (UInt32)theCommand->error << "\n";
	
	// Check for slave - not sure if this is correct or not
	if((theCommand->error & 0xF0) == 0x80)
	{
		selectDrive(ideSlave);
		UInt8 errorSlave = readError();
		if((errorSlave & 0x0F) == 1)
		{
			STATUS("IDE Slave present\n");
			slavePresent = true;
		}
		else
			nkVideo << "IDE Diagnostics Slave Error = " << (UInt32)errorSlave << "\n";
	}
	
	delete theCommand;
}

void IDEDriver::stop()
{
	writeDeviceControl(deviceControlDisableInterrupts);
	
	// Clear interrupts
	readStatus();
	
	disable();
}

void IDEDriver::startAsyncIO(IOCommand* cmd)
{
	if(cmd)
	{
		currCommand = static_cast<IDECommand*>(cmd);
		
		STATUS("IDEDriver::startAsyncIO with command " << (UInt32)cmd << "\n");
		
		if(currCommand->dataPtr)
			FatalAssert((currCommand->dataLen % 512) == 0);
		
		if(!waitForBusyClear(statusDriveReady))
		{
			nkVideo << "ATA device timed out on an async io command\n";
			currCommand->error = -1;
			currCommand->doneIO();
			return;
		}
		
		// Select the drive, setting the head register at the same time
		if(currCommand->setRegisters & setHeadFlag)
			writeHead(0xA0 | (currCommand->drive << 4) | (currCommand->head & 0x0F));
		else
			writeHead(0xA0 | (currCommand->drive << 4));
		
		// Do the other registers
		if(currCommand->setRegisters & setFeaturesFlag)
			writeFeatures(currCommand->features);
		if(currCommand->setRegisters & setSectorCountFlag)
			writeSectorCount(currCommand->sectorCount);
		if(currCommand->setRegisters & setSectorNumberFlag)
			writeSector(currCommand->sectorNumber);
		if(currCommand->setRegisters & setCylinderFlag)
		{
			writeCylinderHigh((currCommand->cylinder >> 8) & 0x00FF);
			writeCylinderLow(currCommand->cylinder & 0x00FF);
		}
		
		// See if this is a DRDY hoser
		if(isAtapi)
		{
			if(currCommand->command == 0x08 || currCommand->command == 0x90)	// DEVICE RESET or EXECUTE DRIVE DIAGNOSTICS
				justDidDRDYHoser = true;
			else
				justDidDRDYHoser = false;
		}
		
		// Send the command
		writeCommand(currCommand->command);
	}
}

void IDEDriver::handleInterrupt()
{
	STATUS("IDEDriver::handleInterrupt\n");
	if(currCommand)
	{
		Boolean				done = false;
		
		STATUS("   IDE Command " << (UInt32)currCommand->command << " at " << (UInt32)currCommand << "\n");
		
		if(!waitForBusyClear())
			nkVideo << "Timed out waiting for IDE status\n";
			
		currCommand->status = readStatus();
		
		if((currCommand->status & statusError) || currCommand->command == 0x90)
			currCommand->error = readError();
		
		if(currCommand->dataLen && !currCommand->error)
		{
			UInt8	busyClear = statusDriveReady | statusDataRequest;
			if(!isAtapi)
				busyClear |= statusDriveSeekComplete;
			if(waitForBusyClear(busyClear))
			{
				ProcessWindow	window(currCommand->process);
				currCommand->dataLen -= 512;
				if(currCommand->command == 0xEC || currCommand->command == 0xA1)	// Identify Drive Data is transferred in little-endian format
				{
					slamRead512LE((Ptr)currCommand->dataPtr);
					currCommand->dataPtr = (void*)((UInt32)currCommand->dataPtr + 512);
				}
				else
				{
					slamRead512((Ptr)currCommand->dataPtr);
					currCommand->dataPtr = (void*)((UInt32)currCommand->dataPtr + 512);
				}
				done = !currCommand->dataLen;
			}
			else
			{
				done = true;
				currCommand->error  = readError();
			}
		}
		else
			done = true;
		
		if(currCommand->error && currCommand->command != 0x90)
			cout << redMsg << "IDE Command " << (UInt32)currCommand->command << " with error " << (UInt32)currCommand->error << "\n" << whiteMsg;
		
		if(done)
		{
			currCommand->doneIO();
			currCommand = nil;
			startAsyncIO(dequeue());
		}
	}
	else
	{
		// Clear any pending interrupts by reading status
		nkVideo << "Ignored an IDE interrupt\n";
		readStatus();
	}
}

void IDEDriver::command(IDECommand* cmd)
{
	cmd->error = 0;
	
	STATUS("IDE Driver starting command at " << (UInt32)cmd << "\n");
	enqueue(cmd);
	/*
	// Do this synchronously (no interrupts) for now...
	while(!cmd->ioDone())
	{
		UInt8	status = readStatus();
		while(status & statusBusy)
		{
			Wait_us(100);
			status = readStatus();
		}
		handleInterrupt();
	}
	*/
}

Boolean IDEDriver::waitForBusyClear(UInt8 setBits)
{
	UInt32 timeout = 2000;
	
	if(isAtapi && justDidDRDYHoser)
		if(setBits & statusDriveReady)
			setBits &= ~statusDriveReady;
	
	do
	{
		UInt8 status = readStatus();
		if(!(status & statusBusy) && (status & setBits) == setBits)
			return true;
		if(timeout < 1000)
			Wait_us(1000);
		else
			Wait_us(1);
	}while(--timeout);
	
	return false;
}

void IDEDriver::selectDrive(Int8 masterOrSlave)
{
	if(!waitForBusyClear())
		nkVideo << "    selectDrive timed out on waitForBusyClear()\n";
	
	Int8	val = 0xA0;
	if(masterOrSlave == ideSlave)
		val |= driveHeadDriveNumber;
	
	writeHead(val);
}

UInt32 IDECommand::ioError()
{
	return error;
}

__asm__ void SlamSector16(register UReg16BE* regs16,register Ptr dest)
{
	// Load 32 bytes into registers r0,r5-r11 and blast them at once, do this 16 times for the
	// full 512 byte sector
	li		r5,16;
	mtctr	r5;
	subi		r4,r4,4;
@loop:
	// Cache hint for the processor
	li		r12,4;
	dcbtst	r12,r4;
	//dcbz		r12,r4;	// We're writing the full cacheline
	
	// Load the bytes
	lhz		r0,0(r3);
	eieio;
	lhz		r5,0(r3);
	eieio;
	lhz		r6,0(r3);
	eieio;
	lhz		r7,0(r3);
	eieio;
	lhz		r8,0(r3);
	eieio;
	lhz		r9,0(r3);
	eieio;
	lhz		r10,0(r3);
	eieio;
	lhz		r11,0(r3);
	eieio;
	slwi(r0,r0,16);
	slwi(r6,r6,16);
	slwi(r8,r8,16);
	slwi(r10,r10,16);
	or	r0,r0,r5;			// r0 contains first 4
	or	r6,r6,r7;			// r6 contains second 4
	or	r8,r8,r9;			// r8 contains third 4
	or	r10,r10,r11;		// r10 contains fourth 4
	lhz		r5,0(r3);
	eieio;
	lhz		r7,0(r3);
	eieio;
	lhz		r9,0(r3);
	eieio;
	lhz		r11,0(r3);
	eieio;
	slwi(r5,r5,16);
	slwi(r9,r9,16);
	or		r5,r5,r7;		// r5 contains fifth 4
	or		r9,r9,r11;	// r9 contains sixth 4
	lhz		r7,0(r3);
	eieio;
	lhz		r11,0(r3);
	eieio;
	lhz		r12,0(r3);
	eieio;
	slwi(r7,r7,16);
	or		r7,r7,r11;	// r7 contains seventh 4
	lhz		r11,0(r3);
	eieio;
	slwi(r12,r12,16);
	or		r12,r12,r11;	// r12 contains eighth 4
	stwu		r0,4(r4);
	stwu		r6,4(r4);
	stwu		r8,4(r4);
	stwu		r10,4(r4);
	stwu		r5,4(r4);
	stwu		r9,4(r4);
	stwu		r7,4(r4);
	stwu		r12,4(r4);
	bdnz		@loop;
	
	blr;
}

__asm__ void SlamSector16LE(register UReg16LE* regs16,register Ptr dest)
{
	// Load 32 bytes into registers r0,r5-r11 and blast them at once, do this 16 times for the
	// full 512 byte sector
	li		r5,16;
	mtctr	r5;
	subi		r4,r4,4;
@loop:
	// Cache hint for the processor
	li		r12,4;
	dcbtst	r12,r4;
	//dcbz		r12,r4;	// We're writing the full cacheline
	
	// Load the bytes
	lhbrx	r0,r0,r3;
	eieio;
	lhbrx	r5,r0,r3;
	eieio;
	lhbrx	r6,r0,r3;
	eieio;
	lhbrx	r7,r0,r3;
	eieio;
	lhbrx	r8,r0,r3;
	eieio;
	lhbrx	r9,r0,r3;
	eieio;
	lhbrx	r10,r0,r3;
	eieio;
	lhbrx	r11,r0,r3;
	eieio;
	slwi(r0,r0,16);
	slwi(r6,r6,16);
	slwi(r8,r8,16);
	slwi(r10,r10,16);
	or	r0,r0,r5;			// r0 contains first 4
	or	r6,r6,r7;			// r6 contains second 4
	or	r8,r8,r9;			// r8 contains third 4
	or	r10,r10,r11;		// r10 contains fourth 4
	lhbrx	r5,r0,r3;
	eieio;
	lhbrx	r7,r0,r3;
	eieio;
	lhbrx	r9,r0,r3;
	eieio;
	lhbrx	r11,r0,r3;
	eieio;
	slwi(r5,r5,16);
	slwi(r9,r9,16);
	or		r5,r5,r7;		// r5 contains fifth 4
	or		r9,r9,r11;	// r9 contains sixth 4
	lhbrx	r7,r0,r3;
	eieio;
	lhbrx	r11,r0,r3;
	eieio;
	lhbrx	r12,r0,r3;
	eieio;
	slwi(r7,r7,16);
	or		r7,r7,r11;	// r7 contains seventh 4
	lhbrx	r11,r0,r3;
	eieio;
	slwi(r12,r12,16);
	or		r12,r12,r11;	// r12 contains eighth 4
	stwu		r0,4(r4);
	stwu		r6,4(r4);
	stwu		r8,4(r4);
	stwu		r10,4(r4);
	stwu		r5,4(r4);
	stwu		r9,4(r4);
	stwu		r7,4(r4);
	stwu		r12,4(r4);
	bdnz		@loop;
	
	blr;
}

__asm__ void SlamSector32(register UReg32BE* regs32,register Ptr dest)
{
	// Load 32 bytes into registers r0,r5-r11 and blast them at once, do this 16 times for the
	// full 512 byte sector
	li		r5,16;
	mtctr	r5;
	subi		r4,r4,4;
	li		r12,4;
@loop:
	// Cache hint for the processor
	dcbtst	r12,r4;
	//dcbz		r12,r4;	// We're writing the full cacheline
	
	// Load the bytes
	lwz		r0,0(r3);
	eieio;
	lwz		r5,0(r3);
	eieio;
	lwz		r6,0(r3);
	eieio;
	lwz		r7,0(r3);
	eieio;
	lwz		r8,0(r3);
	eieio;
	lwz		r9,0(r3);
	eieio;
	lwz		r10,0(r3);
	eieio;
	lwz		r11,0(r3);
	eieio;
	stwu		r0,4(r4);
	stwu		r5,4(r4);
	stwu		r6,4(r4);
	stwu		r7,4(r4);
	stwu		r8,4(r4);
	stwu		r9,4(r4);
	stwu		r10,4(r4);
	stwu		r11,4(r4);
	bdnz		@loop;
	
	blr;
}

__asm__ void SlamSector32LE(register UReg32LE* regs32,register Ptr dest)
{
	// Load 32 bytes into registers r0,r5-r11 and blast them at once, do this 16 times for the
	// full 512 byte sector
	li		r5,16;
	mtctr	r5;
	subi		r4,r4,4;
	li		r12,4;
@loop:
	// Cache hint for the processor
	dcbtst	r12,r4;
	//dcbz		r12,r4;	// We're writing the full cacheline
	
	// Load the bytes
	lwbrx	r0,r0,r3;
	eieio;
	lwbrx	r5,r0,r3;
	eieio;
	lwbrx	r6,r0,r3;
	eieio;
	lwbrx	r7,r0,r3;
	eieio;
	lwbrx	r8,r0,r3;
	eieio;
	lwbrx	r9,r0,r3;
	eieio;
	lwbrx	r10,r0,r3;
	eieio;
	lwbrx	r11,r0,r3;
	eieio;
	rlwinm	r0,r0,16,0,31;
	rlwinm	r5,r5,16,0,31;
	rlwinm	r6,r6,16,0,31;
	rlwinm	r7,r7,16,0,31;
	rlwinm	r8,r8,16,0,31;
	rlwinm	r9,r9,16,0,31;
	rlwinm	r10,r10,16,0,31;
	rlwinm	r11,r11,16,0,31;
	stwu		r0,4(r4);
	stwu		r5,4(r4);
	stwu		r6,4(r4);
	stwu		r7,4(r4);
	stwu		r8,4(r4);
	stwu		r9,4(r4);
	stwu		r10,4(r4);
	stwu		r11,4(r4);
	bdnz		@loop;
	
	blr;
}
