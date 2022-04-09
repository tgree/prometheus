/*
	SCSI Driver.h
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
	Terry Greeniaus	-	Sunday, 13 June 98	-	Added MutexLock for preemptive multitasking...
	Terry Greeniaus	-	Tuesday, 23 June 98	-	Removed MutexLock - IOCommandDriver does this for us
*/
#ifndef __SCSI_DRIVER__
#define __SCSI_DRIVER__

#include "Driver.h"

enum
{
	// SCSI Bus phases.  Phases with a * can be determined by reading whatever register
	// has the MSG,C/D,I/O lines.  The 0b values indicate the MSG,C/D,I/O bits that determine
	// that phase, so not only do this indicate (in C++) what the phase is, but they also map to
	// the SCSI information transfer phases.
	dataOutPhase		=	0,	// 0b000 *
	dataInPhase		=	1,	// 0b001 *
	commandPhase		=	2,	// 0b010 *
	statusPhase		=	3,	// 0b011 *
	selectionPhase		=	4,
	reselectionPhase	=	5,
	msgOutPhase		=	6,	// 0b110 *
	msgInPhase		=	7,	// 0b111 *
	arbitrationPhase	=	8,
	requestStatusPhase	=	9,
	busFreePhase		=	10
};

extern ConstASCII8Str scsiPhaseName[];

enum
{
	// Errors
	noErr		=	0,
	scsiUnknownErr,
	scsiResetErr,
	scsiDisconnectErr,
	scsiParityErr,
	scsiSelectionTimeoutErr,
	scsiLostArbErr,			// This should never happen, since we have id 7
	scsiReselectionErr,
	scsiDontSelectMeErr,
	scsiBusDownErr			// This happens when there are no devices on the bus, and an arbitration will
							// never complete because of floating (unterminated) signals.  I get this on my
							// PowerBooko 3400 with Mesh when no SCSI devices are attached.
};

class SCSIDriver	:	public IOCommandDriver
{
	UInt32				currPhase;
	class SCSICommand*	currCommand;
	
	UInt32				preCountBuffer[15];
	Int8*				preCountBufferPtr;
	Int8*				preCountBufferPtrPhys;
	
	Boolean				requestedStatus;
	Boolean				requestedMessage;
	
	// Stuff for IOCommandDriver
	virtual	void	startAsyncIO(IOCommand* cmd);
	
			void	transferDMAIn();
			void	transferDMAInCompleted();
			void	transferDMAOut();
			void	transferDMAOutCompleted();
			
			void	handleStatusPhase();
			void	handleMessageInPhase();
			void	dumpCommand(SCSICommand* cmd);
protected:
	SCSIDriver(ConstASCII8Str name);
	virtual ~SCSIDriver();
			
			void	scsiError(UInt32 err);				// call this when an error occurs
			void	scsiReady();							// call this when the driver is ready to proceed after being told to do something
			void	scsiPhaseChanged(UInt32 newPhase);	// call this whenever the phase changes
			
			UInt32	scsiID();	// This returns the SCSI ID that you should use for your chip (CPU ID)
			
	// Your SCSI driver must support the following virtual functions.
	virtual	void	reset() = 0;
	virtual	void	arbitrate() = 0;
	virtual	void	selection(UInt32 targetID) = 0;
	
	virtual	void	messageOut(const Int8* data,UInt32 len,Boolean atn) = 0;	// data is logical addr
	virtual	void	requestMessage() = 0;						// tell the target to send us a message
	virtual	Int8	messageIn() = 0;							// return a single message byte, called consecutively for >1 byte messages
	virtual	void	acceptMessage() = 0;						// accept the message
	
	virtual	void	commandOut(const Int8* data,UInt32 len) = 0;	// data is logical addr
	
	virtual	void	dataOut(const Int8* data,UInt32 len) = 0;	// data is logical addr
	virtual	void	dmaOut(const Int8* data,UInt32 len) = 0;	// data is physical addr
	virtual	void	dataIn(Int8* data,UInt32 len) = 0;			// data is logical addr
	virtual	void	dmaIn(Int8* data,UInt32 len) = 0;			// data is physical addr
	virtual	UInt32	getDataTransferLen() = 0;				// returns the number of bytes actually transferred.  This should also stop DMA transfers when called (called once for each transfer)
	virtual	UInt32	readResidualDMAIn(Int8* data) = 0;		// reads residual (usually leftover FIFO data) data and returns the length
	virtual	void		requestStatus() = 0;						// tell the target to give status information
	virtual	UInt8	status() = 0;						// returns the status byte
	
	virtual	void	freeBus() = 0;								// free the bus
	
public:
			void	command(SCSICommand* cmd);
};

#endif
