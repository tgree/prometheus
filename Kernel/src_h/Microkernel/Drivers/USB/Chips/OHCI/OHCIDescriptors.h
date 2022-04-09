/*
	OHCIDescriptors.h
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

#ifndef __OHCI_DESCRIPTORS__
#define __OHCI_DESCRIPTORS__

#include "NKAtomicOperations.h"

#define NUM_OHCI_HASH_BITS			6
#define OHCI_HASH_TABLE_SIZE		(1<<NUM_OHCI_HASH_BITS)

class OHCIPipe;
class OHCITransfer;

class OHCIAllocator
{
protected:
								OHCIAllocator();
	virtual						~OHCIAllocator();
	
	OHCIPipe*						createPipe();
	void							disposePipe( OHCIPipe* p );
	OHCITransfer*					createTransfer();
	void							disposeTransfer( OHCITransfer* t );
	
	OHCITransfer*					searchTransfer( Ptr physAddr );
	
private:
	OHCITransfer*					tTable[OHCI_HASH_TABLE_SIZE];
	
	SpinLock						tLock;
	
	UInt32						getHash( void* addr );
};

#endif /* __OHCI_DESCRIPTORS__ */