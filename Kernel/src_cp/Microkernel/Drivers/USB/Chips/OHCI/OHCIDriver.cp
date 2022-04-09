/*
	OHCIDriver.cp
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
	uusbd/ohci/driver.c		Linux PowerPC			I–aky PŽrez Gonz‡lez	Init code adapted from here, init specs in OHCI
															don't quite work...  Also some valuable tricks from
															other parts of the USB driver.
	
	Version History
	============
	Patrick Varilly		-	Fri, 26 Nov 99		-	Original creation of file
*/

// All things pending are marked FIXME

#include "OHCIDriver.h"
#include "NKDebuggerNub.h"
#include "Macros.h"
#include "Streams.h"
#include "Time.h"
#include "NKVirtualMemory.h"
#include "OHCIRootHub.h"

//static void printEndpoint( OHCIEndpoint *ed );

#pragma mark Public Interfaces
#pragma mark =============

static USBErr				ccConversion[16] =		// Converts CC to USB HC error code
	{ kUSBErr_None, kUSBErr_CRC, kUSBErr_BitStuffing, kUSBErr_DataToggleMismatch, kUSBErr_Stall,
	kUSBErr_DeviceNotResponding,	kUSBErr_PIDCheckFailure, kUSBErr_UnexpectedPID, kUSBErr_DataOverrun,
	kUSBErr_DataUnderrun, kUSBErr_Internal, kUSBErr_Internal, kUSBErr_BufferOverrun, kUSBErr_BufferUnderrun,
	kUSBErr_Internal, kUSBErr_Internal };

OHCIDriver::OHCIDriver()
	: USBHCDriver( "OHCI USB Controller" )
{
	// Set up private variables
	pPending = pPendingTail = nil;
	rootHub = nil;
	frameNumberHigh = 0;
	
	// Allocate dummy endpoint descriptors for all queues, to serve as heads
	UInt32						i;
	for( i = 0; i < kListMax; i++ )
	{
		OHCIPipe					*p;
		p = createPipe();
		FatalAssert( p != nil );
		p->setConfig( kEDSkip | (15 << kEDNumberShift) | 1 );	// Address 1 is our root hub, no endpoint will have this
		p->setNextEDPtr( nil );
		queues[i] = p;
	}
	
	// Link up interrupt queues
	for( i = 0; i < 16; i++ )
	{
		queues[kListInt32ms+i]->next = queues[kListInt16ms+i];
		queues[kListInt32ms+i]->setNextEDPtr( queues[kListInt16ms+i]->physAddr );
		queues[kListInt32ms+16+i]->next = queues[kListInt16ms+i];
		queues[kListInt32ms+16+i]->setNextEDPtr( queues[kListInt16ms+i]->physAddr );
	}
	for( i = 0; i < 8; i++ )
	{
		queues[kListInt16ms+i]->next = queues[kListInt8ms+i];
		queues[kListInt16ms+i]->setNextEDPtr( queues[kListInt8ms+i]->physAddr );
		queues[kListInt16ms+8+i]->next = queues[kListInt8ms+i];
		queues[kListInt16ms+8+i]->setNextEDPtr( queues[kListInt8ms+i]->physAddr );
	}
	for( i = 0; i < 4; i++ )
	{
		queues[kListInt8ms+i]->next = queues[kListInt4ms+i];
		queues[kListInt8ms+i]->setNextEDPtr( queues[kListInt4ms+i]->physAddr );
		queues[kListInt8ms+4+i]->next = queues[kListInt4ms+i];
		queues[kListInt8ms+4+i]->setNextEDPtr( queues[kListInt4ms+i]->physAddr );
	}
	for( i = 0; i < 2; i++ )
	{
		queues[kListInt4ms+i]->next = queues[kListInt2ms+i];
		queues[kListInt4ms+i]->setNextEDPtr( queues[kListInt2ms+i]->physAddr );
		queues[kListInt4ms+2+i]->next = queues[kListInt2ms+i];
		queues[kListInt4ms+2+i]->setNextEDPtr( queues[kListInt2ms+i]->physAddr );
	}
	queues[kListInt2ms]->next = queues[kListInt1ms];
	queues[kListInt2ms]->setNextEDPtr( queues[kListInt1ms]->physAddr );
	queues[kListInt2ms+1]->next = queues[kListInt1ms];
	queues[kListInt2ms+1]->setNextEDPtr( queues[kListInt1ms]->physAddr );
	
	queues[kListInt1ms]->next = queues[kListIsochronous];
	queues[kListInt1ms]->setNextEDPtr( queues[kListIsochronous]->physAddr );
	
	queues[kListIsochronous]->next = nil;
	queues[kListIsochronous]->setNextEDPtr( nil );
}

OHCIDriver::~OHCIDriver()
{
}

void
OHCIDriver::initialize()
{
	// Allocate HCCA on 256-byte boundary
	Ptr					hccaPtr;
	hccaPtr = new Int8[sizeof(OHCIHCCA)+256];
	Assert( hccaPtr != nil );
	hcca = (OHCIHCCA*)ROUND_UP(256, (UInt32)hccaPtr);
	
	// Init HCCA
	UInt32						i;
	for( i = 0; i < 32; i++ )
		setInterruptED(i,queues[kListInt32ms+i]->physAddr);
	hcca->frameNumber = 0;
	hcca->pad1 = 0;
	clearDoneHead();
	for( i = 0; i < 30; i++ )
		hcca->reserved[i] = 0;
	
	// * (OHCI 5.1.1.2) Check we're a compatible chip
	UInt32				revision;
	revision = getRevision() & kRevisionMask;
	if( revision != 0x10 )
	{
		cout << "Incompatible OHCI revision" << (revision>>4) << "." << (revision&0xF) << newLine;
		Panic( "Incompatible OHCI chip!!!" );
	}
	
	// * (OHCI 5.1.1.3) It's reasonable to assume an iMac won't have a BIOS or SMM =)
	// FIXME: Add code to take over chip
	
	// * (OHCI 5.1.1.4) Set up Host Controller
	// We need this to execute in less than 2ms, so enter a critical section to be on the safe side
	{
	NKCriticalSection		critical;
	
	// Issue software reset
	setCommandStatus( kHCReset );
	Wait_us( 10 );
	
	// Set control to have 2:1 Control:Bulk, no lists enabled, no interrupt routing and no remote wakeup or connected, reset
	setControl( 0x1 /* 2:1 */ | kFSReset );
	
	// Disable interrupts until everything is set up
	disableInt( kIntMaster | kIntAll );
	
	// Set HCCA and other queues
	setHCCA( NKGetPhysical(hcca,PROCESS_KERNEL) );
	setControlHeadED( queues[kListControl]->physAddr );
	setControlCurrentED( nil );
	setBulkHeadED( queues[kListBulk]->physAddr );
	setBulkCurrentED( nil );
	clearHcDoneHead();
	
	// Move to operational state, to be able to write to other registers
	setControl( (getControl() & ~kFSMask) | kFSOperational );
	
	// Disable legacy support if necessary
	if( getRevision() & kLegacySupportSupport )
		clearHceControl();
	
	// Set largest data packet size and frame bit size. Actual value is 10104.5, round down to nearest multiple of 8
	UInt32				fmInterval;
	fmInterval = 11999/*getFmInterval()*/;
	setFmInterval( (fmInterval & ~kFullSpeedMPSMask) | (10100 << kFullSpeedMPSShift) );
	
	// Set up PeriodicStart to 90% of FrameInterval
	setPeriodicStart( ((fmInterval & kFrameIntervalMask)*9)/10 );
	
	// Enable all lists
	setControl( getControl() | kControlListEnable | kBulkListEnable | kPeriodicListEnable | kIsochronousListEnable );
	
	// Enable interrupts (and clear anything left pending by MacOS)
	enableInt( 0 | kIntSchedulingOverrun | kIntWritebackDoneHead /*| kIntStartOfFrame*/ /*| kIntResumeDetected*/
		| kIntUnrecoverableError | kIntFrameNumberOverflow /*| kIntRootHubStatusChange*/ | kIntOwnershipChange
		| kIntMaster );
	clearInt( 0xFFFFFFFF );
	}		// End of critical section
}

// Start and stop should really drive the bus into suspend and resume
void
OHCIDriver::start()
{
}

void
OHCIDriver::stop()
{
}

USBHub*
OHCIDriver::getRootHub( USBBus* bus )
{
	if( rootHub )
		return rootHub;
	else
	{
		rootHub = OHCIRootHub::createRootHub( bus, this );
		return rootHub;
	}
}

UInt64
OHCIDriver::getFrameNumber()
{
	// This is directly from OHCI 5.4 (adapted for 64 bits)
	UInt64					high = frameNumberHigh;
	UInt16					low = getFrameNumberFromHC();
	return (high | (low & 0x7FFF)) + ((low ^ high) & 0x8000);
}

USBErr
OHCIDriver::createControlPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
	const Boolean fullSpeed, const UInt16 maxPacketSize )
{
	CriticalSection				critical(ohciLock);
	return doCreatePipe( outPipe, address, endpoint, fullSpeed, maxPacketSize, kListControl );
}

//USBErr
//OHCIDriver::createBulkPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
//	const Boolean fullSpeed, const UInt16 maxPacketSize )
//{
//}

USBErr
OHCIDriver::createInterruptPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
	const Boolean fullSpeed, const UInt16 maxPacketSize, const UInt32 interval )
{
	// For now use only 16ms, and do no bandwidth checking
#pragma unused(interval)
	CriticalSection				critical(ohciLock);
	return doCreatePipe( outPipe, address, endpoint, fullSpeed, maxPacketSize, kListInt16ms );
}

//USBErr
//OHCIDriver::createIsochronousPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
//	const Boolean fullSpeed, const UInt16 maxPacketSize )
//{
//}

USBErr
OHCIDriver::abortPipe( USBHCPipe* pipe )
{
	CriticalSection				critical(ohciLock);
	return doAbortPipe( (OHCIPipe*)pipe );
}

USBErr
OHCIDriver::resetPipe( USBHCPipe* pipe )
{
	CriticalSection				critical(ohciLock);
	OHCIPipe					*p = (OHCIPipe*)pipe;
	
	USBErr					err;
	err = doAbortPipe( p );
	if( err )
		return err;
	return doResumePipe( p );
}

USBErr
OHCIDriver::pausePipe( USBHCPipe* pipe )
{
	CriticalSection				critical(ohciLock);
	return doPausePipe( (OHCIPipe*)pipe );
}

USBErr
OHCIDriver::resumePipe( USBHCPipe* pipe )
{
	CriticalSection				critical(ohciLock);
	return doResumePipe( (OHCIPipe*)pipe );
}

USBErr
OHCIDriver::deletePipe( USBHCPipe* pipe )
{
	CriticalSection				critical(ohciLock);
	OHCIPipe					*p = (OHCIPipe*)pipe;
	
	// Remove from physical and virtual queues
	p->prev->setNextEDPtr( p->getNextEDPtr() );
	p->prev->next = p->next;
	if( p->next )
		p->next->prev = p->prev;
	
	// Mark for deletion
	addToPending( p );
	p->state = kPipeState_PendingDelete;
	disableList( p->list );		// In case we're the current pipe being processed by the HC
	enableSOF();
	
	// Abort all transactions on the pipe
	doAbortPipe( p );
	
	return kUSBErr_None;
}

USBErr
OHCIDriver::controlTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe, ConstUSBControlSetupPtr setup,
	ConstPtr buffer, Boolean shortResponseOK )
{
	CriticalSection				critical(ohciLock);
	
	// Check the pipe is a control pipe
	OHCIPipe					*p = (OHCIPipe*)pipe;
	if( p->list != kListControl )
		return kUSBErr_BadArgs;
	
	// Check that there's a buffer if wLength is not 0
	if( (setup->wLength != 0) && (buffer == nil) )
		return kUSBErr_BadArgs;
	
	// Make a new transaction
	OHCITransaction			*trans;
	trans = new OHCITransaction;
	if( !trans )
		return kUSBErr_OutOfResources;
	outTransaction = (USBTransaction*)trans;
	
	// Initialise fields
	trans->driver = this;
	trans->pipe = p;
	trans->shortResponseOK = shortResponseOK;
	
	// Create setup transfer
	OHCITransfer				*setupT;
	setupT = getFreeTransfer( p );
	if( !setupT )
	{
		delete trans;
		return kUSBErr_OutOfResources;
	}
	setupT->transaction = trans;
	
	// Set up HC fields
	UInt32					config;
	Ptr						firstByte, lastByte;
	void						*physAddr;
	config = (kCCNotAccessed << kTDConditionCodeShift) | kTDDataToggle0 | kTDDelayInterruptNone | kTDPIDSetup;
	setupT->setConfig( config );
	firstByte = (Ptr)setup;
	lastByte = firstByte+8-1;		// 8 bytes, so last byte is 7 bytes from the first one
	// NKLockMemory( setup, sizeof( USBControlSetup ) );
	physAddr = NKGetPhysical( firstByte, PROCESS_CURRENT );
	FatalAssert( physAddr != nil );
	setupT->setCurrentBufferPtr( physAddr );
	physAddr = NKGetPhysical( lastByte, PROCESS_CURRENT );
	FatalAssert( physAddr != nil );
	setupT->setBufferEndPtr( physAddr );
	
	// Identify direction and size
	Boolean					downstream;
	UInt16					bufferSize, maxPacketSize;
	downstream = ((setup->bmRequestType & kBmDirMask) == kBmDownstream);
	if( buffer )
		bufferSize = LE16( setup->wLength );
	else
		bufferSize = 0;
	maxPacketSize = p->getConfig() >> kEDMaxPacketSizeShift;
	
	// Create data transfers
	if( bufferSize > 0 )
	{
		// NKLockMemory( buffer, bufferSize );
		UInt16				toGo = bufferSize;
		Ptr					curBuf = (Ptr)buffer;
		while( toGo > 0 )
		{
			// Transfer in two page blocks
			UInt32			maxSize = (4096 - ((UInt32)curBuf & 0x3FF)) + 4096;
			UInt32			size = (toGo > maxSize) ? maxSize : toGo;
			
			// Make sure size is always a multiple of maxPacketSize
			if( size < toGo )
				size = (UInt16)(size/maxPacketSize) * maxPacketSize;
			
			toGo -= size;
			
			firstByte = curBuf;
			lastByte = firstByte+size-1;
			curBuf += size;
			
			OHCITransfer		*dataT;
			dataT = getFreeTransfer( p );
			FatalAssert( dataT != nil );			// For now.  FIXME: Real out of mem handling here!!!
			dataT->transaction = trans;
			
			// Set config
			config = (kCCNotAccessed << kTDConditionCodeShift) | kTDDataToggleFromED | kTDDelayInterruptNone;
			if( downstream )
				config |= kTDPIDOut;
			else
				config |= kTDPIDIn;
			if( shortResponseOK && (toGo == 0) )	// Last packet
				config |= kTDBufferRounding;
			dataT->setConfig( config );
			
			// Set up buffer
			physAddr = NKGetPhysical( firstByte, PROCESS_CURRENT );
			FatalAssert( physAddr != nil );
			dataT->setCurrentBufferPtr( physAddr );
			physAddr = NKGetPhysical( lastByte, PROCESS_CURRENT );
			FatalAssert( physAddr != nil );
			dataT->setBufferEndPtr( physAddr );
		}
	}
	
	// Create status transfer
	OHCITransfer				*statusT;
	statusT = getFreeTransfer( p );
	FatalAssert( statusT != nil );
	statusT->transaction = trans;
	
	// Fill in
	config = (kCCNotAccessed << kTDConditionCodeShift) | kTDDataToggle1 | (0 << kTDDelayInterruptShift);
	if( downstream )
		config |= kTDPIDIn;
	else
		config |= kTDPIDOut;
	statusT->setConfig( config );
	statusT->setCurrentBufferPtr( nil );
	statusT->setBufferEndPtr( nil );
	
	// Transfers are set up, let's let the HC do the rest
	trans->firstTransfer = setupT;
	trans->lastTransfer = statusT;
	linkTransfers( p );
	
	return kUSBErr_None;
}

USBErr
OHCIDriver::interruptTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe, ConstPtr buffer, const UInt32 bufferSize,
	Boolean shortResponseOK )
{
	CriticalSection				critical(ohciLock);
	
	// Check the pipe is an interrupt pipe
	OHCIPipe					*p = (OHCIPipe*)pipe;
	if( (p->list < kListInt32ms) || (p->list > kListInt1ms) )
		return kUSBErr_BadArgs;
	
	// Check that there's a buffer if bufferSize is not 0
	if( (bufferSize != 0) && (buffer == nil) )
		return kUSBErr_BadArgs;
	
	// Check buffer is not too big
	if( (bufferSize + ((UInt32)buffer & 0x3FF)) > 8096 )
		Panic( "Interrupt buffer is *way* too large!" );
	
	// Make a new transaction
	OHCITransaction			*trans;
	trans = new OHCITransaction;
	if( !trans )
		return kUSBErr_OutOfResources;
	outTransaction = (USBTransaction*)trans;
	
	// Initialise fields
	trans->driver = this;
	trans->pipe = p;
	trans->shortResponseOK = shortResponseOK;
	
	// Create transfer
	OHCITransfer				*t;
	t = getFreeTransfer( p );
	FatalAssert( t != nil );
	t->transaction = trans;
	
	// Figure out direction
	UInt32					config;
	Boolean					downstream;
	config = p->getConfig();
	if( (config & kEDDirectionMask) == kEDDirectionIn )
		downstream = false;
	else
		downstream = true;
	
	// Fill in config
	config = (kCCNotAccessed << kTDConditionCodeShift) | kTDDataToggleFromED | (0 << kTDDelayInterruptShift);
	if( downstream )
		config |= kTDPIDOut;
	else
		config |= kTDPIDIn;
	if( shortResponseOK )
		config |= kTDBufferRounding;
	t->setConfig( config );
	
	// Figure out buffer pointers
	Ptr						firstByte, lastByte;
	void						*physAddr;
	firstByte = (Ptr)buffer;
	lastByte = firstByte+bufferSize-1;
	// NKLockMemory( buffer, bufferSize );
	physAddr = NKGetPhysical( firstByte, PROCESS_CURRENT );
	FatalAssert( physAddr != nil );
	t->setCurrentBufferPtr( physAddr );
	physAddr = NKGetPhysical( lastByte, PROCESS_CURRENT );
	FatalAssert( physAddr != nil );
	t->setBufferEndPtr( physAddr );
	
	// Transfer is set up, let's let the HC do the rest
	trans->firstTransfer = trans->lastTransfer = t;
	linkTransfers( p );
	
	return kUSBErr_None;
}

void
OHCIDriver::abortTransaction( OHCITransaction *trans )
{
	CriticalSection				critical(ohciLock);
	doAbortTransaction( trans );
}

#pragma mark Ê
#pragma mark Internals
#pragma mark =======

void
OHCIDriver::ohciInterrupt()
{
	CriticalSection				critical(ohciLock);	// For MP machines
	
	// Get interrupt status
	UInt32				status, mask;
	status = getIntStatus();
	mask = getIntMask();
	status &= mask;
	
	// Handle each interrupt condition in turn
	if( status & kIntWritebackDoneHead )
		handleDoneTransfers();
	if( status & kIntStartOfFrame )
	{
		// Walk through the pending list doing what's necessary
		OHCIPipe						*p, *next;
		Boolean						controlFilled = false, bulkFilled = false;
		p = pPending;
		while( p )
		{
			next = p->pendingNext;
			p->pendingNext = p->pendingPrev = nil;
			
			// Retire any aborted transactions
			doRetireAborted( p );
			
			switch( p->state )
			{
				case kPipeState_PendingAbort:
					// Revive endpoint
					p->setConfig( p->getConfig() & ~kEDSkip );
					p->state = kPipeState_Running;
					
					// Mark queue as filled if it is, so we don't lose a frame if there's still work to do
					if( (p->list == kListControl) && (p->head != nil) )
						controlFilled = true;
					else if( (p->list == kListBulk) && (p->head != nil) )
						bulkFilled = true;
					break;
				case kPipeState_PendingPause:
					// Move to paused state
					p->state = kPipeState_Paused;
					break;
				case kPipeState_PendingDelete:
					// Make sure we're not curently being processed
					if( (p->list == kListControl) && (getControlCurrentED() == p->physAddr) )
						setControlCurrentED( nil );
					else if( (p->list == kListBulk) && (getBulkCurrentED() == p->physAddr) )
						setBulkCurrentED( nil );
					
					// Destroy placeholder transfer
					disposeTransfer( p->placeHolder );
					
					// Free pipe
					disposePipe( p );
					break;
			}
			
			p = next;
		}
		
		// Clear pending list
		pPending = pPendingTail = nil;
		
		// Enable all lists
		setControl( getControl() | kControlListEnable | kBulkListEnable | kIsochronousListEnable | kPeriodicListEnable );
		
		// Mark appropiate lists as filled
		if( controlFilled || bulkFilled )
		{
			UInt32			cmdStatus = 0;
			if( controlFilled )
				cmdStatus |= kControlListFilled;
			if( bulkFilled )
				cmdStatus |= kBulkListFilled;
			setCommandStatus( cmdStatus );
		}
		
		// Disable SOF
		disableInt( kIntStartOfFrame );
	}
	if( status & kIntRootHubStatusChange )
	{
		rootHubStatSem.up();
		disableInt( kIntRootHubStatusChange );
	}
	if( status & kIntSchedulingOverrun )
		nkVideo << redMsg << "Scheduling Overrun\n" << whiteMsg;
	if( status & kIntResumeDetected )
		nkVideo << redMsg << "Resume Detected\n" << whiteMsg;
	if( status & kIntUnrecoverableError )
		nkVideo << redMsg << "Unrecoverable Error\n" << whiteMsg;
	if( status & kIntFrameNumberOverflow )
		frameNumberHigh += 0x10000 - ((getFrameNumberFromHC() ^ frameNumberHigh) & 0x8000);
	if( status & kIntOwnershipChange )
		nkVideo << redMsg << "Ownership Change\n" << whiteMsg;
	
	// Clear interrupt status
	clearInt( status );
}

void
OHCIDriver::handleDoneTransfers()
{
	void*				doneHead;
	doneHead = getDoneHead();
	
	// Detect interrupt
	doneHead = (void*)(((UInt32)doneHead) & ~0x1);
	
	if( doneHead != nil )
	{
		// Move all transfers to our own done queue, in the order they were done
		OHCITransfer		*doneQ = nil, *t;
		
		while( doneHead )
		{
			t = searchTransfer( (Ptr)doneHead );
			FatalAssert( t != nil );
			
			// Insert it in our done queue
			t->doneNext = doneQ;
			doneQ = t;
			
			// Advance doneHead
			doneHead = (void*)t->getNextTDPtr();
		}
		
		// Go through each transfer, processing it in turn
		t = doneQ;
		while( t )
		{
			OHCITransfer		*next = t->doneNext;
			OHCITransaction	*trans = t->transaction;
			OHCIPipe			*p = trans->pipe;
			ConditionCode		cc;
			
			// Remove it from its proper queue
			if( t->next )
				t->next->prev = t->prev;
			else
				p->placeHolder->prev = t->prev;		// This should be nil by now, but anyway
			if( t->prev )							// As above, td->prev should be nil by now...
				t->prev->next = t->next;
			else
				p->head = t->next;
			
			// Get condition code
			cc = (ConditionCode)(t->getConfig() >> kTDConditionCodeShift);
			
			// Print something if cc is not ok
			if( cc != kCCNoError )
				nkVideo << greenMsg << "OHCI Error " << (UInt32)cc << "\n" << whiteMsg;
			
			// Check to not signal data underrun as error if requested by client
			if( (cc == kCCDataUnderrun) && trans->shortResponseOK )
			{
				if( p->list == kListControl )
				{
					// Proceed to status stage
					if( t != trans->lastTransfer )
					{
						p->head = trans->lastTransfer;
						trans->lastTransfer->prev = nil;
						p->setHeadPtr( trans->lastTransfer->physAddr );
					}
				}
				else
					cc = kCCNoError;
				p->clearHalted();
			}
			
			if( cc != kCCNoError )
			{
				doRetireTransaction( trans, ccConversion[cc] );
				
				// Clear halt bit and restart list if necessary
				p->clearHalted();
				if( p->head )
				{
					if( p->list == kListControl )
						setCommandStatus( kControlListFilled );
					else if( p->list == kListBulk )
						setCommandStatus( kBulkListFilled );
				}
			}
			else if( t == trans->lastTransfer )
			{
				// If this is the last transfer for this transaction, retire it
				doRetireTransaction( trans, kUSBErr_None, false );
			}
			
			t = next;
		}
	}
	else
		usbOut << redMsg << "Done head is nil, but we got an interrupt!\n" << whiteMsg;
}

USBErr
OHCIDriver::doCreatePipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
	const Boolean fullSpeed, const UInt16 maxPacketSize, const OHCIPipeList list )
{
	// Only control and interrupt pipes for now
	if( (list != kListControl) && ((list < kListInt32ms) || (list > kListInt1ms)) )
		return kUSBErr_Unsupported;
	
	// Allocate pipe
	OHCIPipe					*p;
	p = createPipe();
	if( p == nil )
		return kUSBErr_OutOfResources;
	outPipe = (USBHCPipe*)p;
	
	// Allocate placeholder
	p->placeHolder = createTransfer();
	if( p->placeHolder == nil )
	{
		disposePipe( p );
		return kUSBErr_OutOfResources;
	}
	
	// Fill in pipe variables
	p->list = list;
	
	// Fill ED in
	UInt32					config;
	config = (maxPacketSize << kEDMaxPacketSizeShift) | ((endpoint & kEndpointMask) << kEDNumberShift) | address;
	if( !fullSpeed )
		config |= kEDLowSpeed;
	if( list == kListControl )
		config |= kEDDirectionDepends;
	else if( endpoint & kEndpointIn )
		config |= kEDDirectionIn;
	else
		config |= kEDDirectionOut;
	if( list == kListIsochronous )
		config |= kEDIsoFormat;
	p->setConfig(config);
	p->setTailPtr( p->placeHolder->physAddr );
	p->setHeadPtr( p->placeHolder->physAddr );
	p->clearHalted();
	p->clearToggleCarry();
	
	// Link it (prepending)
	OHCIPipe					*q;
	q = queues[list];
	p->next = q->next;
	if( p->next )
		p->next->prev = p;
	p->prev = q;
	q->next = p;
	
	if( p->next )
		p->setNextEDPtr( p->next->physAddr );
	else
		p->setNextEDPtr( nil );
	q->setNextEDPtr( p->physAddr );
	
	// Done!
	p->state = kPipeState_Running;
	return kUSBErr_None;
}

USBErr
OHCIDriver::doAbortPipe( OHCIPipe *p )
{
	// Mark all transactions as abort pending
	OHCITransfer			*t;
	t = p->head;
	while( t )
	{
		t->transaction->abortPending = true;
		t = t->next;
	}
	
	// If we can, abort them all right now.  Otherwise, they'll be aborted on next SOF (if they haven't completed)
	if( p->state == kPipeState_Paused )
		doRetireAborted( p );
	else if( p->state == kPipeState_Running )
	{
		// Schedule abort for next SOF
		addToPending( p );
		p->state = kPipeState_PendingAbort;
		
		p->setConfig( p->getConfig() | kEDSkip );
		enableSOF();
	}
	
	return kUSBErr_None;
}

USBErr
OHCIDriver::doPausePipe( OHCIPipe *p )
{
	// Set skip
	p->setConfig( p->getConfig() | kEDSkip );
	
	// This is just a state change
	if( p->state == kPipeState_Running )
	{
		addToPending( p );
		enableSOF();
	}
	p->state = kPipeState_PendingPause;
	
	return kUSBErr_None;
}

USBErr
OHCIDriver::doResumePipe( OHCIPipe *p )
{
	// This is not as easy as above.  If we're resuming a PendingPause, we might have to change to PendingAbort instead
	// of Running, given that aborts may have been asked for after the pause was asked for
	if( p->state == kPipeState_PendingPause )
	{
		OHCITransfer			*t;
		t = p->head;
		while( t )
		{
			if( t->transaction->abortPending )
				break;
			t = t->next;
		}
		
		if( t != nil )
		{
			p->state = kPipeState_PendingAbort;
			return kUSBErr_None;
		}
	}
	
	// Clear skip and switch to running state
	p->setConfig( p->getConfig() & ~kEDSkip );
	p->state = kPipeState_Running;
	
	return kUSBErr_None;
}


OHCITransfer*
OHCIDriver::getFreeTransfer( OHCIPipe *p )
{
	FatalAssert( p->placeHolder != nil );
	
	OHCITransfer				*t = p->placeHolder;
	
	// Create new placeholder
	p->placeHolder = createTransfer();
	if( p->placeHolder == nil )
	{
		p->placeHolder = t;
		return nil;
	}
	
	// Link
	t->next = nil;
	p->placeHolder->prev = t;
	if( t->prev )
		t->prev->next = t;
	else
		p->head = t;
	t->setNextTDPtr( p->placeHolder->physAddr );
	
	return t;
}

void
OHCIDriver::linkTransfers( OHCIPipe* p )
{
	// Tell HC new transfers are ready to process
	p->setTailPtr( p->placeHolder->physAddr );
	
	// Tell HC list is filled
	if( p->list == kListControl )
		setCommandStatus( kControlListFilled );
	else if( p->list == kListBulk )
		setCommandStatus( kBulkListFilled );
}

void
OHCIDriver::doAbortTransaction( OHCITransaction *trans )
{
	OHCIPipe					*p = trans->pipe;
	
	// Mark for abortion if it hasn't completed yet
	if( !trans->hasDoneIO )
	{
		trans->abortPending = true;
		
		// Move pipe to pending abort transaction state
		addToPending( p );
		p->state = kPipeState_PendingAbort;
		
		// Schedule for abort on next SOF
		p->setConfig( p->getConfig() | kEDSkip );
		enableSOF();
	}
}

void
OHCIDriver::freeTDs( OHCITransaction *trans )
{
	OHCITransfer				*t, *next;
	t = trans->firstTransfer;
	while( t )
	{
		next = t->next;
		disposeTransfer( t );
		if( t == trans->lastTransfer )
			break;
		t = next;
	}
	
	if( next )
		next->prev = nil;		// Unlink for safety
}

// Adds an endpoint to the pending list (if not already in there) at the tail
void
OHCIDriver::addToPending( OHCIPipe* p )
{
	if( p->state >= kPipeState_PendingStart )
		return;
	
	if( pPendingTail )
	{
		pPendingTail->pendingNext = p;
		p->pendingPrev = pPendingTail;
		p->pendingNext = nil;
		pPendingTail = p;
	}
	else
	{
		pPending = pPendingTail = p;
		p->pendingNext = p->pendingPrev = nil;
	}
}

// Removes an endpoint from the pending list
void
OHCIDriver::removeFromPending( OHCIPipe* p )
{
	if( p->pendingNext )
		p->pendingNext->pendingPrev = p->pendingPrev;
	else
		pPendingTail = p->pendingPrev;
	if( p->pendingPrev )
		p->pendingPrev->pendingNext = p->pendingNext;
	else
		pPending = p->pendingNext;
}

void
OHCIDriver::waitForRhStatusChange()
{
	enableInt( kIntRootHubStatusChange );
	rootHubStatSem.down();
}

void
OHCIDriver::disableList( OHCIPipeList list )
{
	// Periodic and isochronous lists are never disabled
	if( list == kListControl )
		setControl( getControl() & ~kControlListEnable );
	else if( list == kListBulk )
		setControl( getControl() & ~kBulkListEnable );
}

void
OHCIDriver::enableSOF()
{
	// We take care not to flag out a valid pending SOF interrupt
	if( (getIntMask() & kIntStartOfFrame) == 0 )
	{
		clearInt( kIntStartOfFrame );
		enableInt( kIntStartOfFrame );
	}
}

void
OHCIDriver::doRetireAborted( OHCIPipe *p )
{
	// Walk through all transfers and, for each one where a transaction is pending,
	// unlink the transaction and dispose of its transfers
	OHCITransfer			*t;
	OHCITransaction		*toAbort;
	t = p->head;
	while( t )
	{
		if( t->transaction->abortPending )
		{
			toAbort = t->transaction;
			t = toAbort->lastTransfer->next;
			
			doRetireTransaction( toAbort, kUSBErr_Aborted );
		}
		else
			t = t->next;
	}
}

void
OHCIDriver::doRetireTransaction( OHCITransaction *trans, USBErr err, Boolean unlink )
{
	if( unlink )
	{
		// Unlink this transaction
		// This assumes the HC can't currently touch this pipe
		OHCITransfer				*t = trans->lastTransfer->next;
		OHCIPipe					*p = trans->pipe;
		if( t )
		{
			p->setHeadPtr( t->physAddr );
			p->head = t;
			p->head->prev->next = nil;
			p->head->prev = nil;
		}
		else
		{
			p->setHeadPtr( p->placeHolder->physAddr );
			p->placeHolder->prev = nil;
			p->head = nil;
		}
	}
	
	// Free transfer descriptors of transaction
	freeTDs( trans );
	
	// Mark as done
	trans->err = err;
	trans->doneIO();
}

#pragma mark Ê
#pragma mark OHCITransaction
#pragma mark ============

OHCITransaction::OHCITransaction()
{
	firstTransfer = lastTransfer = nil;
	shortResponseOK = false;
	abortPending = false;
	err = kUSBErr_None;
}

OHCITransaction::~OHCITransaction()
{
}

UInt32
OHCITransaction::ioError()
{
	return err;
}

USBErr
OHCITransaction::abort()
{
	driver->abortTransaction( this );
	return kUSBErr_None;
}

/*static void
printEndpoint( OHCIEndpoint *ed )
{
	usbOut << greenMsg << hexMsg;
	usbOut << "Endpoint at " << (UInt32)ed << " (" << (UInt32)ed->physAddr << "):\n";
	usbOut << "\tConfig: " << (UInt32)ed->getConfig() << "\n";
	usbOut << "\tTailP: " << (UInt32)ed->getTailPtr() << "\n";
	usbOut << "\tHeadP: " << (UInt32)ed->getHeadPtr() << "\n";
	usbOut << "\tNextED: " << (UInt32)ed->getNextEDPtr() << "\n";
	
	OHCITransfer				*td;
	if( ed->head )
	{
		td = ed->head;
		while( td )
		{
			usbOut << "\t-------------------------------\n";
			usbOut << "\tTransfer " << (UInt32)td << " (" << (UInt32)td->physAddr << "):\n";
			usbOut << "\t\tConfig: " << (UInt32)td->getConfig() << ", CBP: " << (UInt32)td->getCurrentBufferPtr() << "\n";
			usbOut << "\t\tNextTD: " << (UInt32)td->getNextTDPtr() << ", BE: " << (UInt32)td->getBufferEndPtr() << "\n";
			td = td->next;
		}
	}
	usbOut << "-----------------------------------\n";
	usbOut << whiteMsg << decMsg;
}

void
OHCIDriver::printList( EDList list )
{
	usbOut << "Printing list " << (UInt32)list << newLine;
	
	OHCIEndpoint				*ed;
	ed = queues[list]->next;
	while( ed )
	{
		printEndpoint( ed );
		ed = ed->next;
	}
}

void
OHCIDriver::printRegs()
{
	usbOut << hexMsg;
	usbOut << redMsg << "Control Partition:\n" << whiteMsg;
	usbOut << "Revision: " << getRevision() << ", Control: " << getControl() << newLine;
	usbOut << "CmdStatus: " << getCommandStatus() << ", IntStatus: " << getIntStatus() << newLine;
	usbOut << "IntMask: " << getIntMask() << newLine;
	
	usbOut << redMsg << "Memory Partition:\n" << whiteMsg;
	usbOut << "HCCA: " << (UInt32)getHCCA() << ", PCurrentED: " << (UInt32)getPeriodCurrentED() << newLine;
	usbOut << "CHeadED: " << (UInt32)getControlHeadED() << ", CCurrentED: " << (UInt32)getControlCurrentED() << newLine;
	usbOut << "BHeadED: " << (UInt32)getBulkHeadED() << ", BCurrentED: " << (UInt32)getBulkCurrentED() << newLine;
	usbOut << "DoneHead: " << (UInt32)getDoneHead() << newLine;
	
	usbOut << redMsg << "Frame Counter Partition:\n" << whiteMsg;
	usbOut << "FmInterval: " << getFmInterval() << ", FmRemaining: " << getFmRemaining() << newLine;
	usbOut << "FmNumber: " << getFmNumber() << ", PeriodicStart: " << getPeriodicStart() << newLine;
	usbOut << "LSThreshold: " << getLSThreshold() << newLine;
	
	usbOut << redMsg << "Root Hub Partition:\n" << whiteMsg;
	usbOut << "RhDescriptorA: " << getRhDescriptorA() << ", RhDescriptorB: " << getRhDescriptorB() << newLine;
	usbOut << "RhStatus: " << getRhStatus() << ", RhPortStatus[0]: " << getRhPortStatus(0) << newLine;
	usbOut << "RhPortStatus[1]: " << getRhPortStatus(1) << newLine;
	
	usbOut << decMsg << newLine;
}*/