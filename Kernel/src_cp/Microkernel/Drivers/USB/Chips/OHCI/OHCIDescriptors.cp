/*
	OHCIDescriptors.cp
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

/*

	Transfer descriptors are kept all linked together in a hash table, indexed from a number derived from the
*physical* address of the descriptor.  This is because there is no reliable way to go from physical-to-virtual
using the page tables (e.g., many virtual pages pointing to the same physical page).
	Since the descriptors must be aligned on a 16-byte boundary (see OHCI specs), we derive the hash index
from bits to 22 to 27 (the last 6 bits, without counting the last 4 bits, which will always be zero) of the physical
address.  This gives us a 64 list hash table, which will more than suffice for our purposes.
	Note that there is only *one* hash table for transfer descriptors, as we never need to translate phys->virt
with endpoint descriptors.
	The 64-entry size can be adjusted by changing NUM_HASH_BITS (currently 6); see OHCIDescriptors.h.

*/

#include "OHCIDriver.h"
#include "NKProcesses.h"
#include "Macros.h"
#include "NKVirtualMemory.h"

OHCIAllocator::OHCIAllocator()
{
	// Initialize the hash table
	UInt32						i;
	for( i = 0; i < OHCI_HASH_TABLE_SIZE; i++ )
		tTable[i] = nil;
}

OHCIAllocator::~OHCIAllocator()
{
}

OHCIPipe*
OHCIAllocator::createPipe()
{
	// Create it and align it to a 16-byte boundary;
	Ptr							ptr, alignedPtr;
	ptr = new(kernelProcess) Int8[sizeof(OHCIPipe)+15];
	if( !ptr )
		return nil;
	alignedPtr = (Ptr)ROUND_UP(16, (UInt32)ptr);
	
	OHCIPipe						*p;
	p = new(alignedPtr) OHCIPipe;
	p->allocPtr = ptr;
	
	// Get physical address
	// NKLockMemory( pipe, sizeof( OHCIPipe ) );
	p->physAddr = NKGetPhysical( p, PROCESS_KERNEL );
	
	// Clear other variables
	p->list = kListMax;
	p->state = kPipeState_Creating;
	p->next = p->prev = nil;
	p->head = p->placeHolder = nil;
	p->pendingNext = p->pendingPrev = nil;
	
	// Done!
	return p;
}

void
OHCIAllocator::disposePipe( OHCIPipe* pipe )
{
	// NKUnlockMemory( pipe, sizeof( OHCIPipe ) );
	delete pipe->allocPtr;
}

OHCITransfer*
OHCIAllocator::createTransfer()
{
	// Create it and align it to a 16-byte boundary;
	Ptr							ptr, alignedPtr;
	ptr = new(kernelProcess) Int8[sizeof(OHCITransfer)+15];
	if( !ptr )
		return nil;
	alignedPtr = (Ptr)ROUND_UP(16, (UInt32)ptr);
	
	OHCITransfer					*t;
	t = new(alignedPtr) OHCITransfer;
	t->allocPtr = ptr;
	
	// Clear hash list pointers and get physical address
	t->hashNext = t->hashPrev = nil;
	// NKLockMemory( t, sizeof( OHCITransfer ) );
	t->physAddr = NKGetPhysical( t, PROCESS_KERNEL );
	
	// Clear other variables
	t->transaction = nil;
	t->prev = t->next = nil;
	t->doneNext = nil;
	
	// Figure out hash index
	UInt32						hash;
	hash = getHash(t->physAddr);
	
	// Insert into hash table
	{
	CriticalSection					critical(tLock);
	
	t->hashNext = tTable[hash];
	if( t->hashNext )
		t->hashNext->hashPrev = t;
	tTable[hash] = t;
	}
	
	// Done!
	return t;
}

void
OHCIAllocator::disposeTransfer( OHCITransfer* t )
{
	// Figure out hash index
	UInt32						hash;
	hash = getHash(t->physAddr);
	
	// Remove it from hash table
	{
	CriticalSection					critical(tLock);
	
	if( t->hashPrev )
		t->hashPrev->hashNext = t->hashNext;
	else
		tTable[hash] = t->hashNext;
	if( t->hashNext )
		t->hashNext->hashPrev = t->hashPrev;
	}
	
	// Dispose it
	// NKUnlockMemory( t, sizeof( OHCITransfer ) );
	delete t->allocPtr;
}

OHCITransfer*
OHCIAllocator::searchTransfer( Ptr physAddr )
{
	// Figure out hash index
	UInt32						hash;
	hash = getHash(physAddr);
	
	// Look for it
	CriticalSection					critical(tLock);
	OHCITransfer					*t;
	
	t = tTable[hash];
	while( t )
	{
		if( t->physAddr == physAddr )
			break;
		t = t->hashNext;
	}
	
	return t;
}

UInt32
OHCIAllocator::getHash( void* addr )
{
	return (((UInt32)addr) >> 4) & (OHCI_HASH_TABLE_SIZE-1);
}