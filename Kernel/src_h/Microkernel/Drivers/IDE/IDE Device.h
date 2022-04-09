/*
	IDE Device.h
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
	Terry Greeniaus	-	Wednesday, 17 June 98	-	Original creation of file
*/
#ifndef __IDE_DEVICE__
#define __IDE_DEVICE__

#include "Block Device.h"

class IDEDevice	:	public BlockDevice
{
	UInt32				_bus;		// See Block Device. for info on this
	UInt32				_deviceID;	// 0 = master, 1 = slave
	UInt32				_unit;		// Unit number in the device, if it supports multiple
	
	class IDEDriver*		driver;
	
	UInt8				sectorsPerTrack;
	UInt8				numHeads;
protected:
	Boolean				removable;
	
			void			commandSync(class IDECommand* cmd);
			void			commandAsync(class IDECommand* cmd);
			
	virtual	Boolean		initialize();	// Return true if succesful
public:
	IDEDevice(class IDEDriver* driver,UInt32 bus,UInt32 id,UInt32 unit);
	virtual ~IDEDevice();
	
	// Stuff for BlockDevice
	virtual	UInt32		bus();
	virtual	UInt32		deviceID();
	virtual	UInt32		unit();
	virtual	UInt32		sectorSize();
	virtual	UInt32		maxSectorTransfer();
	virtual	IOCommand*	readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);
	virtual	IOCommand*	writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);
	
	// Miscellaneous stuff
			Boolean		probe();		// Return true if the device exists
			
	// Mandatory commands
			IDECommand*	executeDriveDiagnostic();
			IDECommand*	formatTrack();
			IDECommand*	identifyDrive(struct IDEIdentifyDriveData* data);
			IDECommand*	initializeDriveParameters(UInt8 sectorsPerTrack,UInt8 numHeads);
			IDECommand*	readSectorsWithRetry(UInt8 sectorCount,UInt32 absoluteSectorNumber,Int8* dest);	// First sector number == 0, increasing from there
			IDECommand*	readSectorsNoRetry();
			IDECommand*	readVerifySectorsWithRetry();
			IDECommand*	readVerifySectorsNoRetry();
			IDECommand*	seek();
			IDECommand*	writeSectorsWithRetry();
			IDECommand*	writeSectorsNoRetry();
			
	// Optional commands
			IDECommand*	acknowledgeMediaChange();
			IDECommand*	postBoot();
			IDECommand*	preBoot();
			IDECommand*	checkPowerMode();
			IDECommand*	doorLock();
			IDECommand*	doorUnlock();
			IDECommand*	idle();
			IDECommand*	idleImmediate();
			IDECommand*	nop();
			IDECommand*	readBuffer();
			IDECommand*	readDMAWithRetry();
			IDECommand*	readDMANoRetry();
			IDECommand*	readLongWithRetry();
			IDECommand*	readLongNoRetry();
			IDECommand*	readMultiple();
			IDECommand*	recalibrate();
			IDECommand*	setFeatures();
			IDECommand*	setMultipleMode();
			IDECommand*	sleep();
			IDECommand*	standby();
			IDECommand*	standbyImmediate();
			IDECommand*	writeBuffer();
			IDECommand*	writeDMAWithRetry();
			IDECommand*	writeDMANoRetry();
			IDECommand*	writeLongWithRetry();
			IDECommand*	writeLongNoRetry();
			IDECommand*	writeMultiple();
			IDECommand*	writeSame();
			IDECommand*	writeVerify();
	
	// ATA-2 Extensions (optional)
			IDECommand*	downloadMicrocode();
			IDECommand*	mediaEject();
	
	// ATA-3 Extensions
			IDECommand*	identifyDeviceDMA();
			IDECommand*	securityDisablePassword();
			IDECommand*	securityErasePrepare();
			IDECommand*	securityEraseUnit();
			IDECommand*	securityFreezeLock();
			IDECommand*	securitySetPassword();
			IDECommand*	securityUnlock();
			IDECommand*	smartDisableOperations();
			IDECommand*	smartEnableDisableAttributeAutosave();
			IDECommand*	smartEnableOperations();
			IDECommand*	smartReadAttributeThreasholds();
			IDECommand*	smartReadAttributeValues();
			IDECommand*	smartReturnStatus();
			IDECommand*	smartSaveAttributeValues();
	
	// ATAPI/ATA-4 Extensions
			IDECommand*	cfaEraseSectors();
			IDECommand*	cfaRequestExtendedErrorCode();
			IDECommand*	cfaTranslateSector();
			IDECommand*	cfaWriteMultipleWithoutErase();
			IDECommand*	cfaWriteSectorsWithoutErase();
			IDECommand*	deviceReset();
			IDECommand*	flushCache();
			IDECommand*	getMediaStatus();
			IDECommand*	identifyPacketDevice(struct ATAPIIdentifyDriveData* data);
			IDECommand*	mediaLock();
			IDECommand*	mediaUnlock();
			IDECommand*	packet();
			IDECommand*	readDMAQueued();
			IDECommand*	readNativeMaxAddress();
			IDECommand*	service();
			IDECommand*	setMaxAddress();
			IDECommand*	smartExecuteOffLineImmediate();
			IDECommand*	smartReadData();
			IDECommand*	writeDMAQueued();
			
	friend void InitIDEVolumes(void);
};

class IDEBus
{
	class IDEBus*	_next;
	UInt32		_busID;
public:
	IDEDevice*	device[2];	// IDE Device 0-1 (master/slave)
	
	IDEBus(IDEDriver* driver,UInt32 busID);
	
	UInt32		bus();	// Returns the bus id (see Block Device.h)
	class IDEBus*	next();	// Returns the next IDE bus
};

void InitIDE(void);

#endif /* __IDE_DEVICE__ */