/*
	CUDADriver.cp
	Copyright © 1998 by Patrick Varilly

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

			// via_cuda.c copyright notice
/*
 * Device driver for the via-cuda on Apple Powermacs.
 *
 * The VIA (versatile interface adapter) interfaces to the CUDA,
 * a 6805 microprocessor core which controls the ADB (Apple Desktop
 * Bus) which connects to the keyboard and mouse.  The CUDA also
 * controls system power and the RTC (real time clock) chip.
 *
 * Copyright (C) 1996 Paul Mackerras.
 */
/*
	Other sources			Project	Author			Notes
	===========			======	=====			====
	via_cuda.c			LinuxPPC
	Paul Mackerras		Adaptation of these sources, but much more
						readable and understandable(?)

	Version History
	============
	Terry Greeniaus	-	Monday, 15 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Monday, 15 June 98		-	Made it so that CUDAPacket was a KernelObject
	Patrick Varilly		-	Thursday, 18 June 98	-	Fixed timing bug in expectedAttention() (it appeared in Prometheus
												0.8a12. The strange thing is the *exact same* sources worked
												in 0.8a9. Someone might look into the deeper causes of this?)
	Patrick Varilly		-	Saturday, 20 June 98	-	Made ADB fit into the new driver init/start model.
												Also, we now map the CUDA chip here, instead of relying on
												PDM/PCI External Interrupt.cp to do it for us.
	Terry Greeniaus	-	Sunday, 21 June 98		-	Added replyPacket = nil line to CUDADriver() since new doesn't
												do this for us anymore.
	Patrick Varilly		-	Monday, 29 June 98		-	Complete rewrite based on LinuxPPC's via_cuda.c. It's now much
												more stable, and should stop those wierd timing bugs!
	Terry Greeniaus	-	Wed, 1 July 98			-	Canada day! Cool...  With some slight mods, it now works fine on my
												machine.  initialize() has been written correctly - initialize() in any
												driver should be the minimum code to disable interrupts.  start() now
												does the scary stuff.  Also, since CUDA interrupts were never being disabled before
												start() is called, isInterruptEnabled() would always return true, meaning that the
												driver never calls enable()!!!  This may work on a PDM machine, since CUDA interrupts
												are generated differently (no ICR to mask out all interrutps), but on my 7500 the CUDA
												interrupts never got through.  This MAY have been the bug in the old one too, but who knows.
												Also deleted theCommand->driver = this in sendCommand() - don't know where you got that
												from...
	Patrick Varilly		-	Monday, 29 March 99	-	Added ready flag so start() could be called repeatedly
*/
#include "CUDADriver.h"
#include "ADBInternals.h"
#include "NKVirtualMemory.h"
#include "Streams.h"
#include "Memory Utils.h"
#include "Chip Debugger.h"

#define LEVEL_NO_TRACE 				0
#define LEVEL_WAIT_TRACE				1
#define LEVEL_INT_TRACE				2
#define LEVEL_NO_INT_TRACE				4

#define TRACE_LEVEL					(LEVEL_NO_TRACE)

#if ((TRACE_LEVEL & LEVEL_WAIT_TRACE) != 0)
	#define TRACE_WAIT(x)	do { nkVideo << x; Wait_s(2); } while(0)
#else
	#define TRACE_WAIT(x)	do {} while(0)
#endif

#if ((TRACE_LEVEL & LEVEL_INT_TRACE) != 0)
	#define TRACE(x)		do { cout << x; } while(0)
#else
	#define TRACE(x)		do {} while(0)
#endif

#if ((TRACE_LEVEL & LEVEL_NO_INT_TRACE) != 0)
	#define TRACE_NOINT(x)	do { cout << x; } while(0)
#else
	#define TRACE_NOINT(x)	do {} while(0)
#endif

enum
{
	kOneSecondInNS = 1 * 1000 * 1000 * 1000
};

// Possible CUDA states
enum
{
	kCudaStateIdle,
	kCudaStateSentFirstByte,
	kCudaStateSending,
	kCudaStateAwaitingReply,
	kCudaStateReading,
	kCudaStateReadDone
};

// Possible values for intStatus
enum
{
	kCudaCollisionStatus = BIT_MASK(kCudaTransferRequestBit) | BIT_MASK(kCudaTransferInProgressBit)
					| BIT_MASK(kCudaDirectionBit),
	kCudaSendStatus = BIT_MASK(kCudaTransferInProgressBit) | BIT_MASK(kCudaDirectionBit),
	kCudaStartReadStatus = BIT_MASK(kCudaTransferRequestBit),
	kCudaReadStatus = BIT_MASK(kCudaTransferRequestBit) | BIT_MASK(kCudaTransferInProgressBit),
	kCudaFinishReadStatus = BIT_MASK(kCudaTransferInProgressBit)
};

// CUDA packet types
enum
{
	kCudaADBPacket = 0,
	kCudaPseudoPacket = 1
};

// CUDA bits in data[0] which tell about the errors of the command
enum
{
	kCudaSRQAssertMask	= 0x01,			// inactive device asserted SRQ
	kCudaTimeOutMask		= 0x02,			// active device did not have data available
	kCudaSRQErrorMask		= 0x04,			// device asserted excessive SRQ period
	kCudaBusErrorMask		= 0x08,			// timing error in bit cell was detected
	kCudaAutoPollMask		= 0x40,			// data is from an AutoPoll
	kCudaResponseMask		= 0x80			// response Packet in progress
};

static RegisterDescriptor	viaCUDARegisterDescriptor[]	=	{	CHIP_REGISTER(VIA_Chip,dataB,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,handshakeDataA,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataDirectionB,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataDirectionA,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1CounterLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1CounterHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1LatchLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1LatchHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer2CounterLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer2CounterHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,shift,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,auxillaryControl,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,peripheralControl,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,interruptFlag,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,interruptEnable,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataA,REG_NOFLAGS),
													LAST_REGISTER
												};

CUDADriver::CUDADriver( ADBHardware* adb,MachineDevice<VIA_Chip>* device )
	: ADBHardwareDriver( "CUDA ADB Driver", adb ),
	InterruptHandler( device->interrupts[0] )
{
	theChip = device->logicalAddr = (VIA_Chip*)NKIOMap(device->physicalAddr, device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE );
	
	// For the Chip Debugger
	new Chip("VIA CUDA",viaCUDARegisterDescriptor,theChip);
	
	// Initialize internal variables
	devPacket = new CUDAPacket;
	isDevMsg = false;
	state = kCudaStateIdle;
	currCommand = nil;
	replyPacket = nil;
	intStatus = 0;
}

Int8
CUDADriver::synchronizeWithCUDA()
{
	// Synchronize and wait for (a) CUDA to request a transfer and (b) the corresponding interrupt
	synchronize();
	if( !waitForTransferRequest(kOneSecondInNS) )	return 1;
	if( !waitForInterrupt(kOneSecondInNS) ) return 2;
	clearInterrupt();

	// Now desynchronize and wait for (a) CUDA to stop requesting a transfer and (b) the corresponding interrupt
	desynchronize();
	if( !waitForNoTransferRequest(kOneSecondInNS) )	return 3;
	if( !waitForInterrupt(kOneSecondInNS) ) return 4;
	clearInterrupt();

	return 0;
}

void
CUDADriver::initialize()
{
	// Initialize must do the MINIMUM in order to disable this device from generating interrupts
	VIA_CLR_BIT(theChip->interruptEnable,kCudaInterruptBit);
	_eieio();
	
	// Mark driver as not ready (this is used so start can be called safely after the driver is up)
	ready = false;
}

void
CUDADriver::start()
{
	// Check we're not ready
	if( !ready )
	{
		// Set direction of special bits. TIP and TACK are out, TREQ is in
		SET_SET_CLR( theChip->dataDirectionB, kCudaTransferInProgressBit,kCudaTransferAcknowledgeBit,kCudaTransferRequestBit );
		_eieio();

		// Terminate any transaction
		terminateTransaction();

		// Set clock control. We shift in (or out) according to the external clock. Also, change data direction to input
		SET_TWO_BITS( theChip->auxillaryControl, kCudaClockBit1,kCudaClockBit2 );
		_eieio();
		setDataDirectionToInput();

		// Clear any pending interrupts, wait 4ms and clear them again
		clearInterrupt();
		Wait_ms(4);
		clearInterrupt();

		// Synchronize with CUDA
		if( Int8 i = synchronizeWithCUDA() )
		{
			cout << "COULDN'T SYNCHRONIZE WITH CUDA!!! " << i << "\n";
			switch(i)
			{
				case 1:
				case 3:
					cout << "VIA1 Data B: " << ReadUReg8(&theChip->dataB);
					break;
				case 2:
				case 4:
					cout << "VIA1 IFR: " << ReadUReg8(&theChip->interruptFlag);
					break;
			}
			for(;;)
				;
		}

		// Stop any transfers (is this necessary?) and clear any interrupts left
		transferNotInProgress();
		clearInterrupt();
	}
	
	if( !isInterruptEnabled() )	// This will likely return false, unless you disable them yourself (which we now do in initialize())
		enable();
	
	ready = true;
}

void
CUDADriver::stop()
{
	ready = false;
	
	if( isInterruptEnabled() )
		disable();
}

void
CUDADriver::checkInterrupt()
{
	Boolean			saveIntEnable = isInterruptEnabled();
	if( saveIntEnable )
		disable();

	if( isPendingInterrupt() )
		handleInterrupt();

	if( saveIntEnable )
		enable();
}

void
CUDADriver::toggleInterruptMode( Boolean noInterruptMode )
{
	if( noInterruptMode )
		disable();
	else
		enable();
}

// Send the ADB command. Return true if successful
Boolean
CUDADriver::sendCommand( ADBIOCommand* theCommand )
{
	// Make the command into a packet
	CUDAPacket			*thePacket = new(kernelProcess) CUDAPacket;

	if( thePacket == nil )
		Panic( "Couldn't create CUDA Packet!\n" );

	TRACE_NOINT( "Packetizing command\n" );

	thePacket->dataDone = thePacket->dataSize = 0;

	// Set packet type and command
	if( theCommand->isADBCommand )
	{
		thePacket->data[thePacket->dataSize++] = kCudaADBPacket;
		thePacket->data[thePacket->dataSize++] = theCommand->command;
	}
	else
	{
		thePacket->data[thePacket->dataSize++] = kCudaPseudoPacket;
		switch( theCommand->command )
		{
			case kADBPseudoCmdStartStopAutoPoll:	thePacket->data[thePacket->dataSize++] = 0x01;	break;
			case kADBPseudoCmdSetDeviceList:		thePacket->data[thePacket->dataSize++] = 0x19;	break;
			case kADBPseudoCmdSetAutoPollRate:	thePacket->data[thePacket->dataSize++] = 0x14;	break;
			case kADBPseudoCmdPowerDown:		thePacket->data[thePacket->dataSize++] = 0x0A;	break;
			case kADBPseudoCmdRestart:			thePacket->data[thePacket->dataSize++] = 0x11;	break;
			default:
				Panic( "Unrecognized CUDA pseudo command\n" );
				break;
		}
	}

	// Fill rest of data in
	if( theCommand->type == kArgCommand )
	{
		for( Int8 i = 0; i < theCommand->data.info.numArgs; i++ )
			thePacket->data[thePacket->dataSize++] = theCommand->data.info.arg[i];
	}
	else
	{
		Int32			maxSize = kPacketMaxSize - thePacket->dataSize;
		if( theCommand->data.buffer.len > maxSize )
			Panic( "Buffer too big for it to fit in CUDA packet!\n" );

		MemCopy( theCommand->data.buffer.buffer, &thePacket->data[thePacket->dataSize], theCommand->data.buffer.len );
		thePacket->dataSize += theCommand->data.buffer.len;
	}

	// Put packet into driver 1 (we don't use driver2)
	theCommand->driver1 = reinterpret_cast<UInt32>(thePacket);
	
	TRACE_NOINT( "Sending command\n" );

	// ..and go!!!
	enqueue(theCommand);

	return true;
}

// Start sending command cmd to CUDA
void
CUDADriver::startAsyncIO(IOCommand* cmd)
{
	if( cmd != nil )
	{
		currCommand = static_cast<ADBIOCommand*>(cmd);
		internalSend();
	}
}

void
CUDADriver::internalSend()
{
	if( currCommand == nil )
		return;

	NKCriticalSection		critical;

	TRACE_NOINT( "Entered critical section\n" );

	if( isTransferBeingRequested() )	// Is CUDA sending us a byte (maybe from a device message)?
		return;					// currCommand will be processed later

	TRACE_NOINT( "CUDA is ready for another byte\n" );

	CUDAPacket			*thePacket = reinterpret_cast<CUDAPacket*>(currCommand->driver1);

	// Change data direction
	setDataDirectionToOutput();
	sendByte( thePacket->data[thePacket->dataDone++] );
	transferInProgress();

	state = kCudaStateSentFirstByte;

	TRACE_NOINT( "Sent first byte\n" );
}

void
CUDADriver::handleInterrupt()
{
	getInterruptStatus(intStatus);

	switch(state)
	{
		case kCudaStateSentFirstByte:	TRACE( "Sending second byte\n" );					sendSecondByte();		break;
		case kCudaStateSending:		TRACE( "Sending next byte\n" );					sendNextByte();		break;
		case kCudaStateAwaitingReply:	TRACE( "Getting ready for reply\n" );				startReply();			break;
		case kCudaStateReading:		TRACE( "Reading next byte\n" );					readNextByte();		break;
		case kCudaStateReadDone:		TRACE( "Reading last byte & processing response\n" );	processResponse();		break;
		case kCudaStateIdle:			TRACE( "Starting to process device message\n" );		startDeviceMessage();	break;
	}
}

void
CUDADriver::sendSecondByte()
{
	// Here is the point where a collision can happen: both us and a device have started to send data at the same time
	if( intStatus == kCudaCollisionStatus )
	{
		// Collision. Change directions and terminate the current transaction. currCommand will be sent as soon
		// as this device message is handled.
		setDataDirectionToInput();
		clearInterrupt();
		terminateTransaction();
		state = kCudaStateIdle;
	}
	else
	{
		CUDAPacket			*thePacket = reinterpret_cast<CUDAPacket*>(currCommand->driver1);
		Assert( intStatus == kCudaSendStatus );
		sendByte( thePacket->data[thePacket->dataDone++] );
		acknowledgeTransfer();
		state = kCudaStateSending;
	}
}

void
CUDADriver::sendNextByte()
{
	CUDAPacket			*thePacket = reinterpret_cast<CUDAPacket*>(currCommand->driver1);

	// Check to see if we're done
	if( thePacket->dataDone >= thePacket->dataSize )
	{
		// Finished, let's get ready for the reply
		setDataDirectionToInput();
		clearInterrupt();
		terminateTransaction();

		// Clear the packet
		thePacket->dataDone = thePacket->dataSize = 0;

		// Switch state
		state = kCudaStateAwaitingReply;
	}
	else
	{
		sendByte( thePacket->data[thePacket->dataDone++] );
		acknowledgeTransfer();
	}
}

void
CUDADriver::startReply()
{
	CUDAPacket			*thePacket = reinterpret_cast<CUDAPacket*>(currCommand->driver1);

	// Get ready for a command reply
	Assert( intStatus == kCudaStartReadStatus );

	clearInterrupt();
	transferInProgress();
	state = kCudaStateReading;

	replyPacket = thePacket;
	isDevMsg = false;
}

void
CUDADriver::readNextByte()
{
	readByte( replyPacket->data[replyPacket->dataDone++] );
	if( intStatus == kCudaFinishReadStatus )
	{
		// We're done!
		terminateTransaction();
		state = kCudaStateReadDone;
	}
	else
	{
		Assert( intStatus == kCudaReadStatus );
		acknowledgeTransfer();
	}
}

void
CUDADriver::processResponse()
{
	UInt8*	data;
	UInt8	headerSize;

	clearInterrupt();

	// Make data point to the actual packet data, not the header
	TRACE_WAIT( "Processing response. replyPacket is " << (UInt32)replyPacket << "\n" );
	if( replyPacket->data[0] == 5 )
		headerSize = 4;
	else
		headerSize = 3;
	data = &replyPacket->data[headerSize];

	replyPacket->dataSize = replyPacket->dataDone - headerSize;
	TRACE_WAIT( "Header is " << headerSize << " bytes big\n"
				"Data is at " << (UInt32)data << "\n"
				"Data is therefore " << replyPacket->dataSize << " bytes big\n" );

	if( isDevMsg )
	{
		TRACE_WAIT( "Device packet for " << (replyPacket->data[2] >> 4) << " at " << (UInt32)data << "\n" );
		adb->DeviceMessage( replyPacket->data[2] >> 4,replyPacket->dataSize, data );
	}
	else
	{
		// Copy reply
		currCommand->replyLen = replyPacket->dataSize;
		TRACE_WAIT( "Copying " << currCommand->replyLen << "-byte reply\n" );
		if( currCommand->replyLen != 0 )
		{
			currCommand->reply = new(kernelProcess) UInt8[currCommand->replyLen];
			MemCopy( data, currCommand->reply,currCommand->replyLen );
			TRACE_WAIT( "Reply is at " << (UInt32)currCommand->reply << "\n" );
		}

		// Get result
		UInt8			rawResult = replyPacket->data[1];
		currCommand->result = kADBResultOK;

		if( rawResult & kCudaTimeOutMask )
			currCommand->result = kADBResultTimeout;
		else if( rawResult & kCudaSRQAssertMask )
			currCommand->result = kADBResultUnknown;
		else if( rawResult & kCudaSRQErrorMask )
			currCommand->result = kADBResultPacketRequestError;
		else if( rawResult & kCudaBusErrorMask )
			currCommand->result = kADBResultBusError;

		TRACE_WAIT( "Command result is " << currCommand->result << "\n" );
		
		// Mark command as done
		currCommand->doneIO();
		
		// Destroy temporary packet (replyPacket by now is equal to currCommand->driver1)
		delete replyPacket;

		IOCommand			*nextCommand = dequeue();

		if( nextCommand != nil )
			currCommand = static_cast<ADBIOCommand*>(nextCommand);
		else
			currCommand = nil;

		TRACE_WAIT( "Next command is " << (UInt32)currCommand << "\n" );
	}

	// Check for a device message which needs to be handled
	if( intStatus == kCudaStartReadStatus )
	{
		TRACE_WAIT( "New device message coming in...\n" );
		// Set up for reading device message
		transferInProgress();
		state = kCudaStateReading;
		replyPacket = devPacket;
		isDevMsg = true;

		// Clear reply packet
		replyPacket->dataDone = replyPacket->dataSize = 0;
	}
	else
	{
		state = kCudaStateIdle;
		if( currCommand )
		{
			TRACE_WAIT( "Now sending command " << (UInt32)currCommand << "\n" );
			internalSend();		// currCommand has already been set before to the next command to process
		}
	}

	TRACE_WAIT( "Finished processing reply\n" );
}

void
CUDADriver::startDeviceMessage()
{
	Assert( intStatus == kCudaStartReadStatus );
	clearInterrupt();

	// Get ready for the device message
	transferInProgress();
	state = kCudaStateReading;
	replyPacket = devPacket;
	isDevMsg = true;

	// Clear reply packet
	replyPacket->dataDone = replyPacket->dataSize = 0;
}
