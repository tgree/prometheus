/*
	ATA Command Factory.cp
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
	Terry Greeniaus	-	Monday, 23 June 98		-	Original creation of file
	Terry Greeniaus	-	Wednesday, 26 Oct 99	-	Added preliminary ATAPI support
*/
#include "ATA Command Factory.h"
#include "IDE Commands.h"
#include "IDE Driver.h"

IDECommand* ATACommandFactory::executeDriveDiagnostic()
{
	IDECommand*	cmd = new IDECommand;
	cmd->dataPtr = nil;
	cmd->dataLen = 0;
	cmd->command = 0x90;
	cmd->drive = ideMaster;
	cmd->setRegisters = 0;
	return cmd;
}

IDECommand* ATACommandFactory::identifyDrive(IDEIdentifyDriveData* data)
{
	FatalAssert(data != nil);
	IDECommand*	cmd = new IDECommand;
	cmd->dataPtr = (void*)data;
	cmd->dataLen = 512;
	cmd->command = 0xEC;
	cmd->setRegisters = 0;
	return cmd;
}

IDECommand* ATACommandFactory::identifyPacketDevice(ATAPIIdentifyDriveData* data)
{
	FatalAssert(data != nil);
	IDECommand*	cmd = new IDECommand;
	cmd->dataPtr = (void*)data;
	cmd->dataLen = 512;
	cmd->command = 0xA1;
	cmd->setRegisters = 0;
	return cmd;
}

IDECommand* ATACommandFactory::initializeDriveParameters(UInt8 sectorsPerTrack,UInt8 numHeads)
{
	FatalAssert(((numHeads-1) & 0xF0) == 0);
	IDECommand*	cmd = new IDECommand;
	cmd->dataPtr = nil;
	cmd->dataLen = 0;
	cmd->command = 0x91;
	cmd->setRegisters = (setSectorCountFlag | setHeadFlag);
	cmd->sectorCount = sectorsPerTrack;
	cmd->head = (numHeads - 1);
	return cmd;
}

IDECommand* ATACommandFactory::readSectorsWithRetry(UInt8 sectorCount,UInt8 sectorNumber,UInt16 cylinder,UInt8 head,Int8* dest)
{
	FatalAssert((head & 0xF0) == 0);
	FatalAssert(dest != nil);
	IDECommand*	cmd = new IDECommand;
	cmd->dataPtr = dest;
	cmd->dataLen = 512*sectorCount;
	cmd->command = 0x20;
	cmd->setRegisters = (setSectorCountFlag | setSectorNumberFlag | setCylinderFlag | setHeadFlag);
	cmd->sectorCount = sectorCount;
	cmd->sectorNumber = sectorNumber;
	cmd->cylinder = cylinder;
	cmd->head = head;
	return cmd;
}