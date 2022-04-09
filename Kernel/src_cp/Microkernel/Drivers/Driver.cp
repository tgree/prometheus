/*
	Driver.cp
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
	Patrick Varilly		-	Friday, 12 June 98	-	Added support for enqueueing to the head of the queue,
											for ADB classes
	Patrick Varilly		-	Thu, 13 Jan 2000	-	Changed IOCommand paradigm from polling (via ioDone()), to
											notifying when the command is actually done
*/
#include "Kernel Types.h"
#include "Driver.h"
#include "NKAtomicOperations.h"
#include "NKVideo.h"
#include "ANSI.h"
#include "NKThreads.h"

Driver::Driver(ConstASCII8Str driverName)
{
	_driverName = new ASCII8[strlen(driverName) + 1];
	strcpy(const_cast<ASCII8Str>(_driverName),driverName);
}

Driver::~Driver()
{
}

ConstASCII8Str Driver::driverName()
{
	return _driverName;
}

IOCommand::IOCommand()
{
	process = CurrProcess::process();
	processID = process->processID();
	hasDoneIO = false;
	waitingThread = nil;
}

IOCommand::~IOCommand()
{
}

void
IOCommand::doneIO()
{
	CriticalSection		critical(ioLock);
	
	if( !hasDoneIO )
	{
		hasDoneIO = true;
		if( waitingThread )
			waitingThread->doneIO();
	}
}

UInt32 DummyIOCommand::ioError()
{
	return 0;
}

IOCommandDriver::IOCommandDriver(ConstASCII8Str driverName):
	Driver(driverName)
{
}

IOCommandDriver::~IOCommandDriver()
{
	while(commandQueue.numElems())
		;
}

IOCommand* IOCommandDriver::dequeue()
{
	commandQueue.dequeue();
	return commandQueue[0];
}

void IOCommandDriver::enqueue(IOCommand* cmd)
{
	if(commandQueue.enqueue(cmd))
		startAsyncIO(cmd);
}

void IOCommandDriver::enqueueAtHead(IOCommand* cmd)
{
	if(commandQueue.enqueueAtHead(cmd))
		startAsyncIO(cmd);
}

IOCommand* IOCommandDriver::getHead()
{
	return commandQueue[0];
}

IOCommandFullDuplexDriver::IOCommandFullDuplexDriver(ConstASCII8Str driverName):
	Driver(driverName)
{
}

IOCommandFullDuplexDriver::~IOCommandFullDuplexDriver()
{
	while(readQueue.numElems())
		;
	while(writeQueue.numElems())
		;
}

IOCommand* IOCommandFullDuplexDriver::dequeueRead()
{
	readQueue.dequeue();
	return readQueue[0];
}

IOCommand* IOCommandFullDuplexDriver::dequeueWrite()
{
	writeQueue.dequeue();
	return writeQueue[0];
}

void IOCommandFullDuplexDriver::enqueueRead(IOCommand* cmd)
{
	if(readQueue.enqueue(cmd))
		startAsyncIORead(cmd);
}

void IOCommandFullDuplexDriver::enqueueWrite(IOCommand* cmd)
{
	if(writeQueue.enqueue(cmd))
		startAsyncIOWrite(cmd);
}