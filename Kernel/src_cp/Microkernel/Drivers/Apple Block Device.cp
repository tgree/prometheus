/*
	Apple Block Devices.cp
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
#include "Macros.h"
#include "Apple Block Device.h"
#include "Kernel Console.h"
#include "NKVideo.h"
#include "ANSI.h"
#include "NKMemoryManager.h"

static AppleBlockDeviceManager appleBlockDeviceManager;

AppleBlockDeviceManager::AppleBlockDeviceManager()
{
}

AppleBlockDeviceManager::~AppleBlockDeviceManager()
{
}

Boolean AppleBlockDeviceManager::tryToBuildPartitionList(BlockDevice* device)
{
	// First determine if we recognize the partitioning scheme
	UInt32 blockLen = device->sectorSize();
	
	if(blockLen != 512)
	{
		cout << "Bad block size\n";
		return false;
	}
	
	AppleBlock0	block0;
	device->readSectors((Int8*)&block0,0,1);
	
	if(block0.sbSig != appleBlock0Sig)
		return false;
	
	// Check to see that the blocksize of the device is the same as that expected by the apple Block 0 info.  We would also check
	// numBlocks() here, however it seems some devices have "extra" blocks that aren't accounted for in the Block 0 info,
	// but are still returned by numBlocks().
	if(blockLen != block0.sbBlkSize)
	{
		cout << "Apple Block 0 sbBlkSize != scsiDevice->blockLen()\n";
		return false;
	}
	
	// OK, so we think we have an Apple partitioning scheme here!  Build the list!
	ApplePartitionBlockHeader	header;
	device->readSectors((Int8*)&header,1,1);
	
	if(header.pmSig != applePartitionSig)
	{
		cout << "An Apple partition had an invalid partition table signature at block 1!\n";
		return false;
	}
	
	for(Int32 sector = 1;sector < header.pmMapBlkCnt + 1;sector++)
	{
		device->readSectors((Int8*)&header,sector,1);
		if(header.pmSig != applePartitionSig)
			continue;
		
		cout << "\tPartition[" << sector - 1 << "]: " << header.pmPartName << "   -  " << header.pmPartType << "\n";
		BlockDevicePartition* newPartition;
		newPartition = new AppleBlockDevicePartition(&header,device,(strcmp(header.pmPartType,"Apple_HFS") ? unknownPartitionType : appleHFSPartitionType));
		BlockDeviceManager::addPartitionToDevice(device,newPartition);
	}
	
	return true;
}

AppleBlockDevicePartition::AppleBlockDevicePartition(ApplePartitionBlockHeader* _partitionHeader,BlockDevice* _myDevice,UInt32 partitionType):
	BlockDevicePartition(_partitionHeader->pmPyPartStart,_partitionHeader->pmPartBlkCnt,_myDevice)
{
	_partitionType = partitionType;
}

UInt32 AppleBlockDevicePartition::partitionType()
{
	return _partitionType;
}
