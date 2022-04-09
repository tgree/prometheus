/*
	SCSI Driver.cp
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
	Terry Greeniaus	-	Friday, 19 June 98	-	Changed invalidation stuff for DMA in command() to fix
											some cache sync problems.  Also made it do invalidate instead of
											flush the cache.
*/
#include "Kernel Types.h"
#include "Macros.h"
#include "SCSI Driver.h"
#include "SCSI Commands.h"
#include "NKDebuggerNub.h"
#include "Kernel Console.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "Memory Utils.h"

#define	DISP_STATUS	0
#if		DISP_STATUS
#define	STATUS(str)	cout << str
#else
#define	STATUS(str)	do{}while(0)
#endif

ConstASCII8Str	scsiPhaseName[]	=	{	"dataOutPhase",
										"dataInPhase",
										"commandPhase",
										"statusPhase",
										"selectionPhase",
										"reselectionPhase",
										"msgOutPhase",
										"msgInPhase",
										"arbitrationPhase",
										"requestStatusPhase",
										"busFreePhase"
									};

SCSIDriver::SCSIDriver(ConstASCII8Str name):
	IOCommandDriver(name)
{
	preCountBufferPtr = (Int8*)ROUND_UP(32,(Int32)preCountBuffer);
	preCountBufferPtrPhys = (Int8*)NKGetPhysical(preCountBufferPtr,PROCESS_KERNEL);
	
	currPhase = busFreePhase;
	currCommand = nil;
}

SCSIDriver::~SCSIDriver()
{
}

void SCSIDriver::scsiError(UInt32 err)
{
	switch(err)
	{
		case scsiUnknownErr:
			STATUS("SCSIDriver::scsiError(scsiUnknownErr)\n");
			Panic("SCSI unknown error!\n");
		break;
		case scsiResetErr:
			STATUS("SCSIDriver::scsiError(scsiResetErr)\n");
			currPhase = busFreePhase;
			freeBus();
		break;
		case scsiDisconnectErr:
			STATUS("SCSIDriver::scsiError(scsiDisconnectErr)\n");
			if(currPhase != busFreePhase)
			{
				currCommand->error = SCSI_RET_DEVICE_DOWN;
				currPhase = busFreePhase;
				freeBus();
				//dumpCommand(currCommand);
			}
		break;
		case scsiParityErr:
			STATUS("SCSIDriver::scsiError(scsiParityErr)\n");
			Panic("SCSI parity error!\n");
		break;
		case scsiSelectionTimeoutErr:
			STATUS("SCSIDriver::scsiError(scsiSelectionTimeoutErr)\n");
			currCommand->error = SCSI_RET_DEVICE_DOWN;
			currPhase = busFreePhase;
			freeBus();
		break;
		case scsiLostArbErr:
			STATUS("SCSIDriver::scsiError(scsiLostArbErr)\n");
			Panic("SCSI lost arbitration error!\n");
		break;
		case scsiReselectionErr:
			STATUS("SCSIDriver::scsiError(scsiReselectionErr)\n");
			Panic("SCSI reselection error!\n");
		break;
		case scsiDontSelectMeErr:
			STATUS("SCSIDriver::scsiError(scsiDontSelectMeErr)\n");
			Panic("Some SCSI freak tried to select us!\n");
		break;
		case scsiBusDownErr:
			currCommand->error = SCSI_RET_DEVICE_DOWN;
			currPhase = busFreePhase;
			scsiReady();
		break;
	}
}

void SCSIDriver::scsiReady()
{
	STATUS("SCSIDriver::scsiReady() - " << scsiPhaseName[currPhase] << "\n");
	
	Int8	scsiIdentifyMsg = SCSI_IDENTIFY;
	
	switch(currPhase)
	{
		case arbitrationPhase:
			currPhase = selectionPhase;
			selection(currCommand->targetID);
		break;
		case selectionPhase:
			currPhase = msgOutPhase;
			messageOut(&scsiIdentifyMsg,1,false);
		break;
		case msgOutPhase:
			currPhase = commandPhase;
			commandOut((Int8*)&currCommand->cmd,currCommand->cmdLen);
		break;
		case commandPhase:
			if(currCommand->remainData)
			{
				if(currCommand->transactionFlags & STATE_DMA_IN)
				{
					currPhase = dataInPhase;
					transferDMAIn();
				}
				else
				{
					currPhase = dataOutPhase;
					transferDMAOut();
				}
			}
			else
			{
				currPhase = statusPhase;
				handleStatusPhase();
			}
		break;
		case dataInPhase:
			transferDMAInCompleted();
			if(currCommand->remainData)
				transferDMAIn();
			else
			{
				currPhase = statusPhase;
				handleStatusPhase();
			}
		break;
		case statusPhase:
			handleStatusPhase();
		break;
		case msgInPhase:
			handleMessageInPhase();
		break;
		case busFreePhase:
			currCommand->doneIO();
			startAsyncIO(dequeue());
		break;
	}
}

void SCSIDriver::scsiPhaseChanged(UInt32 newPhase)
{
	STATUS("SCSIDriver::scsiPhaseChanged(): " << scsiPhaseName[currPhase] << " - " << scsiPhaseName[newPhase] << "\n");
	switch(currPhase)
	{
		case busFreePhase:
			switch(newPhase)
			{
				case arbitrationPhase:
					currPhase = newPhase;
				break;
				default:
					Panic("busFreePhase didn't switch to arbitration phase!\n");
				break;
			}
		break;
		case arbitrationPhase:
			switch(newPhase)
			{
				case selectionPhase:
					scsiReady();
				break;
				default:
					Panic("arbitrationPhase didn't switch to (re)selection phase!\n");
				break;
			}
		break;
		case selectionPhase:
			switch(newPhase)
			{
				case msgOutPhase:
					scsiReady();
				break;
				default:
					Panic("selectionPhase didn't switch to message out phase!\n");
				break;
			}
		break;
		case msgOutPhase:
			switch(newPhase)
			{
				case commandPhase:
					scsiReady();
				break;
				default:
					Panic("msgOutPhase didn't switch to command phase!\n");
				break;
			}
		break;
		case commandPhase:
			switch(newPhase)
			{
				case dataInPhase:
				case dataOutPhase:
				case statusPhase:
					scsiReady();
				break;
				default:
					Panic("commandPhase didn't switch to a good phase!\n");
				break;
			}
		break;
		case dataInPhase:
			transferDMAInCompleted();
			switch(newPhase)
			{
				case statusPhase:
					currPhase = statusPhase;
					handleStatusPhase();
				break;
				default:
					Panic("dataInPhase didn't switch to status phase!\n");
				break;
			}
		break;
		case dataOutPhase:
			transferDMAOutCompleted();
			switch(newPhase)
			{
				case statusPhase:
					currPhase = statusPhase;
					handleStatusPhase();
				break;
				default:
					Panic("dataOutPhase didn't switch to status phase!\n");
				break;
			}
		break;
		case statusPhase:
			switch(newPhase)
			{
				case msgInPhase:
					currPhase = newPhase;
					handleMessageInPhase();
				break;
				default:
					Panic("statusPhase didn't switch to message in phase!\n");
				break;
			}
		break;
		case msgInPhase:
			switch(newPhase)
			{
				case busFreePhase:
					currPhase = newPhase;
					scsiReady();
				break;
				default:
					Panic("msgInPhase didn't switch to bus free phase!\n");
				break;
			}
		break;
	}
}

UInt32 SCSIDriver::scsiID()
{
	return 7;
}

void SCSIDriver::command(SCSICommand* cmd)
{
	FatalAssert(cmd != nil);
	
	cmd->msgLen = 0;
	cmd->remainMsg = 0;
	cmd->remainData = cmd->dataLen;
	cmd->error = -1;
	
	// Invalidate all parts of the DMA transfer
	UInt32		startInval;
	UInt32		endInval;
	if(cmd->transactionFlags & STATE_DMA_IN)
	{
		/*
		if((Int32)cmd->dataPtr & 0x0000001F)
			_dcbf(cmd->dataPtr);
		if(((Int32)cmd->dataPtr + cmd->dataLen - 1) & 0x0000001F)
			_dcbf((void*)((Int32)cmd->dataPtr + cmd->dataLen - 1));
		*/
		startInval = ROUND_UP(32,(UInt32)cmd->dataPtr);
		endInval = ROUND_DOWN(32,(UInt32)cmd->dataPtr + cmd->dataLen);
		
		// Must do the invalidate first (I think).  The issue is that sometimes residual
		// bytes are left in a chip's FIFO.  Those bytes will be transferred through the
		// processor rather than through DMA.  If one of those cache lines were accessed
		// before the invalidate had occurred, then we would lose that FIFO data.  Thus it
		// must be flushed BEFORE we start any DMA transfers, thus it must be flushed
		// before interrupts start handling this command.  There's still a catch though -
		// the driver handles misaligned and != 32 byte transfers through the processor
		// rather than DMA.  I THINK that most SCSI chips only leave bytes in the FIFO if
		// it is an odd byte transfer, in which case it's handled through the processor anyhow
		// and there are no cache sync issues.  But this is just for safety right now...
		for(UInt32 i=startInval;i<endInval;i += 32)
			_dcbi((void*)i);
	}
	
	enqueue(cmd);
}

void SCSIDriver::startAsyncIO(IOCommand* cmd)
{
	if(cmd)
	{
		STATUS("--------------------------\nSCSIDriver::startAsyncIO()\n");
		currCommand = static_cast<SCSICommand*>(cmd);
		requestedStatus = false;
		currPhase = arbitrationPhase;
		arbitrate();
	}
}

void SCSIDriver::transferDMAIn()
{
	Int8*		destPtr;
	UInt32	len;
	
	if(!currCommand->remainData)
		return;
	
	// DMA transfers must begin on a 32 byte boundary for cache sync and DMA chip purposes
	if((UInt32)currCommand->dataPtr & 0x0000001F)
	{
		// This is a misaligned transfer.  Align it by doing a partial DMA to our (aligned)
		// 32 byte buffer and MemCopying it to the destination in transferDMAInCompleted().
		// This transfer is always less than 32 bytes.
		len = (32 - ((UInt32)currCommand->dataPtr & 0x000001F));
		len = (len > currCommand->remainData ? currCommand->remainData : len);
		destPtr = preCountBufferPtrPhys;
		_dcbi(preCountBufferPtr);
	}
	// Partial cache line transfers cannot be DMAed directly to the line, for cache sync purposes
	else if(currCommand->remainData < 32)
	{
		// This is a partial transfer.  Transfer it to our 32 byte buffer and MemCopy it to the
		// final destination in transferDMAInCompleted().
		// This transfer is always less than 32 bytes
		len = currCommand->remainData;
		destPtr = preCountBufferPtrPhys;
		_dcbi(preCountBufferPtr);
	}
	else
	{
		// This is a 32 byte aligned transfer, which is also >= 32 bytes.  We copy as many full
		// cache lines as we can, but are sure NOT to overlap into the last cache line.
		// This transfer is always >= 32 bytes
		
		// Figure out the max contig physical memory we can DMA to
		UInt32 maxContig = NKMaxContig(currCommand->dataPtr,currCommand->processID);
		
		// Make it a multiple of 32 bytes (rounding down)
		len = (currCommand->remainData > maxContig ? maxContig : currCommand->remainData) & 0xFFFFFFE0;
		
		// Don't copy more than the chip can handle!
		len = (len > 65536 ? 65536 : len);
		
		// Set up the destination pointer
		destPtr = (Int8*)NKGetPhysical((void*)currCommand->dataPtr,currCommand->processID);
	}
	
	currPhase = dataInPhase;
	currCommand->requestData = len;
	currCommand->transactionFlags |= STATE_DMA_RUNNING;
	
	STATUS("SCSIDriver::transferDMA: dest = " << (UInt32)destPtr << ", len = " << len << "\n");
	dmaIn(destPtr,len);
}

void SCSIDriver::transferDMAInCompleted()
{
	UInt32 transferLen = currCommand->requestData - getDataTransferLen();
	currCommand->remainData -= transferLen;
	
	{
		ProcessWindow	window(currCommand->process);
		if(currCommand->requestData < 32)
		{
			// This was transferred via our 32 byte buffer, either to align the next transfer to the
			// start of a cache line, or to do < 32 byte transfer
			MemCopy(preCountBufferPtr,currCommand->dataPtr,transferLen);
			//_dcbf((void*)((UInt32)currCommand->dataPtr + transferLen - 1));
		}
		
		currCommand->dataPtr = (void*)((Int8*)currCommand->dataPtr + transferLen);
		
		UInt32 residual = readResidualDMAIn((Int8*)currCommand->dataPtr);
		STATUS("Read residual " << residual << "\n");
	}
	
	STATUS("SCSIDriver::transferDMAInCompleted(), transferLen = " << transferLen << "\n");
	
	currCommand->transactionFlags &= ~STATE_DMA_RUNNING;
}

void SCSIDriver::transferDMAOut()
{
	Panic("SCSIDriver::transferDMAOut() unimplemented!\n");
}

void SCSIDriver::transferDMAOutCompleted()
{
	Panic("SCSIDriver::transferDMAOutCompleted() unimplemented!\n");
}

void SCSIDriver::handleStatusPhase()
{
	Assert(currPhase == statusPhase);
	
	if(!requestedStatus)
	{
		requestedStatus = true;
		requestStatus();
	}
	else
	{
		Int8 statusByte = status();
		
		if(((statusByte >> 1) & 0x1F) != SCSI_ST_GOOD)
		{
			currCommand->error = ( ((statusByte >> 1) & 0x1F) == SCSI_ST_BUSY) ? SCSI_RET_RETRY : SCSI_RET_NEED_SENSE;
			STATUS("  Status: unsuccesful\n");
		}
		else
		{
			currCommand->error = SCSI_RET_SUCCESS;
			STATUS("  Status: success\n");
		}
		
		currPhase = msgInPhase;
		requestMessage();
	}
}

void SCSIDriver::handleMessageInPhase()
{
	switch(messageIn())
	{
		case SCSI_DISCONNECT:
			STATUS("SCSIDriver::handleMessagePhase() - SCSI Disconnect\n");
		break;
		case SCSI_COMMAND_COMPLETE:
			STATUS("SCSIDriver::handleMessagePhase() - SCSI command complete\n");
		break;
		default:
			Panic("Unknown message in!\n");
		break;
	}
	
	acceptMessage();
	currPhase = busFreePhase;
	freeBus();
}

void SCSIDriver::dumpCommand(SCSICommand* cmd)
{
	cout << "SCSI Command Dump:\n   ";
	switch(cmd->cmd.scsi6.opcode)
	{
		case SCSI_CMD_CHANGE_DEF:				cout << "Change definition";		break;
		case SCSI_CMD_INQUIRY:					cout << "Inquiry";				break;
		case SCSI_CMD_TEST_UNIT_READY:			cout << "Test unit ready";			break;
		case SCSI_CMD_READ_6:					cout << "Read (6)";				break;
		case SCSI_CMD_READ_10:				cout << "Read (10)";				break;
		case SCSI_CMD_READ_CAPACITY:			cout << "Read capacity";			break;
		case SCSI_CMD_REQUEST_SENSE:			cout << "Request sense";			break;
		case SCSI_CMD_PREVENT_ALLOW_REMOVAL:	cout << "Prevent/allow removal";	break;
	}
	cout << "\n  Cmd len: " << cmd->cmdLen << "\n";
	cout << "  dataPtr: " << (UInt32)cmd->dataPtr << "\n";
	cout << "  dataLen: " << cmd->dataLen << "\n";
	cout << "  targetID: " << cmd->targetID << "\n";
	cout << "  transactionFlags: " << (const)cmd->transactionFlags << "\n";
	cout << "  error: " << (const)cmd->error << "\n";
}