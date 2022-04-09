/*
	Block Device.cp
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
	Terry Greeniaus	-	Wed, 2 Feb 2000		-	Fixed stupid bug in BlockDevicePartition::compute(MDB)Checksum()
*/
#include "Kernel Types.h"
#include "Block Device.h"
#include "NKThreads.h"
#include "NKVideo.h"
#include "NKDebuggerNub.h"
#include "File Systems.h"
#include "Kernel Console.h"
#include "Apple HFS.h"

static BlockDeviceManager*	blockDeviceManagerList = nil;

BlockDevice::BlockDevice()
{
	partitionList = nil;
}

BlockDevice::~BlockDevice()
{
	BlockDevicePartition*	partition = partitionList;
	while(partition)
	{
		partition = partition->next;
		delete partitionList;
		partitionList = partition;
	}
}

void BlockDevice::readSectors(Int8* p,UInt32 sector,UInt32 numSectors)
{
	IOCommand*	cmd = readSectorsAsync(p,sector,numSectors);
	if(cmd)
		CurrThread::blockForIO(cmd);
	
	delete cmd;
}

void BlockDevice::writeSectors(const Int8* p,UInt32 sector,UInt32 numSectors)
{
	IOCommand*	cmd = writeSectorsAsync(p,sector,numSectors);
	if(cmd)
		CurrThread::blockForIO(cmd);
	
	delete cmd;
}

void BlockDevice::buildFileSystems()
{
	BlockDevicePartition*	currPartition = partitionList;
	while(currPartition)
	{
		FileSystemManager::buildFileSystem(currPartition);
		currPartition = currPartition->next;
	}
}

BlockDevicePartition::BlockDevicePartition(UInt32 _firstSector,UInt32 _numSectors,BlockDevice* _myDevice)
{
	firstSector = _firstSector;
	numSectors = _numSectors;
	myDevice = _myDevice;
	next = nil;
}

BlockDevicePartition::~BlockDevicePartition()
{
}

UInt32 BlockDevicePartition::partitionType()
{
	return unknownPartitionType;
}

UInt32 BlockDevicePartition::sectorSize()
{
	return myDevice->sectorSize();
}

UInt32 BlockDevicePartition::maxSectorTransfer()
{
	return myDevice->maxSectorTransfer();
}

IOCommand* BlockDevicePartition::readSectorsAsync(Int8* p,UInt32 sector,UInt32 n)
{
	if(sector + n > numSectors)
		cout << "Tried to read past end of partition!\n";
	else
		return myDevice->readSectorsAsync(p,sector + firstSector,n);
	
	return nil;
}

IOCommand* BlockDevicePartition::writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 n)
{
	if(sector + n > numSectors)
		cout << "Tried to write past end of partition!\n";
	else
		return myDevice->writeSectorsAsync(p,sector + firstSector,n);
	
	return nil;
}

void BlockDevicePartition::readSectors(Int8* p,UInt32 sector,UInt32 numSectors)
{
	IOCommand*	cmd = readSectorsAsync(p,sector,numSectors);
	if(cmd)
		CurrThread::blockForIO(cmd);
	
	delete cmd;
}

void BlockDevicePartition::writeSectors(const Int8* p,UInt32 sector,UInt32 numSectors)
{
	IOCommand*	cmd = writeSectorsAsync(p,sector,numSectors);
	if(cmd)
		CurrThread::blockForIO(cmd);
	
	delete cmd;
}

UInt32 BlockDevicePartition::computeChecksum(UInt32 sector)
{
	UInt32	sectorData[512/4];
	UInt64	checkSum = 0;
	readSectors((Int8*)&sectorData[0],sector,1);
	for(UInt32 i=0;i<512/4;i++)
		checkSum += sectorData[i];
	while(checkSum & 0xFFFFFFFF00000000)
		checkSum = (checkSum >> 32) + (checkSum & 0x00000000FFFFFFFF);
	
	return checkSum;
}

UInt32 BlockDevicePartition::computeMDBChecksum(UInt32 sector)
{
	UInt32				sectorData[512/4];
	MasterDirectoryBlock*	mdbPtr = (MasterDirectoryBlock*)&sectorData[0];
	UInt32*				checkSumPtr = (UInt32*)&mdbPtr->drVN[0];
	UInt64				checkSum = 0;
	
	readSectors((Int8*)&sectorData[0],sector,1);
	
	checkSum += mdbPtr->drSigWord;
	checkSum += mdbPtr->drCrDate;
	checkSum += mdbPtr->drNmAlBlks;
	checkSum += mdbPtr->drAlBlkSiz;
	for(UInt32 i =0;i<7;i++)	// Checksum the name
		checkSum += *checkSumPtr++;
	
	while(checkSum & 0xFFFFFFFF00000000)
		checkSum = (checkSum >> 32) + (checkSum & 0x00000000FFFFFFFF);
	
	return checkSum;
}

BlockDeviceManager::BlockDeviceManager()
{
	next = blockDeviceManagerList;
	blockDeviceManagerList = this;
}

BlockDeviceManager::~BlockDeviceManager()
{
	Panic("Fill this in later!\n");
}

void BlockDeviceManager::addPartitionToDevice(BlockDevice* device,BlockDevicePartition* partition)
{
	BlockDevicePartition* currPartition = device->partitionList;
	while(currPartition)
	{
		if(!currPartition->next)
			break;
		currPartition = currPartition->next;
	}
	if(currPartition)
		currPartition->next = partition;
	else
		device->partitionList = partition;
}

Boolean BlockDeviceManager::buildPartitionList(BlockDevice* device)
{
	BlockDeviceManager*	currManager = blockDeviceManagerList;
	while(currManager)
	{
		if(currManager->tryToBuildPartitionList(device))
		{
			cout << "\tBuilt partition list!\n";
			return true;
		}
		currManager = currManager->next;
	}
	cout << "\tNo block device manager knew how this was partitioned\n";
	
	return false;
}
