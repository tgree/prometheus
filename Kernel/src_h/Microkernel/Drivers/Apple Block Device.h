/*
	Apple Block Device.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __APPLE_BLOCK_DEVICE__
#define __APPLE_BLOCK_DEVICE__

#include "Block Device.h"

typedef struct AppleDiskDriverInfo
{
	UInt32		ddBlock;
	UInt16		ddSize;
	UInt16		ddType;
}AppleDiskDriverInfo;

enum
{
	appleDiskDriver	=	1
};

typedef struct AppleBlock0
{
	UInt16			sbSig;
	UInt16			sbBlkSize;
	UInt32			sbBlkCount;
	UInt16			sbDevType;
	UInt16			sbDevID;
	UInt32			sbData;
	UInt16			sbDrvrCount;
	AppleDiskDriverInfo	ddInfo[61];
	UInt8			pad[6];
}AppleBlock0;

enum
{
	appleBlock0Sig	=	0x4552
};

typedef struct ApplePartitionBlockHeader
{
	UInt16		pmSig;
	UInt16		pmSigPad;
	UInt32		pmMapBlkCnt;
	UInt32		pmPyPartStart;
	UInt32		pmPartBlkCnt;
	ASCII8		pmPartName[32];
	ASCII8		pmPartType[32];
	UInt32		pmLgDataStart;
	UInt32		pmDataCnt;
	UInt32		pmPartStatus;
	UInt32		pmLgBootStart;
	UInt32		pmBootSize;
	UInt32		pmBootAddr;
	UInt32		pmBootAddr2;
	UInt32		pmBootEntry;
	UInt32		pmBootEntry2;
	UInt32		pmBootCksum;
	ASCII8		pmProcessor[16];
	UInt8		pmPad[376];
}ApplePartitionBlockHeader;

enum
{
	applePartitionSig	=	0x504D
};

class AppleBlockDevicePartition	:	public BlockDevicePartition
{
	UInt32		_partitionType;
protected:
	AppleBlockDevicePartition(ApplePartitionBlockHeader* partitionHeader,BlockDevice* myDevice,UInt32 partitionType);
	
	virtual	UInt32		partitionType();
	
	friend class AppleBlockDeviceManager;
};

class AppleBlockDeviceManager	:	public BlockDeviceManager
{
public:
	AppleBlockDeviceManager();
	~AppleBlockDeviceManager();
	
protected:
	virtual	Boolean	tryToBuildPartitionList(BlockDevice* device);
};

#endif /* __APPLE_BLOCK_DEVICE__ */
