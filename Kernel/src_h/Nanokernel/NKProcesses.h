/*
	NKProcesses.h
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
#ifndef __PROCESSES__
#define __PROCESSES__

#include "NKAtomicOperations.h"
#include "NKMemoryManager.h"

// A process ID is a 20-bit number.  Process ID 0 is reserved and process ID 1 is reserved for the kernel.  A virtual segment ID,
// as stored in a SR register or in a PTE, is:
//
//	(n << 20) | ProcessID
//
// Where n is any number from 0 to 15.  In other words, there are a total of 1048574 processes (other than the kernel process)
// available for use.

typedef UInt32 ProcessID;

enum
{
	PROCESS_CURRENT		=	0,			// Can be used anywhere a ProcessID is, to designate the current thread's ProcessID
	PROCESS_KERNEL		=	1,			// The kernel ProcessID
	PROCESS_LAST			=	1048575		// The highest possible ProcessID
};

// A Process is the structure associated with a given process.  A Process consumes no CPU time, rather it is just a structure
// which holds information.  For a Process to start using CPU time, it must allocate Threads.
class Process	:	public KernelObject
{
	UInt32				processGoodSig;
	ProcessID				id;					// This Process' ProcessID
	class Process*			parent;				// This Process' parent Process
	class Process*			nextProcess;			// Next process (global queue)
	class Process*			nextSibling;			// Next process (parent's queue of child processes)
	SpinLock				childListLock;			// Lock for childList
	class Process*			childList;				// List of this process' children processes
	SpinLock				threadListLock;			// Lock for threadList
	class Thread*			threadList;			// List of all threads allocated in this process
	SpinLock				codeFragListLock;		// Lock for codeFragList
	class CodeFragment*	codeFragList;			// List of all code fragments for this process
	
	
	
			void		threadCreated(Thread* theThread);
			void		threadDeleted(Thread* theThread);
			void		subProcessKilled(Process* theProcess);
public:
	MemoryTOC			toc;					// The memory manager TOC for this process
	
	// YOU probably never want to call either of these 2 functions...
	Process(ProcessID id,Process* parent);		// Pass 0 as a process ID to get a new, unused ID
	~Process();
	
			Process*	newSubProcess();
			ProcessID	processID();
			void		enterMemorySpace();	// Sets up this Process' memory space
	
	friend class Thread;
	friend class CodeFragment;
	friend	void		NKInitProcesses();
	friend	void*	operator new(UInt32 len,Process* process);
	friend	void		operator delete(void*);
};

struct CurrProcess
{
	static	Process*	process();			// Returns the current process
	static	Process*	osProcess();		// Returns the current OS process
	static	Process*	osSubProcess();	// Returns the current OS sub-process
};

// This class is only to be used by the kernel, from the kernel segment, or from an OS process on an OS subprocess, from the OS segment.  Any
// other use will cause a hard crash.
class ProcessWindow
{
	Process*	oldProcess;
public:
	ProcessWindow(Process* newProcess);
	~ProcessWindow();
};

// In the PPC memory segment model, processes are set up in the following way:
//	Segment		Length		Assignment
//	0			256 MB		Kernel
//	1			256 MB		Current OS [this is a Process, but not necessarily the current one.]
//							All spawned process of the OS Process have access to this space.
//	2-15			3.5 GB		Current Process [this is the Process's memory space - only 1 Process (per cpu)
//							can view this memory at a time]
//
//	Therefore, SR0 always holds the ProcessID of the Kernel, SR1 always holds the ProcessID of the current OS [yes you
//	can have multiple OS's going at once], and SR2-15 always holds the ProcessID of the current Process.  As you can
//	see, the kernel and the OS are both limited to 256 MB.  However, this is extensible in the future - no addresses are
//	hard-wired into the kernel - meaning that if an OS or kernel ever requires > 256 MB, those segments can be increased
//	at the expense of the Current Process memory space size.
void		NKInitProcesses();

extern Process* kernelProcess;

#endif /* __PROCESSES__ */