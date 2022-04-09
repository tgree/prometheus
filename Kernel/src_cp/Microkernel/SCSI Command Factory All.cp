/*
	SCSI Command Factory All.cp
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
#include "SCSI Commands.h"
#include "SCSI Device.h"
#include "SCSI Command Factory.h"

SCSICommand* SCSICommandFactory::changeDefinition(SCSIDevice* device,UInt32 defParam)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiChangeDefinition.opcode = SCSI_CMD_CHANGE_DEF;
	cmd->cmd.scsiChangeDefinition.lun = device->unit();
	cmd->cmd.scsiChangeDefinition.rsrv1 = 0;
	cmd->cmd.scsiChangeDefinition.rsrv2 = 0;
	cmd->cmd.scsiChangeDefinition.save = 0;
	cmd->cmd.scsiChangeDefinition.rsrv3 = 0;
	cmd->cmd.scsiChangeDefinition.defParam = defParam;
	cmd->cmd.scsiChangeDefinition.rsrv4[0] = 0;
	cmd->cmd.scsiChangeDefinition.rsrv4[1] = 0;
	cmd->cmd.scsiChangeDefinition.rsrv4[2] = 0;
	cmd->cmd.scsiChangeDefinition.rsrv4[3] = 0;
	cmd->cmd.scsiChangeDefinition.paramDataLen = 0;
	cmd->cmd.scsiChangeDefinition.control = 0;
	cmd->cmdLen = 10;
	cmd->transactionFlags = 0;
	cmd->dataLen = 0;
	cmd->targetID = device->deviceID();
	return cmd;
}

SCSICommand* SCSICommandFactory::inquiry(SCSIDevice* device,SCSIInquiryData* data)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiInquiry.opcode = SCSI_CMD_INQUIRY;
	cmd->cmd.scsiInquiry.lun = device->unit();
	cmd->cmd.scsiInquiry.rsrv1 = 0;
	cmd->cmd.scsiInquiry.evpd = 0;
	cmd->cmd.scsiInquiry.pageCode = 0;
	cmd->cmd.scsiInquiry.rsrv2 = 0;
	cmd->cmd.scsiInquiry.allocationLen = 0xFF;
	cmd->cmd.scsiInquiry.control = 0;
	cmd->cmdLen = 6;
	cmd->transactionFlags = STATE_DMA_IN;
	cmd->dataLen = 0xFF;
	cmd->dataPtr = (void*)(data ? (UInt8*)data : cmd->result);
	cmd->targetID = device->deviceID();
	return cmd;
}

SCSICommand* SCSICommandFactory::testUnitReady(SCSIDevice* device)
{
	SCSICommand*	cmd = new SCSICommand;
	cmd->cmd.scsiTestUnitReady.opcode = SCSI_CMD_TEST_UNIT_READY;
	cmd->cmd.scsiTestUnitReady.lun = device->unit();
	cmd->cmd.scsiTestUnitReady.rsrv1 = 0;
	cmd->cmd.scsiTestUnitReady.rsrv2[0] = 0;
	cmd->cmd.scsiTestUnitReady.rsrv2[1] = 0;
	cmd->cmd.scsiTestUnitReady.rsrv2[2] = 0;
	cmd->cmd.scsiTestUnitReady.control = 0;
	cmd->cmdLen = 6;
	cmd->transactionFlags = 0;
	cmd->dataLen = 0;
	cmd->targetID = device->deviceID();
	return cmd;
}

SCSICommand* SCSICommandFactory::requestSense(SCSIDevice* device,scsi_sense_data* data)
{
	SCSICommand*	cmd = new SCSICommand;
	scsi_sense_data*	realData = (data ? data : (scsi_sense_data*)cmd->result);
	cmd->cmd.scsiRequestSense.opcode = SCSI_CMD_REQUEST_SENSE;
	cmd->cmd.scsiRequestSense.lun = device->unit();
	cmd->cmd.scsiRequestSense.rsrv1 = 0;
	cmd->cmd.scsiRequestSense.rsrv2[0] = 0;
	cmd->cmd.scsiRequestSense.rsrv2[1] = 0;
	cmd->cmd.scsiRequestSense.allocationLen = 0xFF;
	cmd->cmd.scsiRequestSense.control = 0;
	cmd->cmdLen = 6;
	cmd->transactionFlags = STATE_DMA_IN;
	cmd->dataLen = 0xFF;
	cmd->dataPtr = (UInt8*)realData;
	cmd->targetID = device->deviceID();
	return cmd;
}
