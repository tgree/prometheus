/*
	NKAtomicOperations.h
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
	Patrick Varilly		-	Friday, 12 June 98	-	Added support for enqueueAtHead() for new ADB classes
*/
#ifndef __RESERVATIONS__
#define __RESERVATIONS__

#include "Assembly.h"
#include "Kernel Types.h"
#include "NKVideo.h"

class NKSpinLock
{
	// NKMutexLock can be used as a global variable MutexLock in the NanoKernel before C++ objects
	// are initialized (which happens in the MicroKernel).  Make sure you call the init() function first though!
	UInt32		cacheLine[7];
	UInt32*		reservationAddr;
	UInt32		numRequests;
	Boolean		lockInited;
public:
	void			init();
	
	void			lock();
	void			unlock();
	Boolean		isLocked();
};

class NKSpinLocker
{
	NKSpinLock&	theLock;
	Boolean		myLock;
public:
	NKSpinLocker(NKSpinLock& nkMutexLock):theLock(nkMutexLock)
	{
		myLock = 0;
		theLock.lock();
		myLock = 1;
	}
	~NKSpinLocker()
	{
		if(myLock)
			theLock.unlock();
	}
	
	void	lock()
	{
		if(myLock)
			return;
		theLock.lock();
		myLock = 1;
	}
	void	unlock()
	{
		if(myLock)
		{
			myLock = 0;
			theLock.unlock();
		}
	}
};

class SpinLock
{
	UInt32		cacheLine[7];
	UInt32*		reservationAddr;
	UInt32		numRequests;
	Boolean		lockInited;
public:
	SpinLock();
	~SpinLock();
	
	void		lock();
	void		unlock();
	Boolean	isLocked();
};

class SpinLocker
{
	SpinLock&	theLock;
	Boolean		myLock;
public:
	SpinLocker(SpinLock& mutexLock):theLock(mutexLock)
	{
		myLock = 0;
		theLock.lock();
		myLock = 1;
	}
	~SpinLocker()
	{
		if(myLock)
			theLock.unlock();
	}
	
	void	lock()
	{
		if(myLock)
			return;
		theLock.lock();
		myLock = 1;
	}
	void	unlock()
	{
		if(myLock)
		{
			myLock = 0;
			theLock.unlock();
		}
	}
};

class NKCriticalSection
{
	UInt32	oldMSR;
public:
	NKCriticalSection()	{oldMSR = DisableInterrupts();}
	~NKCriticalSection()	{SetMSR(oldMSR);}
};

class CriticalSection
{
	UInt32	oldMSR;
	SpinLock&	theLock;
	Boolean	myLock;
	
public:
	CriticalSection(SpinLock& inLock) : theLock(inLock) { oldMSR = DisableInterrupts(); theLock.lock(); myLock = true; }
	~CriticalSection() { if( myLock ) theLock.unlock(); SetMSR(oldMSR); }
	
	void	lock()
	{
		if(myLock)
			return;
		theLock.lock();
		myLock = 1;
	}
	void	unlock()
	{
		if(myLock)
		{
			myLock = 0;
			theLock.unlock();
		}
	}
};

UInt32	FetchAndNop(register UInt32* addr);
UInt32	FetchAndStore(register UInt32* addr,register UInt32 val);
UInt32	FetchAndAdd(register UInt32* addr,register UInt32 amount);
Boolean	TestAndSet(register UInt32* addr,register UInt32 newVal,register UInt32* oldVal);

#endif /* __RESERVATIONS__ */