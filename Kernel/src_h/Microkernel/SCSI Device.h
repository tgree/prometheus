/*
	SCSI Device.h
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
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __SCSI_DEVICE__
#define __SCSI_DEVICE__

#include "Block Device.h"
#include "ShutDown.h"

class SCSIDevice
{
	UInt32				_bus;			// See Block Device.h for info on this
	UInt32				_deviceID;		// The SCSI ID of this device
	UInt32				_unit;			// The unit number within this device, for devices which support multiple units

	class SCSIDriver*		driver;
protected:
	Boolean				removable;
	
			void		commandSync(class SCSICommand* cmd);
			void		commandAsync(class SCSICommand* cmd);
	
public:
	SCSIDevice(SCSIDriver* driver,UInt32 bus,UInt32 id,UInt32 unit);
	virtual ~SCSIDevice();
	
	// Miscellaneous commands
	virtual	UInt32			bus();
	virtual	UInt32			deviceID();
	virtual	UInt32			unit();
	virtual	Boolean			initialize();	// Start up the device, returns true if successful.
			Boolean			probe();		// Returns true if this device exists.
	
	// Mandatory commands
			SCSICommand*		inquiry(struct SCSIInquiryData* data = 0);
			SCSICommand*		requestSense(struct scsi_sense_data* data);
			SCSICommand*		sendDiagnostic();
			SCSICommand*		testUnitReady();
	
	// Optional commands
			SCSICommand*		changeDefinition(UInt32 defParam);
			SCSICommand*		compare();
			SCSICommand*		copy();
			SCSICommand*		copyAndVerify();
			SCSICommand*		logSelect();
			SCSICommand*		logSense();
			SCSICommand*		readBuffer();
			SCSICommand*		receiveDiagnosticResults();
			SCSICommand*		writeBuffer();
	
	// Device-specific commands
			SCSICommand*		modeSelect6();
			SCSICommand*		modeSelect10();
			SCSICommand*		modeSense6();
			SCSICommand*		modeSense10();
	
	// Cool stuff
	static	SCSIDevice*		makeMeIntoARealSCSIDevice(SCSIDevice* oldDev);	// If it returns non-nil, don't forget to delete oldDev!
	
	friend class SCSICommandFactory;
};

struct SCSIDirectAccessDevice	:	public BlockDevice,
							public SCSIDevice,
							public ShutDownHandler
{
protected:
	Boolean		removable;
public:
	SCSIDirectAccessDevice(UInt32 bus,UInt32 deviceID,UInt32 unit,SCSIDriver* driver);
	virtual ~SCSIDirectAccessDevice();
	
	// Stuff for SCSIDevice
	virtual	Boolean			initialize();
	
	// Stuff for BlockDevice
	virtual	UInt32			bus();
	virtual	UInt32			deviceID();
	virtual	UInt32			unit();
	virtual	UInt32			sectorSize();
	virtual	UInt32			maxSectorTransfer();
	virtual	IOCommand*		readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);		// Reads a sector into memory
	virtual	IOCommand*		writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);	// Writes a sector from memory
	
	// Stuff for ShutDownHandler
	virtual	void				shutDown(Boolean isShutDown);
	
	// Mandatory commands
			SCSICommand*		formatUnit();
			SCSICommand*		read6();
			SCSICommand*		read10(Int8* p,UInt32 sector,UInt32 numSectors);
			SCSICommand*		readCapacity(UInt32** size,UInt32** num);
			SCSICommand*		release();
			SCSICommand*		reserve();
	
	// Optional commands
			SCSICommand*		lockUnlockCache();
			SCSICommand*		preFetch();
			SCSICommand*		preventAllowMediumRemoval(Boolean allow);
			SCSICommand*		readDefectData();
			SCSICommand*		readLong();
			SCSICommand*		reassingBlocks();
			SCSICommand*		rezeroUnit();
			SCSICommand*		searchDataEqual();
			SCSICommand*		searchDataHigh();
			SCSICommand*		searchDataLow();
			SCSICommand*		seek6();
			SCSICommand*		seek10();
			SCSICommand*		setLimits();
			SCSICommand*		startStopUnit(Boolean immediate,Boolean loadEject,Boolean start);
			SCSICommand*		synchronizeCache();
			SCSICommand*		verify();
			SCSICommand*		write6();
			SCSICommand*		write10();
			SCSICommand*		writeAndVerify();
			SCSICommand*		writeLong();
			SCSICommand*		writeSame();
};

struct SCSICDRomDevice	:	public BlockDevice,
						public SCSIDevice
{
	SCSICDRomDevice(UInt32 bus,UInt32 deviceID,UInt32 unit,SCSIDriver* driver);
	virtual ~SCSICDRomDevice();
	
	// Stuff for SCSIDevice
	virtual	Boolean			initialize();
	
	// Stuff for BlockDevice
	virtual	UInt32			bus();
	virtual	UInt32			deviceID();
	virtual	UInt32			unit();
	virtual	UInt32			sectorSize();
	virtual	UInt32			maxSectorTransfer();
	virtual	IOCommand*		readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);		// Reads a sector into memory
	virtual	IOCommand*		writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);	// Writes a sector from memory
	
	// Mandatory commands
			SCSICommand*		read10(Int8* p,UInt32 sector,UInt32 numSectors);
			SCSICommand*		readCDROMCapacity();
			SCSICommand*		release();
			SCSICommand*		reserve();
	
	// Optional commands
			SCSICommand*		lockUnlockCache();
			SCSICommand*		modeSelect6();
			SCSICommand*		modeSelect10();
			SCSICommand*		modeSense6();
			SCSICommand*		modeSense10();
			SCSICommand*		pauseResume();
			SCSICommand*		playAudio10();
			SCSICommand*		playAudio12();
			SCSICommand*		playAudioMSF();
			SCSICommand*		playAudioTrackIndex();
			SCSICommand*		playTrackRelative10();
			SCSICommand*		playTrackRelative12();
			SCSICommand*		preFetch();
			SCSICommand*		preventAllowMediumRemoval();
			SCSICommand*		read6();
			SCSICommand*		read12();
			SCSICommand*		readHeader();
			SCSICommand*		readLong();
			SCSICommand*		readTOC();
			SCSICommand*		rezeroUnit();
			SCSICommand*		searchDataEqual10();
			SCSICommand*		searchDataEqual12();
			SCSICommand*		searchDataHigh10();
			SCSICommand*		searchDataHigh12();
			SCSICommand*		searchDataLow10();
			SCSICommand*		searchDataLow12();
			SCSICommand*		seek6();
			SCSICommand*		seek10();
			SCSICommand*		setLimits10();
			SCSICommand*		setLimits12();
			SCSICommand*		startStopUnit(Boolean immediate,Boolean loadEject,Boolean start);
			SCSICommand*		verify10();
			SCSICommand*		verify12();
};

class SCSIBus
{
	class SCSIBus*	_next;
	UInt32		_busID;
public:
	SCSIDevice*	device[7];	// SCSI Devices 0-6 on this bus (7 == CPU)
	
	SCSIBus(SCSIDriver* driver,UInt32 busID);
	
	UInt32		bus();	// Returns the bus id (see Block Device.h)
	class SCSIBus*	next();	// Returns the next SCSI bus	
};

void InitSCSI(void);
void InitSCSIVolumes(void);

#endif	/* __SCSI_BLOCK_DEVICE__ */
