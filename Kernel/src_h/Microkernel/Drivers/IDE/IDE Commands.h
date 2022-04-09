/*
	IDE Commands.h
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
#ifndef __IDE_COMMANDS__
#define __IDE_COMMANDS__

#include "Driver.h"

enum
{
	// Flags for IDECommand::setRegisters
	setFeaturesFlag			=	0x01,
	setSectorCountFlag		=	0x02,
	setSectorNumberFlag	=	0x04,
	setCylinderFlag			=	0x08,
	setHeadFlag			=	0x10
};

struct IDECommand	:	public IOCommand
{
	void*				dataPtr;			// A pointer to the data for any DMA commands
	UInt32				dataLen;			// The length of data to transfer for any DMA commands
	UInt8				command;
	UInt8				drive;
	volatile UInt8			error;
	volatile UInt8			status;
	
	Boolean				setRegisters;
	UInt8				features;
	UInt8				sectorCount;
	UInt8				sectorNumber;
	UInt16				cylinder;
	UInt8				head;
	
	virtual	UInt32		ioError();
};

#pragma options align=packed
typedef struct IDEIdentifyDriveData
{
	// Offset 0
	UInt16		rsrv1						:	1;
	UInt16		formatSpeedToleranceGapRequired	:	1;
	UInt16		trackOffsetOptionAvailable		:	1;
	UInt16		dataStrobeOffetOptionAvailable		:	1;
	UInt16		rotationalSpeedTolerance			:	1;
	UInt16		transfer10Mbs					:	1;
	UInt16		transfer5Mbs					:	1;
	UInt16		transferLess5Mbs				:	1;
	
	// Offset 1
	UInt16		removable						:	1;
	UInt16		fixed							:	1;
	UInt16		spindleMotorControlImplemented	:	1;
	UInt16		headSwitchTimeLong				:	1;
	UInt16		notMFMEncoded					:	1;
	UInt16		softSectored					:	1;
	UInt16		hardSectored					:	1;
	UInt16		rsrv2						:	1;
	
	// Offset 2
	UInt16		numCylinders;
	UInt16		rsrv3;
	UInt16		numHeads;
	UInt16		unformattedBytesPerTrack;
	UInt16		unformattedBytesPerSector;
	UInt16		sectorsPerTrack;
	UInt16		vendorUnique1[3];
	
	// Offset 20
	ASCII8		serialNumber[20];
	
	// Offset 40
	UInt16		bufferType;
	UInt16		bufferSize;
	UInt16		eccBytesAvailable;
	
	// Offset 46
	ASCII8		firmwareRev[8];
	
	// Offset 54
	ASCII8		modelNumber[40];
	
	// Offset 94
	UInt8		vendorUnique2;
	UInt8		readWriteMultipleSectorsPerInterrupt;
	UInt16		canPerform32bitIO;
	
	// Offset 98
	UInt8		rsrv4		:	6;
	UInt8		lba			:	1;
	UInt8		dma			:	1;
	UInt8		rsrv5;
	UInt16		rsrv6;
	UInt8		pioCycleTimingMode;
	UInt8		vendorUnique3;
	UInt8		dmaCycleTimingMode;
	UInt8		vendorUnique4;
	UInt16		rsrv7		:	15;
	UInt16		nextFieldsValid	:	1;
	UInt16		currNumCylinders;
	UInt16		currNumHeads;
	UInt16		currNumSectors;
	UInt16		currCapacitySectors;
	
	// Offset 118
	UInt8		rsrv8			:	7;
	UInt8		multipleSectorValid	:	1;
	UInt8		multipleSectorCount;
	
	UInt32		totalAddressableSectors;
	UInt8		singleWordDMATransferModeActive;
	UInt8		singleWordDMAModesSupported;
	UInt8		multiWordDMATransferModeActive;
	UInt8		multiWordDMAModesSupported;
	
	UInt8		rsrv9[148];
	UInt8		vendorUnique5[64];
	UInt8		rsrv10[192];
}IDEIdentifyDriveData;

typedef struct ATAPIIdentifyDriveData
{
	// Offset 0
	UInt16		deviceType	:	2;
	UInt16		rsrv1		:	1;
	UInt16		packetSet		:	5;
	UInt16		removable		:	1;
	UInt16		drqInfo		:	2;
	UInt16		rsrv2		:	3;
	UInt16		packetSize	:	2;
	
	// Offset 2
	UInt16		rsrv3[9];
	
	// Offset 20
	ASCII8		serialNumber[20];
	
	// Offset 40
	UInt16		rsrv4[3];
	
	// Offset 46
	ASCII8		firmwareRev[8];
	
	// Offset 54
	ASCII8		modelNumber[40];
	
	// Offset 94
	UInt16		rsrv5[2];
	
	// Offset 98
	UInt16		interleavedDMA	:	1;
	UInt16		commandQueueing	:	1;
	UInt16		overlapOperation	:	1;
	UInt16		ataSRST			:	1;
	UInt16		iordy			:	1;
	UInt16		mayDisableIORDY	:	1;
	UInt16		lba				:	1;
	UInt16		dma				:	1;
	UInt16		vendorSpecific1	:	8;
	
	// Offset 100
	UInt16		rsrv6;
	
	// Offset 102
	UInt16		pioTranfserMode	:	8;
	UInt16		vendorSpecific2	:	8;
	
	// Offset 104
	UInt16		rsrv7;
	
	// Offset 106
	UInt16		rsrv8			:	13;
	UInt16		fieldsValid1		:	1;
	UInt16		fieldsValid2		:	1;
	UInt16		fieldsValid3		:	1;
	
	// Offset 108
	UInt16		rsrv9[9];
	
	// Offset 126
	UInt16		rsrv10				:	5;
	UInt16		multiwordDMA2		:	1;
	UInt16		multiwordDMA1		:	1;
	UInt16		multiwordDMA0		:	1;
	UInt16		rsrv11				:	5;
	UInt16		multiwordDMA2Supported	:	1;
	UInt16		multiwordDMA1Supported	:	1;
	UInt16		multiwordDMA0Supported	:	1;
	
	// Offset 128
	UInt16		rsrv12				:	8;
	UInt16		advancedPIOModes		:	8;
	
	// Offset 130
	UInt16		minimumMultiwordDMATime;
	
	// Offset 132
	UInt16		reccMultiwordDMATime;
	
	// Offset 134
	UInt16		minimumPIOTimeNoFlow;
	
	// Offset 136
	UInt16		minimumPIOTimeFlow;
	
	// Offset 138
	UInt16		rsrv13[2];
	
	// Offset 142
	UInt16		busReleaseTypicalNS;
	
	// Offset 144
	UInt16		typicalServiceBSYClearNS;
	
	// Offset 146
	UInt16		rsrv14[2];
	
	// Offset 150
	UInt16		rsrv15		:	11;
	UInt16		maxQueueDepth	:	5;
	
	// Offset 152
	UInt16		rsrv16[4];
	
	// Offset 160
	UInt16		rsrv17				:	11;
	UInt16		supportsATA_ATAPI4	:	1;
	UInt16		supportsATA3			:	1;
	UInt16		supportsATA2			:	1;
	UInt16		supportsATA1			:	1;
	UInt16		rsrv18				:	1;
	
	// Offset 162
	UInt16		minorVersionNumber;
	
	// Offset 164
	UInt16		rsrv19					:	1;
	UInt16		nopSupported				:	1;
	UInt16		readBufferSupported			:	1;
	UInt16		writeBufferSupported		:	1;
	UInt16		rsrv20					:	1;
	UInt16		hostProtectAreaSupported	:	1;
	UInt16		deviceResetSupported		:	1;
	UInt16		serviceInterruptSupported	:	1;
	UInt16		releaseInterruptSupported		:	1;
	UInt16		lookAheadSupported			:	1;
	UInt16		writeCacheSupported		:	1;
	UInt16		packetSetSupported			:	1;
	UInt16		powerMgmtSetSupported		:	1;
	UInt16		removableMediaSetSupported	:	1;
	UInt16		securityModeSetSupported	:	1;
	UInt16		smartSetSupported			:	1;
	
	// Offset 166
	UInt16		zero1						:	1;
	UInt16		one1							:	1;
	UInt16		rsrv21						:	9;
	UInt16		removableMediaStatusSetSupported	:	1;
	UInt16		rsrv22						:	3;
	UInt16		downloadMicrocodeSupported		:	1;
	
	// Offset 168
	UInt16		zero2	:	1;
	UInt16		one2		:	1;
	UInt16		rsrv23	:	14;
	
	// Offset 170
	UInt16		sameAsOffset164;
	
	// Offset 172
	UInt16		sameAsOffset166;
	
	// Offset 174
	UInt16		sameAsOffset168;
	
	// Offset 176
	UInt16		rsrv24			:	5;
	UInt16		ultraDMA2Selected	:	1;
	UInt16		ultraDMA1Selected	:	1;
	UInt16		ultraDMA0Selected	:	1;
	UInt16		rsrv25			:	5;
	UInt16		ultraDMA2Supported	:	1;
	UInt16		ultraDMA1Supported	:	1;
	UInt16		ultraDMA0Supported	:	1;
	
	// Offset 178
	UInt16		rsrv26[38];
	
	// Offset 256
	UInt16		rsrv27				:	7;
	UInt16		securityLevel			:	1;
	UInt16		rsrv28				:	2;
	UInt16		enhancedEraseSupported	:	1;
	UInt16		securityCountExpired	:	1;
	UInt16		securityFrozen			:	1;
	UInt16		securityLocked			:	1;
	UInt16		securityEnabled			:	1;
	UInt16		securitySupported		:	1;
	
	// Offset 258
	UInt16		vendorSpecific3[31];
	
	// Offset 320
	UInt16		rsrv29[96];
}ATAPIIdentifyDriveData;

#pragma options align=reset

#endif /* __IDE_COMMANDS__ */