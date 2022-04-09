/*
	Block Device.h
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
#ifndef __BLOCK_DEVICE__
#define __BLOCK_DEVICE__

enum
{
	// For the "bus" in BlockDevice
	internalSCSIBus	=	0,	// Internal SCSI bus (internal/external on 1 bus machines)
	externalSCSIBus	=	1,	// External SCSI bus (undefined on 1 bus machines)
	floppyBus			=	2,	// Floppy drive
	internalIDE0Bus		=	3,	// Internal IDE bus 0
	internalIDE1Bus		=	4,	// Internal IDE bus 1 (Media bay)
	unknownIDEBus		=	5	// Some other IDE bus (could be PCI card bus)
};

class BlockDevice
{
protected:
	class BlockDevicePartition*	partitionList;
	
	BlockDevice();
public:
	virtual ~BlockDevice();
	
	virtual	UInt32			bus() = 0;				// Returns the bus ID
	virtual	UInt32			deviceID() = 0;			// Returns the device ID on the bus
	virtual	UInt32			unit() = 0;				// Returns the unit
	virtual	UInt32			sectorSize() = 0;		// Returns a sector length in bytes
	virtual	UInt32			maxSectorTransfer() = 0;	// Return the maximum number of sectors that can be read in 1 operation
	virtual	class IOCommand*	readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors) = 0;	// Reads sectors
	virtual	class IOCommand*	writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors) = 0;	// Writes sectors
	
			void				readSectors(Int8* p,UInt32 sector,UInt32 numSectors);
			void				writeSectors(const Int8* p,UInt32 sector,UInt32 numSectors);
			
			void				buildFileSystems();
	friend class BlockDeviceManager;
};

enum
{
	// For BlockDevicePartition partitionType
	unknownPartitionType	=	0,
	appleHFSPartitionType	=	1
};

class BlockDevicePartition
{
protected:
	class BlockDevicePartition*	next;
	UInt32					firstSector;
	UInt32					numSectors;
	
	virtual ~BlockDevicePartition();
public:
	BlockDevice*				myDevice;
	
	BlockDevicePartition(UInt32 firstSector,UInt32 numSectors,BlockDevice* myDevice);
	
	virtual	UInt32		partitionType();	// Return unknownPartitionType by default
	
			UInt32		sectorSize();
			UInt32		maxSectorTransfer();
	virtual	IOCommand*	readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);
	virtual	IOCommand*	writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);
	
			void			readSectors(Int8* p,UInt32 sector,UInt32 numSectors);
			void			writeSectors(const Int8* p,UInt32 sector,UInt32 numSectors);
	
			UInt32		computeChecksum(UInt32 sector);		// Computes a checksum for the sector - used to identify the kernel booter volume
			UInt32		computeMDBChecksum(UInt32 sector);	// Computes an MDB checksum for the sector - new way of identifying boot volume
	friend class BlockDevice;
	friend class BlockDeviceManager;
};

class BlockDeviceManager
{
	// Make a BlockDeviceManager subclass for every partitioning scheme.  Currently we only have one for an Apple partitioning scheme.
	// Note that this is how the partitions are organized on a drive, not how an individual partition works.
	BlockDeviceManager*	next;
protected:
	BlockDeviceManager();
	virtual ~BlockDeviceManager();
	
	virtual	Boolean	tryToBuildPartitionList(BlockDevice* device) = 0;	// Build the partition list for this device.  Return true if you recognize it, false otherwise.

	static	void		addPartitionToDevice(BlockDevice* device,BlockDevicePartition* partition);	// Call this from your BlockDeviceManager when you find a partition to add
public:
	static	Boolean	buildPartitionList(BlockDevice* device);
};

#endif /*__BLOCK_DEVICE__*/