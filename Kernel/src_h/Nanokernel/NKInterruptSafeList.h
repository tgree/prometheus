/*
	NKInterruptSafeList.h
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
	Terry Greeniaus	-	Sunday, 9 August 98	-	Original creation of file (moved from NKAtomicOperations.h)
	Patrick Varilly		-	Thursday, 18 Nov 99 -	Added Boolean result to dequeue(elem) to know if the element
											was found.
*/
#ifndef __NK_INTERRUPT_SAFE_LIST__
#define __NK_INTERRUPT_SAFE_LIST__

#include "NKMemoryManager.h"

class _NKInterruptSafeListElem	:	public KernelObject
{
	class _NKInterruptSafeListElem*	next;
	void*						data;
	
	friend class _NKInterruptSafeList;
};

class _NKInterruptSafeList
{
	SpinLock					listLock;
	_NKInterruptSafeListElem*	head;
	_NKInterruptSafeListElem*	tail;

protected:
	_NKInterruptSafeList();
	virtual ~_NKInterruptSafeList();
	
	Boolean	_enqueue(void* data);		// Returns true if this is the only element in the list
	Boolean	_enqueueAtHead(void* data);	// Returns true if this is the only element in the list
	void*	_dequeue(void);			// Returns and dequeues the first element in the list (nil if no elements in the list)
	Boolean	_dequeue(void* data);		// Dequeues the element data, if in the list.  Returns true if it was in the list.
	void*	_getElem(UInt32 index);		// Returns the nth element in the list (doesn't dequeue it)
public:
	UInt32	numElems();
};

template<class T>
struct NKInterruptSafeList	:	public _NKInterruptSafeList
{
	Boolean	enqueue(T* data)		{return _enqueue(reinterpret_cast<void*>(data));}	// Returns true if this is the only element in the list (useful for starting async io operations - you know they aren't already going if this returns true)
	Boolean	enqueueAtHead(T* data)	{return _enqueueAtHead(reinterpret_cast<void*>(data));}// Returns true if this is the only element in the list
	T*		dequeue(void)			{return reinterpret_cast<T*>(_dequeue());}		// Returns and dequeues the first element in the list (the oldest element in the list)
	Boolean	dequeue(T* data)		{return _dequeue(reinterpret_cast<void*>(data));}	// Dequeues the element data, if in the list.  Returns true if it was in the list.
	T*		operator[](UInt32 index)	{return reinterpret_cast<T*>(_getElem(index));}	// Returns the nth element in the list (doesn't dequeue it)
};

#endif /* __NK_INTERRUPT_SAFE_LIST__ */