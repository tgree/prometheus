/*
	IDE Device.cp
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
#include "NKThreads.h"
#include "IDE Device.h"
#include "IBM IDE Driver.h"
#include "IDE Commands.h"
#include "ATA Command Factory.h"
#include "NKVideo.h"
#include "Kernel Console.h"
#include "ANSI.h"

#define	SECTOR_FROM_BLOCK(block,secPerTrack,numHeads)	(((block)%(secPerTrack))+1)
#define	HEAD_FROM_BLOCK(block,secPerTrack,numHeads)		((((block)+1-SECTOR_FROM_BLOCK(block,secPerTrack,numHeads))/(secPerTrack))%(numHeads))
#define	CYLINDER_FROM_BLOCK(block,secPerTrack,numHeads)	(((((block)+1-SECTOR_FROM_BLOCK(block,secPerTrack,numHeads))/(secPerTrack))-HEAD_FROM_BLOCK(block,secPerTrack,numHeads))/(numHeads))

void InitIDE(void)
{
	IDEBus*	theIDEBus;
	if(machine.ide0Chip.physicalAddr)
	{
		IDEDriver* driver;
		driver = new IBMIDEDriver("IDE 0 (Internal)",&machine.ide0Chip);
		machine.driverList.enqueue(driver);
		theIDEBus = new IDEBus(driver,internalIDE0Bus);
	}
}

void InitIDEVolumes()
{
	IDEBus*	ideBus = machine.ideBusses;
	if(!ideBus)
		return;
	
	cout << "\nDetermining IDE partition types:\n\n";
	while(ideBus)
	{
		for(Int32 i=0;i<2;i++)
		{
			if(ideBus->device[i]->probe())
			{
				cout << redMsg << "Bus " << ideBus->bus() << ", IDE ID " << i << " Present:\n" << whiteMsg;
				if(!ideBus->device[i]->initialize())
					cout << redMsg << "\t\tBut Down\n" << whiteMsg;
				cout << "\n";
			}
		}
		ideBus = ideBus->next();
	}
}

IDEDevice::IDEDevice(IDEDriver* _driver,UInt32 bus,UInt32 id,UInt32 unit)
{
	driver = _driver;
	_bus = bus;
	_deviceID = id;
	_unit = unit;
	removable = false;
}

IDEDevice::~IDEDevice()
{
}

UInt32 IDEDevice::bus()
{
	return _bus;
}

UInt32 IDEDevice::deviceID()
{
	return _deviceID;
}

UInt32 IDEDevice::unit()
{
	return _unit;
}

UInt32 IDEDevice::sectorSize()
{
	return 512;
}

UInt32 IDEDevice::maxSectorTransfer()
{
	return 256;
}

IOCommand* IDEDevice::readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors)
{
	FatalAssert(p != nil);
	FatalAssert((numSectors > 0) && (numSectors <= 256));
	return readSectorsWithRetry(numSectors,sector,p);
}

IOCommand* IDEDevice::writeSectorsAsync(const Int8* /*p*/,UInt32 /*sector*/,UInt32 /*numSectors*/)
{
	Panic("SCSIDirectAccessDevice::writeSectorsAsync() unimplemented!\n");
	return nil;
}

Boolean IDEDevice::initialize()
{
	IDEIdentifyDriveData		ideIdentifyData;
	ATAPIIdentifyDriveData	atapiIdentifyData;
	IDECommand*			cmd;
	ASCII8*				serialNumber;
	ASCII8*				firmwareRev;
	ASCII8*				modelNumber;
	Boolean				removable;
	ASCII8				buffer[41] = {0};
	UInt32				error = 0;
	
	if(!driver->isAtapi)
	{
		cmd = identifyDrive(&ideIdentifyData);
		serialNumber = ideIdentifyData.serialNumber;
		firmwareRev = ideIdentifyData.firmwareRev;
		modelNumber = ideIdentifyData.modelNumber;
		removable = ideIdentifyData.removable;
	}
	else
	{
		cmd = identifyPacketDevice(&atapiIdentifyData);
		serialNumber = atapiIdentifyData.serialNumber;
		firmwareRev = atapiIdentifyData.firmwareRev;
		modelNumber = atapiIdentifyData.modelNumber;
		removable = atapiIdentifyData.removable;
	}
	CurrThread::blockForIO(cmd);
	
	if(cmd->error)
	{
		cout << "IDE identify drive error: " << (const)cmd->error << "\n";
		error = cmd->error;
	}
	
	delete cmd;
	
	if(!error)
	{
		strncpy(buffer,serialNumber,20);
		cout << "\tSerial Number: " << buffer << "\n";
		strncpy(buffer,firmwareRev,8);
		cout << "\tfirmwareRev: " << buffer  << "\n";
		strncpy(buffer,modelNumber,40);
		cout << "\tmodelNumber: " << buffer << "\n";
		if(removable)
			cout << "\tRemovable\n";
		
		if(driver->isAtapi)
			error = 1;
	}
	
	if(!error)
	{	
		sectorsPerTrack = ideIdentifyData.sectorsPerTrack;
		numHeads = ideIdentifyData.numHeads;
		cout << "\tsectorsPerTrack: " << (UInt32)sectorsPerTrack << "\n";
		cout << "\tnumHeads: " << (UInt32)numHeads << "\n";
		
		cmd = initializeDriveParameters(sectorsPerTrack,numHeads);
		CurrThread::blockForIO(cmd);
		
		if(cmd->error)
		{
			cout << "InitializeDriveParamters error: " << (const)cmd->error << "\n";
			error = cmd->error;
		}
		delete cmd;
	}
	
	if(!error)
	{
		BlockDeviceManager::buildPartitionList(this);
		buildFileSystems();	
	}
	
	return (error == 0);
}

void IDEDevice::commandSync(IDECommand* cmd)
{
	cmd->drive = _deviceID;
	driver->command(cmd);
	CurrThread::blockForIO(cmd);
}

void IDEDevice::commandAsync(IDECommand* cmd)
{
	cmd->drive = _deviceID;
	driver->command(cmd);
}

Boolean IDEDevice::probe()
{
	return ((_deviceID == ideMaster) ? driver->masterPresent : driver->slavePresent);
}

IDECommand* IDEDevice::executeDriveDiagnostic()
{
	IDECommand*	cmd = ATACommandFactory::executeDriveDiagnostic();
	commandAsync(cmd);
	return cmd;
}

IDECommand* IDEDevice::identifyDrive(IDEIdentifyDriveData* data)
{
	IDECommand*	cmd = ATACommandFactory::identifyDrive(data);
	commandAsync(cmd);
	return cmd;
}

IDECommand* IDEDevice::identifyPacketDevice(ATAPIIdentifyDriveData* data)
{
	IDECommand*	cmd = ATACommandFactory::identifyPacketDevice(data);
	commandAsync(cmd);
	return cmd;
}

IDECommand* IDEDevice::initializeDriveParameters(UInt8 sectorsPerTrack,UInt8 numHeads)
{
	IDECommand*	cmd = ATACommandFactory::initializeDriveParameters(sectorsPerTrack,numHeads);
	commandAsync(cmd);
	return cmd;
}

IDECommand* IDEDevice::readSectorsWithRetry(UInt8 sectorCount,UInt32 absoluteSectorNumber,Int8* dest)
{
	
	UInt32	sectorNumber = SECTOR_FROM_BLOCK(absoluteSectorNumber,sectorsPerTrack,numHeads);
	UInt32	head = HEAD_FROM_BLOCK(absoluteSectorNumber,sectorsPerTrack,numHeads);
	UInt32	cylinder = CYLINDER_FROM_BLOCK(absoluteSectorNumber,sectorsPerTrack,numHeads);
	
	UInt32	verifyAbsoluteSector = (cylinder*numHeads + head)*sectorsPerTrack + sectorNumber - 1;
	if(absoluteSectorNumber != verifyAbsoluteSector)
	{
		cout << redMsg << "IDE Sector function error:\n" << whiteMsg;
		cout << "readSectorsWithRetry(" << (UInt32)sectorCount << ", " << absoluteSectorNumber << ", " << (UInt32)dest << ")\n";
		cout << "\tabsoluteSectorNumber(1 based): " << absoluteSectorNumber << "\n";
		cout << "\tverifySectorNumber(1 based): " << verifyAbsoluteSector << "\n";
		cout << "\tsectorNumber = " << sectorNumber << "\n";
		cout << "\thead = " << head << "\n";
		cout << "\tcylinder = " << cylinder << "\n";
		cout << "\tsectorsPerTrack = " << sectorsPerTrack << "\n";
		cout << "\tnumHeads = " << numHeads << "\n\n";
		dout << "LBA to CHS translation failed!\n";
	}
	
	IDECommand*	cmd = ATACommandFactory::readSectorsWithRetry(sectorCount,sectorNumber,cylinder,head,dest);
	commandAsync(cmd);
	return cmd;
}

IDEBus::IDEBus(IDEDriver* driver,UInt32 busID)
{
	_busID = busID;
	_next = nil;
	
	device[0] = new IDEDevice(driver,busID,ideMaster,0);
	device[1] = new IDEDevice(driver,busID,ideSlave,0);
	
	IDEBus*	bus = machine.ideBusses;
	IDEBus*	prev = nil;
	while(bus)
	{
		prev = bus;
		bus = bus->_next;
	}
	if(prev)
		prev->_next = this;
	else
		machine.ideBusses = this;
}

UInt32 IDEBus::bus()
{
	return _busID;
}

IDEBus* IDEBus::next()
{
	return _next;
}
