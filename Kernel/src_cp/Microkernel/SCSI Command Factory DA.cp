/*
	SCSI Command Factory DA.cp
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
	Terry Greeniaus	-	Friday, 19 June 98	-	Nilled the reserved fields in read10 since new doesn't do this anymore.
*/
#include "SCSI Commands.h"
#include "SCSI Device.h"
#include "SCSI Command Factory.h"

SCSICommand* SCSICommandFactory::startStopUnit(SCSIDevice* device,Boolean immediate,Boolean loadEject,Boolean start)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiStartStop.opcode = SCSI_CMD_START_STOP_UNIT;
	cmd->cmd.scsiStartStop.lun = device->unit();
	cmd->cmd.scsiStartStop.rsrv1 = 0;
	cmd->cmd.scsiStartStop.immed = (immediate != 0);
	cmd->cmd.scsiStartStop.rsrv2[0] = 0;
	cmd->cmd.scsiStartStop.rsrv2[1] = 0;
	cmd->cmd.scsiStartStop.rsrv3 = 0;
	cmd->cmd.scsiStartStop.LoEj = (loadEject != 0);
	cmd->cmd.scsiStartStop.start = (start != 0);
	cmd->cmd.scsiStartStop.control = 0;
	cmd->cmdLen = 6;
	cmd->transactionFlags = 0;
	cmd->dataLen = 0;
	cmd->targetID = device->deviceID();
	return cmd;
}

SCSICommand* SCSICommandFactory::readCapacity(SCSIDevice* device,UInt32** size,UInt32** num)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiReadCapacity.opcode = SCSI_CMD_READ_CAPACITY;
	cmd->cmd.scsiReadCapacity.lun = device->unit();
	cmd->cmd.scsiReadCapacity.rsrv1 = 0;
	cmd->cmd.scsiReadCapacity.relAdr = 0;
	cmd->cmd.scsiReadCapacity.logAddr = 0;
	cmd->cmd.scsiReadCapacity.rsrv2[0] = 0;
	cmd->cmd.scsiReadCapacity.rsrv2[1] = 0;
	cmd->cmd.scsiReadCapacity.rsrv3 = 0;
	cmd->cmd.scsiReadCapacity.pmi = 0;
	cmd->cmd.scsiReadCapacity.control = 0;
	cmd->cmdLen = 10;
	cmd->transactionFlags = STATE_DMA_IN;
	cmd->dataLen = 8;
	cmd->dataPtr = (void*)cmd->result;
	cmd->targetID = device->deviceID();
	*num = (UInt32*)&cmd->result[0];
	*size = (UInt32*)&cmd->result[4];
	return cmd;
}

SCSICommand* SCSICommandFactory::read10(SCSIDevice* device,Int8* p,UInt32 sector,UInt32 numSectors,UInt32 sectorSize)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiRead10.opcode = SCSI_CMD_READ_10;
	cmd->cmd.scsiRead10.lun = device->unit();
	cmd->cmd.scsiRead10.rsrv1 = 0;
	cmd->cmd.scsiRead10.logAddr = sector;
	cmd->cmd.scsiRead10.rsrv2 = 0;
	cmd->cmd.scsiRead10.len = numSectors;
	cmd->cmd.scsiRead10.control = 0;
	cmd->cmdLen = 10;
	cmd->transactionFlags = STATE_DMA_IN;
	cmd->dataLen = sectorSize*numSectors;
	cmd->dataPtr = (UInt8*)p;
	cmd->targetID = device->deviceID();
	return cmd;
}

SCSICommand* SCSICommandFactory::preventAllowMediumRemoval(SCSIDevice* device,Boolean allow)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiPreventAllowRemoval.opcode = SCSI_CMD_PREVENT_ALLOW_REMOVAL;
	cmd->cmd.scsiPreventAllowRemoval.lun = device->unit();
	cmd->cmd.scsiPreventAllowRemoval.rsrv1 = 0;
	cmd->cmd.scsiPreventAllowRemoval.rsrv2[0] = 0;
	cmd->cmd.scsiPreventAllowRemoval.rsrv2[1] = 0;
	cmd->cmd.scsiPreventAllowRemoval.rsrv3 = 0;
	cmd->cmd.scsiPreventAllowRemoval.prevent = !allow;
	cmd->cmd.scsiPreventAllowRemoval.control = 0;
	cmd->cmdLen = 6;
	cmd->transactionFlags = 0;
	cmd->dataLen = 0;
	cmd->targetID = device->deviceID();
	return cmd;
}
