/*
	OHCIDriver.h
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
	Patrick Varilly		-	Fri, 26 Nov 99		-	Original creation of file
*/

#ifndef __OHCI_DRIVER__
#define __OHCI_DRIVER__

#include "OHCIBits.h"
#include "USBHCDriver.h"
#include "OHCIDescriptors.h"
#include "NKVirtualMemory.h"

// Possible lists pipe can be in
typedef enum OHCIPipeList
{
	kListControl = 0,
	kListBulk = kListControl+1,
	kListInt32ms = kListBulk+32,			// 32 32ms lists
	kListInt16ms = kListInt32ms+16,		// 16 16ms lists
	kListInt8ms = kListInt16ms+8,			// 8 8ms lists
	kListInt4ms = kListInt8ms+4,			// 4 4ms lists
	kListInt2ms = kListInt4ms+2,			// 2 2ms lists
	kListInt1ms = kListInt2ms+1,			// 1 1ms list
	kListIsochronous = kListInt1ms+1,
	kListMax = kListIsochronous+1			// 63 interrupt lists, one iso, one control, one bulk
} OHCIPipeList;

// Possible states for an pipe
typedef enum OHCIPipeState
{
	kPipeState_Creating = 0,
	kPipeState_Running,
	kPipeState_Paused,
	
	kPipeState_PendingStart,
	kPipeState_PendingAbort = kPipeState_PendingStart,
	kPipeState_PendingPause,
	kPipeState_PendingDelete
} OHCIPipeState;

// Actual pipe and transfer structures (size must be a multiple of 16)
class OHCITransfer;
class OHCIPipe;
class OHCITransaction;

class OHCIPipe :
	public OHCIEndpointDescriptor,
	public USBHCPipe			// This just lets us cast easily, USBHCPipe is compltely blank
{
public:
	inline UInt32			getConfig() { NKFlushCaches( this, 16 ); return ReadUReg32LE( &config ); }
	inline void				setConfig( UInt32 val ) { WriteUReg32LE( val, &config ); NKFlushCaches( this, 16 ); }
	inline void*			getTailPtr() { NKFlushCaches( this, 16 ); return (void*)ReadUReg32LE( &tailPtr ); }
	inline void				setTailPtr( void* val ) { WriteUReg32LE( (UInt32)val, &tailPtr ); }
	inline void*			getHeadPtr() { NKFlushCaches( this, 16 ); return (void*)ReadUReg32LE( &headPtr ); }
	inline void				setHeadPtr( void* val )
		{ WriteUReg32LE( ((UInt32)getHeadPtr() & (kEDToggleCarryMask | kEDHalted)) | (UInt32)val, &headPtr ); NKFlushCaches( this, 16 ); }
	inline void				clearHalted()
		{ WriteUReg32LE( (UInt32)getHeadPtr() & ~kEDHalted, &headPtr ); NKFlushCaches( this, 16 ); }
	inline void				clearToggleCarry()
		{ WriteUReg32LE( (UInt32)getHeadPtr() & ~kEDToggleCarryMask, &headPtr ); NKFlushCaches( this, 16 ); }
	inline void*			getNextEDPtr() { NKFlushCaches( this, 16 ); return (void*)ReadUReg32LE( &nextEDPtr ); }
	inline void				setNextEDPtr( void* val ) { WriteUReg32LE( (UInt32)val, &nextEDPtr ); NKFlushCaches( this, 16 ); }
	
	// Endpoint info
	OHCIPipeList			list;
	OHCIPipeState			state;
	
	OHCIPipe				*next, *prev;				// Virtual pipe list (one for each queue)
	OHCITransfer			*head, *placeHolder;		// Transfer list
	OHCIPipe				*pendingNext, *pendingPrev;	// Pipe pending list
	
	// For OHCIAllocator (keeps track of physical addresses)
	Ptr					allocPtr;
	void*				physAddr;
};

class OHCITransfer :
	public OHCITransferDescriptor
{
public:
	inline UInt32			getConfig() { NKFlushCaches( this, 16 ); return ReadUReg32LE( &config ); }
	inline void				setConfig( UInt32 val ) { WriteUReg32LE( val, &config ); NKFlushCaches( this, 16 ); }
	inline UInt32			getCurrentBufferPtr() { NKFlushCaches( this, 16 ); return ReadUReg32LE( &currentBufferPtr ); }
	inline void				setCurrentBufferPtr( void* val ) { WriteUReg32LE( (UInt32)val, &currentBufferPtr ); NKFlushCaches( this, 16 ); }
	inline UInt32			getNextTDPtr() { NKFlushCaches( this, 16 ); return ReadUReg32LE( &nextTDPtr ); }
	inline void				setNextTDPtr( void* val ) { WriteUReg32LE( (UInt32)val, &nextTDPtr ); NKFlushCaches( this, 16 ); }
	inline UInt32			getBufferEndPtr() { NKFlushCaches( this, 16 ); return ReadUReg32LE( &bufferEndPtr ); }
	inline void				setBufferEndPtr( void* val ) { WriteUReg32LE( (UInt32)val, &bufferEndPtr ); NKFlushCaches( this, 16 ); }
	
	OHCITransfer			*next, *prev;			// Virtual transfer list for endpoint
	OHCITransaction		*transaction;			// Transaction we belong to
	OHCITransfer			*doneNext;			// For reversing the HCs physical done queue
	
	// For OHCIAllocator (keeps track of physical addresses)
	OHCITransfer			*hashNext, *hashPrev;
	Ptr					allocPtr;
	void*				physAddr;
};

class OHCITransaction :
	public USBTransaction
{
public:
						OHCITransaction();
	virtual				~OHCITransaction();
	
	virtual UInt32			ioError();
	
	virtual USBErr			abort();
	
protected:
	friend class OHCIDriver;
	
	OHCITransfer			*firstTransfer, *lastTransfer;	// Transfers associated with this transaction
	Boolean				shortResponseOK;			// True if the response from the device needn't fill the buffer given
	Boolean				abortPending;				// True if this transaction will be aborted at the next SOF
	USBErr				err;						// The error that occured, if any
	
	OHCIDriver			*driver;					// The driver that issued this transaction
	OHCIPipe				*pipe;					// The pipe this transaction is running on
};

class OHCIRootHub;

class OHCIDriver :
	public USBHCDriver,
	public OHCIAllocator
{
public:
						OHCIDriver();
	virtual				~OHCIDriver();
	
	// Root hub (into slot 1, if it's integrated into the driver.  Otherwise, it's probed by the USB driver on top)
	virtual USBHub*		getRootHub( USBBus* bus );
	
	// General USB protocol handling 
	virtual UInt64			getFrameNumber();			// Will overflow after about 584.5 million years... dang!
	
	// Pipe management
	virtual USBErr			createControlPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
							ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize );
	//virtual USBErr		createBulkPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
	//						const Boolean fullSpeed, const UInt16 maxPacketSize );
	virtual USBErr			createInterruptPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
							ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize,
							const UInt32 interval );
							// interval is number of ms between polls.  Actual rate guaranteed to be less or equal.
	//virtual USBErr		createIsochronousPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
	//						ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize );
	virtual USBErr			abortPipe( USBHCPipe* pipe );
	virtual USBErr			resetPipe( USBHCPipe* pipe );
	virtual USBErr			pausePipe( USBHCPipe* pipe );
	virtual USBErr			resumePipe( USBHCPipe* pipe );
	virtual USBErr			deletePipe( USBHCPipe* pipe );
	
	// Transactions
	virtual USBErr			controlTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe,
							ConstUSBControlSetupPtr setup, ConstPtr buffer = nil, Boolean shortResponseOK = false );
	virtual USBErr			interruptTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe,
							ConstPtr buffer, const UInt32 bufferSize, Boolean shortResponseOK = false );
	
	// For Driver
	virtual void			initialize();
	virtual void			start();
	virtual void			stop();
	
protected:
	friend class OHCIRootHub;
	friend class OHCITransaction;
	
	// For OHCITransaction to abort itself
	void					abortTransaction( OHCITransaction *trans );
	
	// Interrupts
	void					ohciInterrupt();
	
	// * HC register readers
	// Control and Status
	virtual UInt32			getRevision() = 0;
	virtual UInt32			getControl() = 0;
	virtual UInt32			getCommandStatus() = 0;
	virtual UInt32			getIntStatus() = 0;
	virtual UInt32			getIntMask() = 0;
	// Memory Pointers
	virtual void*			getHCCA() = 0;
	virtual void*			getPeriodCurrentED() = 0;
	virtual void*			getControlHeadED() = 0;
	virtual void*			getControlCurrentED() = 0;
	virtual void*			getBulkHeadED() = 0;
	virtual void*			getBulkCurrentED() = 0;
	virtual void*			getHcDoneHead() = 0;
	// Frame Counter
	virtual UInt32			getFmInterval() = 0;
	virtual UInt32			getFmRemaining() = 0;
	virtual UInt32			getFmNumber() = 0;
	virtual UInt32			getPeriodicStart() = 0;
	virtual UInt32			getLSThreshold() = 0;
	// Root Hub
	virtual UInt32			getRhDescriptorA() = 0;
	virtual UInt32			getRhDescriptorB() = 0;
	virtual UInt32			getRhStatus() = 0;
	virtual UInt32			getRhPortStatus( UInt8 port ) = 0;
	
	// * HC register writers
	// Control and Status
	virtual void			setControl( UInt32 val ) = 0;
	virtual void			setCommandStatus( UInt32 val ) = 0;
	virtual void			enableInt( UInt32 val ) = 0;
	virtual void			disableInt( UInt32 val ) = 0;
	virtual void			clearInt( UInt32 val ) = 0;
	// Memory Pointers
	virtual void			setHCCA( void* val ) = 0;
	virtual void			setControlHeadED( void* val ) = 0;
	virtual void			setControlCurrentED( void* val ) = 0;
	virtual void			setBulkHeadED( void* val ) = 0;
	virtual void			setBulkCurrentED( void* val ) = 0;
	virtual void			clearHcDoneHead() = 0;
	// Frame Counter
	virtual void			setPeriodicStart( UInt32 val ) = 0;
	virtual void			setFmInterval( UInt32 val ) = 0;
	virtual void			setLSThreshold( UInt32 val ) = 0;
	// Root Hub
	virtual void			setRhDescriptorA( UInt32 val ) = 0;
	virtual void			setRhDescriptorB( UInt32 val ) = 0;
	virtual void			setRhStatus( UInt32 val ) = 0;
	virtual void			setRhPortStatus( UInt8 port, UInt32 val ) = 0;
	// Legacy
	virtual void			clearHceControl() = 0;

	// * HCCA readers
	inline void*			getInterruptED( UInt8 fmNumber ) { NKFlushCaches( hcca, sizeof(OHCIHCCA) ); return (void*)ReadUReg32LE( &hcca->interruptTable[fmNumber] ); }
	inline UInt32			getFrameNumberFromHC() { NKFlushCaches( hcca, sizeof(OHCIHCCA) ); return ReadUReg16LE( &hcca->frameNumber ); }
	inline void*			getDoneHead() { NKFlushCaches( hcca, sizeof(OHCIHCCA) ); return (void*)ReadUReg32LE( &hcca->doneHead ); }
	
	// * HCCA writers
	inline void				setInterruptED( UInt8 fmNumber, void* ed ) { WriteUReg32LE( (UInt32)ed, &hcca->interruptTable[fmNumber] ); NKFlushCaches( hcca, sizeof(OHCIHCCA) ); }
	inline void				clearDoneHead() { hcca->doneHead = nil; NKFlushCaches( hcca, sizeof(OHCIHCCA) ); }
	
	// The various queues
	OHCIPipe				*queues[kListMax];		// Dummy EDs at heads of lists
	OHCIPipe				*pPending, *pPendingTail;// All endpoints whose state is kEDState_PendingXXX go here
	void					addToPending( OHCIPipe* pipe );
	void					removeFromPending( OHCIPipe* pipe );
	
	// Lock for MP machines
	SpinLock				ohciLock;
	
	// The root hub
	OHCIRootHub			*rootHub;
	MutexLock			rootHubStatSem;
	void					waitForRhStatusChange();
	
	// Private functions
	USBErr				doCreatePipe( USBHCPipe*& outPipe, ConstUSBDevID address,
							ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize,
							const OHCIPipeList list );
	USBErr				doAbortPipe( OHCIPipe *p );
	USBErr				doPausePipe( OHCIPipe *p );
	USBErr				doResumePipe( OHCIPipe *p );
	
	void					doAbortTransaction( OHCITransaction *trans );
	void					doRetireAborted( OHCIPipe *p );
	void					doRetireTransaction( OHCITransaction *trans, USBErr err, Boolean unlink = true );
	
	void					handleDoneTransfers();
	
	OHCITransfer*			getFreeTransfer( OHCIPipe *p );
	void					linkTransfers( OHCIPipe* p );
	
	void					freeTDs( OHCITransaction *trans );
	
	void					disableList( OHCIPipeList list );
	void					enableSOF();
	
//	void					printList( OHCIPipeList list );
//	void					printRegs();
	
private:
	OHCIHCCA				*hcca;
	UInt64				frameNumberHigh;
};

#endif /* __OHCI_DRIVER__ */