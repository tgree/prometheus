/*
	NKProcesses.cp
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
#include "Kernel Types.h"
#include "NKProcesses.h"
#include "NKAtomicOperations.h"
#include "NKThreads.h"
#include "NKMachineInit.h"
#include "NKVideo.h"
#include "ANSI.h"

static NKSpinLock	processListLock;
static Process*	processList = nil;
UInt32			kernelProcessMem[(sizeof(Process) + 3)/4];
Process*			kernelProcess;

void NKInitProcesses()
{
	processListLock.init();
	kernelProcess = new((void*)kernelProcessMem) Process(PROCESS_KERNEL,nil);
}

Process::Process(ProcessID _id,Process* _parent):
	toc(	((_id == PROCESS_KERNEL) ? machine.kernelStart :
			( (!_parent) ? (void*)0x10000000 : (void*)0x20000000 )),
		((_id == PROCESS_KERNEL) ? (0x10000000 - (UInt32)machine.kernelStart) :
			( (!_parent) ? 0x10000000 : 0xE0000000 )),
		((_id == PROCESS_KERNEL) ? ((UInt32)machine.freeMemStart - (UInt32)machine.kernelStart) :
			0))
{
	if(_id)
		id = _id;
	else
	{
		// Randomly choose an unused process ID.  By randomly picking it we are more likely
		// to get a free one the first time than having to search for one, since the ID space is large
		// (1048575 ids) and the number of processes running will generally be small in comparison.
		// If we do a normal search from ID 2 (the first free one), it will be more likely that we
		// need to do a long search to find a free ID, since the numbers would be allocated
		// consecutively.  Of course, if the ID space was small, on the order of the number of processes
		// running, we would do a simple search probably.
		NKSpinLocker	locker(processListLock);
		UInt32	firstID = (rand()*PROCESS_LAST/65535) % PROCESS_LAST;
		if(firstID < 2)
			firstID = 2;
		for(id=firstID;id<=PROCESS_LAST;id++)
		{
			Process*	theProcess = processList;
			while(theProcess)
			{
				if(theProcess->processID() == id)
					break;
				theProcess = theProcess->nextProcess;
			}
			if(!theProcess)
				break;
		}
		if(id > PROCESS_LAST)
		{
			for(id=2;id<firstID;id++)
			{
				Process*	theProcess = processList;
				while(theProcess)
				{
					if(theProcess->processID() == id)
						break;
					theProcess = theProcess->nextProcess;
				}
				if(!theProcess)
					break;
			}
			FatalAssert(id != firstID);
		}
		
		FatalAssert(id <= PROCESS_LAST);
	}
	toc.processID = id;
	
	parent = _parent;
	threadList = nil;
	childList = nil;
	processGoodSig = 'Good';
	nkVideo << "New process ID: " << id << "\n";
	
	NKSpinLocker	locker(processListLock);
	nextProcess = processList;
	processList = this;
}

Process::~Process()
{
	nkVideo << "Killing process ID: " << id << "\n";
	{
		childListLock.lock();
		NKCriticalSection	criticalSection;
		childListLock.unlock();
		while(childList)
			delete childList;
	}
	
	{
		ProcessWindow	window(this);
		threadListLock.lock();
		NKCriticalSection	criticalSection;
		threadListLock.unlock();
		while(threadList)
		{
			if(threadList->process != this)
				threadList->process = this;
			delete threadList;
		}
	}
	
	processGoodSig = 'Bad!';
	
	NKSpinLocker	locker2(processListLock);
	Process*		currProcess = processList;
	Process*		prevProcess = nil;
	while(currProcess)
	{
		if(currProcess == this)
		{
			if(prevProcess)
				prevProcess->nextProcess = nextProcess;
			else
				processList = nextProcess;
			break;
		}
		prevProcess = currProcess;
		currProcess = currProcess->nextProcess;
	}
	
	if(parent)
		parent->subProcessKilled(this);
}

void Process::threadCreated(Thread* theThread)
{
	Assert(processGoodSig == 'Good');
	SpinLocker	locker(threadListLock);
	theThread->nextInProcess = threadList;
	threadList = theThread;
}

void Process::threadDeleted(Thread* theThread)
{
	Assert(processGoodSig == 'Good');
	SpinLocker	locker(threadListLock);
	Thread*		currThread = threadList;
	Thread*		prevThread = nil;
	while(currThread)
	{
		if(currThread == theThread)
		{
			if(prevThread)
				prevThread->nextInProcess = currThread->nextInProcess;
			else
				threadList = threadList->nextInProcess;
			break;
		}
		prevThread = currThread;
		currThread = currThread->nextInProcess;
	}
}

void Process::subProcessKilled(Process* theProcess)
{
	Assert(processGoodSig == 'Good');
	SpinLocker	locker(childListLock);
	Process*		currProcess = childList;
	Process*		prev = nil;
	while(currProcess)
	{
		if(currProcess == theProcess)
		{
			if(prev)
				prev->nextProcess = theProcess->nextProcess;
			else
				childList = childList->nextProcess;
			break;
		}
		prev = currProcess;
		currProcess = currProcess->nextProcess;
	}
}

Process* Process::newSubProcess()
{
	Assert(processGoodSig == 'Good');
	SpinLocker	locker(childListLock);
	Process*		retProcess = new Process(0,this);
	retProcess->nextSibling = childList;
	childList = retProcess;
	
	return retProcess;
}

ProcessID Process::processID()
{
	Assert(processGoodSig == 'Good');
	return id;
}

void Process::enterMemorySpace()
{
	Assert(processGoodSig == 'Good');
	
	// Only modify the OS segment if this is an OS process (i.e. has no parent)
	UInt32 stackSeg = ((_getSP() >> 28) & 0x0000000F);
	if(!parent && stackSeg != 1)
		_setSR(1,((_getSR(1) & 0xFFF00000) | id));
	
	UInt32 srSettings = (_getSR(2) & 0xFF000000);
	for(Int32 i=2;i<16;i++)
	{
		if(i != stackSeg)
			_setSR(i,0x00100000*i | srSettings | id);
	}
}

Process* CurrProcess::process()
{
	if((_getSR(15) & 0x000FFFFF) == PROCESS_KERNEL)
		return kernelProcess;
	
	return CurrThread::thread()->process;
}

ProcessWindow::ProcessWindow(Process* newProcess)
{
	oldProcess = CurrProcess::process();
	CurrThread::thread()->process = newProcess;
	newProcess->enterMemorySpace();
}

ProcessWindow::~ProcessWindow()
{
	oldProcess->enterMemorySpace();
	CurrThread::thread()->process = oldProcess;
}
