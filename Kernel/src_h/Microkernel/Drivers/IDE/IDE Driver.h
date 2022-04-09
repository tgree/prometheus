/*
	IDE Driver.h
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
	Terry Greeniaus	-	Tuesday, 23 June 98	-	Original creation of file
*/
#ifndef __IDE_DRIVER__
#define __IDE_DRIVER__

#include "NKMachineInit.h"
#include "Driver.h"
#include "External Interrupt.h"

enum
{
	ideMaster	=	0,
	ideSlave	=	1
};

class IDEDriver	:	public IOCommandDriver,
				public InterruptHandler
{
	class IDECommand*	currCommand;
	Boolean			masterPresent;
	Boolean			slavePresent;
	Boolean			isAtapi;
	Boolean			justDidDRDYHoser;
	
	// Don't overload this
	virtual	void	startAsyncIO(IOCommand* cmd);
	
	// Don't overload this either (I think)
	virtual	void	handleInterrupt();
	
	// Private stuff
			Boolean	waitForBusyClear(UInt8 setBits = 0);
			void		printResetResults();
			void		selectDrive(Int8 masterOrSlave);
protected:
	IDEDriver(ConstASCII8Str name,UInt32 int1,UInt32 int2 = PMAC_DEV_NO_INT);
	virtual ~IDEDriver();
	
	// Stuff for driver - you should overload these, but be sure to call IDEDriver::initialize() at
	// the ___end___ of your routine.
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// Read accessors - you must provide these
	virtual	UInt8	readError() = 0;
	virtual	UInt8	readSectorCount() = 0;
	virtual	UInt8	readSector() = 0;
	virtual	UInt8	readCylinderLow() = 0;
	virtual	UInt8	readCylinderHigh() = 0;
	virtual	UInt8	readHead() = 0;
	virtual	UInt8	readStatus() = 0;
	
	// Write accessors - you must provide these too
	virtual	void		writeData16(UInt16 data) = 0;	// Writes to the 16-bit data register
	virtual	void		writeFeatures(UInt8 data) = 0;
	virtual	void		writeSectorCount(UInt8 data) = 0;
	virtual	void		writeSector(UInt8 data) = 0;
	virtual	void		writeCylinderLow(UInt8 data) = 0;
	virtual	void		writeCylinderHigh(UInt8 data) = 0;
	virtual	void		writeHead(UInt8 data) = 0;
	virtual	void		writeCommand(UInt8 data) = 0;
	virtual	void		writeDeviceControl(UInt8 data) = 0;
	
	// For slamming a 512 byte sector for the IDE data register into memory.  Called when doing polled IO.
	virtual	void		slamRead512(Ptr destLogical) = 0;
	virtual	void		slamRead512LE(Ptr destLogical) = 0;	// Little-endian version
public:
			void		command(class IDECommand* cmd);
	friend class IDEDevice;
};

// You should probably call these from your slamRead512() routine.  Highly optimized.
void SlamSector16(register UReg16BE* regs16,register Ptr dest);	// Reads 512 bytes from regs16 and puts it in dest
void SlamSector16LE(register UReg16LE* regs16,register Ptr dest);
void SlamSector32(register UReg32BE* regs32,register Ptr dest);	// Reads 512 bytes from regs32 and puts it in dest
void SlamSector32LE(register UReg32LE* regs16,register Ptr dest);

#endif /* __IDE_DRIVER__ */