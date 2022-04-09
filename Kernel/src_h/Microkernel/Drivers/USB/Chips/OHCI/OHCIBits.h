/*
	OHCIBits.h
	Copyright © 1999 by Patrick Varilly

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
	Patrick Varilly		-	Sun, 28 Nov 99		-	Original creation of file, factored this from OHCIDriver.h
											for simplicity
*/

#ifndef __OHCI_BITS__
#define __OHCI_BITS__

// HC register bit masks and shifts (where necessary)
enum
{
	// HcRevision
	kRevisionMask				= 0x000000FF,
	kLegacySupportSupport		= 0x00000100,
	
	// HcControl
	kPeriodicListEnable			= 0x00000004,
	kIsochronousListEnable		= 0x00000008,
	kControlListEnable			= 0x00000010,
	kBulkListEnable				= 0x00000020,
	kFSReset					= 0x00000000,
	kFSResume				= 0x00000040,
	kFSOperational				= 0x00000080,
	kFSSuspend				= 0x000000C0,
	kFSMask					= 0x000000C0,
	kInterruptRouting			= 0x00000100,
	kRWConnected				= 0x00000200,
	kRWEnable					= 0x00000400,
	kControlMask				= 0x000007FF,
	
	// HcCommandStatus
	kHCReset					= 0x00000001,
	kControlListFilled			= 0x00000002,
	kBulkListFilled				= 0x00000004,
	kOwnershipChangeRequest		= 0x00000008,
	
	// Interrupts
	kIntSchedulingOverrun		= 0x00000001,
	kIntWritebackDoneHead		= 0x00000002,
	kIntStartOfFrame			= 0x00000004,
	kIntResumeDetected			= 0x00000008,
	kIntUnrecoverableError		= 0x00000010,
	kIntFrameNumberOverflow	= 0x00000020,
	kIntRootHubStatusChange		= 0x00000040,
	kIntOwnershipChange			= 0x40000000,
	kIntAll					= 0x4000007F,
	kIntMaster				= 0x80000000,
	
	// HcFmInterval
	kFrameIntervalMask			= 0x00003FFF,
	kFullSpeedMPSMask			= 0x7FFF0000,
	kFullSpeedMPSShift			= 16,
	kFrameIntervalToggle		= 0x80000000
	
	// Root Hub bits in OHCIRootHub.h
};

typedef struct OHCIHCCA
{
	UReg32LE					interruptTable[32];
	UReg16LE					frameNumber;
	UReg16LE					pad1;
	UReg32LE					doneHead;
	UReg32LE					reserved[30];
		// This is 120 bytes here, so OHCIHCCA is 256 bytes; it's 116 bytes in the spec; an error in the OHCI spec, maybe?
} OHCIHCCA;

// Endpoint descriptor structure
typedef struct OHCIEndpointDescriptor
{
	UReg32LE					config;
	UReg32LE					tailPtr;
	UReg32LE					headPtr;
	UReg32LE					nextEDPtr;
} OHCIEndpointDescriptor;

// Transfer descriptor structure
typedef struct OHCITransferDescriptor
{
	UReg32LE					config;
	UReg32LE					currentBufferPtr;
	UReg32LE					nextTDPtr;
	UReg32LE					bufferEndPtr;
} OHCITransferDescriptor;

// Endpoint and transfer descriptor bit masks and shifts (where necessary)
enum
{
	// ED config word
	kEDNumberShift			= 7,
	kEDDirectionDepends			= 0x00000000,
	kEDDirectionOut			= 0x00000800,
	kEDDirectionIn				= 0x00001000,
	kEDDirectionMask			= 0x00001800,
	kEDLowSpeed				= 0x00002000,
	kEDSkip					= 0x00004000,
	kEDIsoFormat				= 0x00008000,
	kEDMaxPacketSizeShift		= 16,
	
	// ED flags in headPtr
	kEDHalted					= 0x00000001,
	kEDToggleCarryMask			= 0x00000002,
	
	// TD config word
	kTDBufferRounding			= 0x00040000,
	kTDPIDSetup				= 0x00000000,
	kTDPIDOut					= 0x00080000,
	kTDPIDIn					= 0x00100000,
	kTDDelayInterruptShift		= 21,
	kTDDelayInterruptMask		= 0x7,
	kTDDelayInterruptNone		= (0x7 << kTDDelayInterruptShift),
	kTDDataToggleFromED		= 0x00000000,
	kTDDataToggle0			= 0x02000000,
	kTDDataToggle1			= 0x03000000,
	kTDConditionCodeShift		= 28
	
	// *** Iso TDs pending
};

// Condition codes
typedef enum ConditionCode
{
	kCCNoError				= 0,
	kCCCRC					= 1,
	kCCBitStuffing				= 2,
	kCCDataToggleMismatch		= 3,
	kCCStall					= 4,
	kCCDeviceNotResponding		= 5,
	kCCPIDCheckFailure			= 6,
	kCCUnexpectedPID			= 7,
	kCCDataOverrun			= 8,
	kCCDataUnderrun			= 9,
	kCCReserved1				= 10,
	kCCReserved2				= 11,
	kCCBufferOverrun			= 12,
	kCCBufferUnderrun			= 13,
	kCCNotAccessed			= 14,
	kCCNotAccessed2			= 15
} ConditionCode;

#endif /* __OHCI_BITS__ */