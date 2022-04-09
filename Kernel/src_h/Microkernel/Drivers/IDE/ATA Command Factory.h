/*
	ATA Command Factory.h
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
	Terry Greeniaus	-	Monday, 23 June 98	-	Original creation of file
*/
#ifndef __ATA_COMMAND_FACTORY__
#define __ATA_COMMAND_FACTORY__

struct ATACommandFactory
{
	// Mandatory commands
	static struct IDECommand*	executeDriveDiagnostic();
	static struct IDECommand*	formatTrack();
	static struct IDECommand*	identifyDrive(struct IDEIdentifyDriveData* data);
	static struct IDECommand*	initializeDriveParameters(UInt8 sectorsPerTrack,UInt8 numHeads);
	static struct IDECommand*	readLongWithRetry();
	static struct IDECommand*	readLongNoRetry();
	static struct IDECommand*	readSectorsWithRetry(UInt8 sectorCount,UInt8 sectorNumber,UInt16 cylinder,UInt8 head,Int8* dest);
	static struct IDECommand*	readSectorsNoRetry();
	static struct IDECommand*	readVerifySectorsWithRetry();
	static struct IDECommand*	readVerifySectorsNoRetry();
	static struct IDECommand*	recalibrate();
	static struct IDECommand*	seek();
	static struct IDECommand*	writeLongWithRetry();
	static struct IDECommand*	writeLongNoRetry();
	static struct IDECommand*	writeSectorsWithRetry();
	static struct IDECommand*	writeSectorsNoRetry();
	
	// Optional commands
	static struct IDECommand*	acknowledgeMediaChange();
	static struct IDECommand*	postBoot();
	static struct IDECommand*	preBoot();
	static struct IDECommand*	checkPowerMode();
	static struct IDECommand*	doorLock();
	static struct IDECommand*	doorUnlock();
	static struct IDECommand*	idle();
	static struct IDECommand*	idleImmediate();
	static struct IDECommand*	nop();
	static struct IDECommand*	readBuffer();
	static struct IDECommand*	readDMAWithRetry();
	static struct IDECommand*	readDMANoRetry();
	static struct IDECommand*	readMultiple();
	static struct IDECommand*	setFeatures();
	static struct IDECommand*	setMultipleMode();
	static struct IDECommand*	sleep();
	static struct IDECommand*	standby();
	static struct IDECommand*	standbyImmediate();
	static struct IDECommand*	writeBuffer();
	static struct IDECommand*	writeDMAWithRetry();
	static struct IDECommand*	writeDMANoRetry();
	static struct IDECommand*	writeMultiple();
	static struct IDECommand*	writeSame();
	static struct IDECommand*	writeVerify();
	
	// ATAPI Commands
	static struct IDECommand*	identifyPacketDevice(struct ATAPIIdentifyDriveData* data);
};

#endif /* __ATA_COMMAND_FACTORY__ */