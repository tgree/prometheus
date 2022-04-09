/*
	NKAtomicOperations.cp
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
	Patrick Varilly		-	Friday, 12 June 98	-	Added support for enqueueing to the head of the list
											to support new ADB stuff
	Patrick Varilly		-	Thursday, 18 Nov 99 -	Added Boolean result to dequeue(elem) to know if the element
											was found.
*/
#include "Macros.h"
#include "Compiler.h"
#include "Kernel Types.h"
#include "NKVideo.h"
#include "NKAtomicOperations.h"
#include "NKInterruptSafeList.h"
#include "NKDebuggerNub.h"

static void		_Lock(register UInt32* addr);
static void		_Unlock(register UInt32* addr);

void NKSpinLock::init()
{
	reservationAddr = (UInt32*)ROUND_UP(16,(UInt32)cacheLine);
	*reservationAddr = 0;
	numRequests = 0;
	lockInited = true;
}

void NKSpinLock::lock()
{
	Assert(lockInited);
	numRequests++;
	_Lock(reservationAddr);
	numRequests--;
}

void NKSpinLock::unlock()
{
	Assert(lockInited);
	_Unlock(reservationAddr);
}

Boolean NKSpinLock::isLocked()
{
	Assert(lockInited);
	return (*reservationAddr != 0);
}

SpinLock::SpinLock()
{
	reservationAddr = (UInt32*)( ((UInt32)cacheLine + 12) & 0xFFFFFFFC );
	*reservationAddr = false;
	numRequests = 0;
	lockInited = true;
}

SpinLock::~SpinLock()
{
	if(numRequests)
	{
		nkVideo << "PANIC: Deleted a SpinLock with outstanding requests!\n";
		while(numRequests)
			;
	}
}

void SpinLock::lock()
{
	if(lockInited)
	{
		numRequests++;
		_Lock(reservationAddr);
		numRequests--;
	}
}

void SpinLock::unlock()
{
	if(lockInited)
		_Unlock(reservationAddr);
}

Boolean SpinLock::isLocked()
{
	// Assumes lock can't be changed to check this
	
	if(lockInited)
		return (*reservationAddr != 0);
	else
		return false;		// Should this return true?
}

_NKInterruptSafeList::_NKInterruptSafeList()
{
	head = tail = nil;
}

_NKInterruptSafeList::~_NKInterruptSafeList()
{
	_NKInterruptSafeListElem*	elem = head;
	_NKInterruptSafeListElem*	nextElem;
	while(elem)
	{
		nextElem = elem->next;
		delete elem;
		elem = elem->next;
	}
}

Boolean _NKInterruptSafeList::_enqueue(void* data)
{
	_NKInterruptSafeListElem*	elem = new _NKInterruptSafeListElem;
	elem->next = nil;
	elem->data = data;
	
	CriticalSection				critical(listLock);
	
	if(tail)
	{
		tail->next = elem;
		tail = elem;
		return false;
	}
	else
	{
		head = tail = elem;
		return true;
	}
}

Boolean _NKInterruptSafeList::_enqueueAtHead(void* data)
{
	_NKInterruptSafeListElem*	elem = new _NKInterruptSafeListElem;

	elem->data = data;

	CriticalSection				critical(listLock);
	
	elem->next = head;
	
	if(head)
	{
		if( head == tail )
			tail = elem;
		head = elem;
		return false;
	}
	else
	{
		head = tail = elem;
		return true;
	}
}

void* _NKInterruptSafeList::_dequeue(void)
{
	_NKInterruptSafeListElem*	elem;
	
	{
		CriticalSection		critical(listLock);
		
		if(head == nil)
			return nil;
		
		elem = head;
		head = head->next;
		if(elem == tail)
			tail = nil;
	}
	
	void*	retVal = elem->data;
	
	delete elem;
	
	return retVal;
}

Boolean _NKInterruptSafeList::_dequeue(void* data)
{
	_NKInterruptSafeListElem*	elem;
	
	{
		CriticalSection		critical(listLock);
		
		if(head == nil)
			return false;
		
		elem = head;
		_NKInterruptSafeListElem*	prev = nil;
		while(elem)
		{
			if(elem->data == data)
			{
				if(prev)
					prev->next = elem->next;
				else
					head = elem->next;
				
				if(!elem->next)
					tail = prev;
				
				break;
			}
			elem = elem->next;
		}
	}
	
	delete elem;
	
	return (elem != nil);
}

void* _NKInterruptSafeList::_getElem(UInt32 index)
{
	CriticalSection				critical(listLock);
	
	_NKInterruptSafeListElem*	elem = head;
	index++;
	while(--index && elem)
		elem = elem->next;
	
	if(!index && elem)
		return elem->data;
	
	return nil;
}

UInt32 _NKInterruptSafeList::numElems()
{
	UInt32				num = 0;
	
	CriticalSection				critical(listLock);
	
	_NKInterruptSafeListElem*	elem = head;
	while(elem)
	{
		num++;
		elem = elem->next;
	}
	
	return num;
}

__asm__ UInt32 FetchAndNop(register UInt32* addr)
{
	// The fetch and no-op primitive atomically returns the current value in a word in memory.
loop:	lwarx	r4,r0,r3;
	stwcx.	r4,r0,r3;
	bne-		loop;
	
	mr		r3,r4;
	blr;
}

__asm__ UInt32 FetchAndStore(register UInt32* addr,register UInt32 val)
{
	// The fetch and store primitive atomically loads and replaces a word in memory.  Returns the old value.
loop:	lwarx	r5,r0,r3;
	stwcx.	r4,r0,r3;
	bne-		loop;
	
	mr		r3,r5;
	blr;
}

__asm__ UInt32 FetchAndAdd(register UInt32* addr,register UInt32 amount)
{
	// The fetch and add primitive atomically increments a word in memory.  Returns the old value.
loop:	lwarx	r5,r0,r3;
	add		r0,r4,r5;
	stwcx.	r0,r0,r3;
	bne-		loop;
	
	mr		r3,r5;
	blr;
}

__asm__ Boolean TestAndSet(register UInt32* addr,register UInt32 newVal,register UInt32* oldVal)
{
	// Test and set tests that a word in memory is zero.  If it is zero, it sets it to the new value, otherwise
	// it leaves it alone.  It returns true if the value was zero (i.e. the set succeeded) and false if it was non-zero.
loop:		lwarx	r6,r0,r3;
		cmpwi	r6,0;
		bne		nonZero;
		stwcx.	r4,r0,r3;
		bne-		loop;
zero:	li		r3,1;
		stw		r6,0(r5);
		blr;
nonZero:	li		r3,0;
		blr;
}

static __asm__ void _Lock(register UInt32* addr)
{
	// Accquire a lock
	li		r4,1;
loop:	lwarx	r5,r0,r3;
	cmpwi	r5,0;
	bne		loop;
	stwcx.	r4,r0,r3;
	bne-		loop;
	isync;
	blr;
}

static __asm__ void _Unlock(register UInt32* addr)
{
	// Release a lock
	sync;
	li		r4,0;
	stw		r4,0(r3);
	blr;
}