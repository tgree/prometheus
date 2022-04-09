/*
	Driver.h
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
	Patrick Varilly		-	Friday, 12 June 98	-	Updated IOCommandDriver queueing for new ADB classes
	Terry Greeniaus	-	Sat, 13 June 98	-	Added support for new driver initialization procedure
*/
#ifndef __DRIVER__
#define __DRIVER__

#include "NKProcesses.h"
#include "NKAtomicOperations.h"
#include "NKInterruptSafeList.h"

class Driver
{
protected:
	ConstASCII8Str	_driverName;
public:
	Driver(ConstASCII8Str driverName);	// Your constructor should map all necessary memory and allocate any buffers, etc.  You also register your interrupt here.
	virtual ~Driver();				// Your destructor should release all memory allocated in the constructor
	
	// ***** The following is subject to change, since I'm not sure I really like this driver format:
	virtual	void	initialize() = 0;		// Initialize will be called for you to reset the device - and make it so that it generates NO interrupts.
	virtual	void	start() = 0;		// Start is called when you should set up the device to start generating interrupts.
	virtual	void	stop() = 0;		// Stop is called when you should disable your device from generating interrupts (without resetting the device if possible).
	
			ConstASCII8Str	driverName();
};

struct IOCommand	:	public KernelObject
{
	ProcessID			processID;
	Process*			process;
	SpinLock			ioLock;
	UInt32			hasDoneIO;
	Thread			*waitingThread;
	
	IOCommand();
	virtual ~IOCommand();
	
			void			doneIO();		// Call when I/O is done
	virtual	UInt32		ioError() = 0;	// Returns the error for this IOCommand (note that this is class-dependant!)
};

struct DummyIOCommand	:	public IOCommand
{
	virtual	UInt32		ioError();
};

class IOCommandDriver	:	public Driver
{
	NKInterruptSafeList<IOCommand>	commandQueue;
protected:
	IOCommandDriver(ConstASCII8Str driverName);
	virtual ~IOCommandDriver();
public:
	IOCommand*	dequeue();	// Call this to get the next IOCommand to process.  You should call it when processing of any IOCommand has completed.  If it doesn't return nil,
						// you must start processing the command it returns (asynchronously of course)
			void	enqueue(IOCommand* cmd);	// Call this to enqueue an IOCommand and to process it (i.e. call it when state is ready to start asynch IO)
			void	enqueueAtHead(IOCommand* cmd);	// Call this to enqueue an IOCommand and to process it ASAP (i.e. call it as soon as current command finishes, even if there are others)
	IOCommand*	getHead();	// Call this to get the next IOCommand to be processed.  Unlike dequeue, this doesn't remove the item.
			
	virtual	void	startAsyncIO(IOCommand* cmd) = 0;	// Overload this.  It will be called to start up processing of commands in the queue.  You should probably call this routine each time you
												// dequeue() a new command too, but that depends on how your driver works.
};

class IOCommandFullDuplexDriver	:	public Driver
{
	NKInterruptSafeList<IOCommand>	readQueue;
	NKInterruptSafeList<IOCommand>	writeQueue;
protected:
	IOCommandFullDuplexDriver(ConstASCII8Str driverName);
	virtual ~IOCommandFullDuplexDriver();
public:
			IOCommand*	dequeueRead();		// Call this to get the next input IOCommand to process.  You should call it when processing of the previous input IOCommand completes.
			IOCommand*	dequeueWrite();	// Call this to get the next output IOCommand to process.  You should call it when processing of the previous ouput IOCommand completes.
	
			void			enqueueRead(IOCommand* cmd);	// Call this to enqueue an input IOCommand and to process it
			void			enqueueWrite(IOCommand* cmd);	// Call this to enqueue an output IOCommand and to process it
	
	virtual	void			startAsyncIORead(IOCommand* cmd) = 0;		// Overload.  It is called when you should start processing an input IOCommand.
	virtual	void			startAsyncIOWrite(IOCommand* cmd) = 0;	// Overload.  It is called when you should start processing an output IOCommand.
};

#endif
