/*
	NKMemoryManager.cp
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
	Terry Greeniaus	-	Friday, 19 June 98	-	operator new() no longer zeroes blocks, but it can zap them
											for stress testing (see config.h)
	Terry Greeniaus	-	Friday, 19 June 98	-	Added MemSet() function, made MemZero() call MemSet()
	Terry Greeniaus	-	Sunday, 9 August 98	-	Completely re-wrote to use a table-of-contents based memory system
*/
#include "Macros.h"
#include "NKMemoryManager.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "Kernel Console.h"

typedef struct PhysPageHeader
{
	UInt32				lock[4];	// Reserve the first cache line for a lock
	struct PhysPageHeader*	next;	// The next unused physical page
}PhysPageHeader;

typedef struct TOCEntry
{
	void*		entryAddr;	// The logical address of this block
	UInt32		entryLen;		// The length of this block - not valid if this is an unused entry
	UInt32		entryPP;		// The page protection for this block
	UInt32		entryType;	// The type of block (free/allocated)
}TOCEntry;

enum
{
	// For TOCEntry::entryType
	freeEntryType		=	0,	// This entry is unused memory
	allocatedEntryType	=	1,	// This entry is used memory
	tocEntryType		=	2,	// This entry is a TOCPage
	unusedEntryType	=	3	// This is an unused TOC entry
};

typedef struct TOCPage
{
	// TOCPage takes up exactly one 4K page
	struct TOCPage*	nextPage;
	UInt32			rsrv1;
	UInt32			rsrv2;
	UInt32			rsrv3;
	TOCEntry			entry[127];
}TOCPage;

static NKSpinLock		physPageLock;
static PhysPageHeader	firstFreePhysicalPage;			// firstFreePhysicalPage.next points to the very first free physical page in memory

static UInt32			freeMem = 0;

static void*	NKDequeueFreePhysicalPage();
static void	NKEnqueueFreePhysicalPage(void* thePage);

void NKInitMemoryManager(void)
{
	physPageLock.init();
	
	// NKInitMemoryManager makes a linked list of all the unallocated physical RAM pages in the system.  Now, since these
	// pages are unallocated, we make the linked list directly out of the pages.  Each linked list element is places at
	// the very beginning of an unallocated physical pages.  Pages which are already allocated are not touched, meaning
	// that the linked list ends up being simply all of the unallocated pages.  The first page in the list can be found at
	// address "firstFreePhysicalPage.next".
	register UInt32		unaccountedMemory = machine.memSize;
	register UInt32		currPage;
	register UInt32		prevPage = (UInt32)&firstFreePhysicalPage;
	
	firstFreePhysicalPage.lock[0] = 0;
	firstFreePhysicalPage.next = nil;
	
	// Get as much memory from below the kernel as possible.  Right now, the kernel is loaded at machine.kernelStart (at 2M at the
	// time of writing) because in PDM machines, the video buffer is located at the start of memory.  Thus, if the screen doesn't
	// start at nil, we can take all the 2M below the kernel for free mem.  Actually, though, we leave the first three pages in case
	// interrupt vectors starting from nil are used.
	
	// Disable data relocation.
	register UInt32		oldMSR = DisableDR();
	
	// Account for memory between nil and kernel start
	UInt32			endPage = ROUND_DOWN(4096,(UInt32)machine.kernelStart);
	if( machine.videoParams.physicalAddr == nil )
	{
		// OOPS!  Figure out end of screen and start from there
		// Interesting.  We could be stealing video memory here, and using it as normal RAM.  Not
		// sure if that's a good idea or not - think when we get video drivers and someone changes
		// resolution...
		currPage = ROUND_UP(4096,machine.videoParams.rowBytes*machine.videoParams.height);
	}
	else
		// Start after the last interrupt vector position, even if we do use ROM interrupts (as if 12K were a big deal!)
		currPage = 48*0x100;	// This is page-aligned
	unaccountedMemory -= (UInt32)currPage;
	
	while( (currPage < (UInt32)machine.kernelStart) && unaccountedMemory )
	{
		((PhysPageHeader*)prevPage)->next = (PhysPageHeader*)currPage;	// Make the previous free page link to this one
		((PhysPageHeader*)currPage)->lock[0] = 0;						// Initialize the lock
		prevPage = currPage;
		currPage += 4096;
		unaccountedMemory -= 4096;
	}
	
	// Memory used by the kernel code/data is accounted for
	unaccountedMemory -= ROUND_UP(4096,(UInt32)machine.kernelEnd) - ROUND_DOWN(4096,(UInt32)machine.kernelStart);
	currPage = ROUND_UP(4096,(UInt32)machine.kernelEnd);
	
	// Account for memory between kernel code/data and page table
	while( (currPage < (UInt32)PTBase) && unaccountedMemory)
	{
		((PhysPageHeader*)prevPage)->next = (PhysPageHeader*)currPage;	// Make the previous free page link to this one
		((PhysPageHeader*)currPage)->lock[0] = 0;						// Initialize the lock
		prevPage = currPage;
		currPage += 4096;
		unaccountedMemory -= 4096;
	}
	
	// Memory used by the page table is accounted for
	currPage += PTLen;
	unaccountedMemory -= PTLen;
	
	// Now for everything else!
	while(unaccountedMemory)
	{
		((PhysPageHeader*)prevPage)->next = (PhysPageHeader*)currPage;	// Make the previous free page link to this one
		((PhysPageHeader*)currPage)->next = nil;						// Just in case this is the last page...
		((PhysPageHeader*)currPage)->lock[0] = 0;						// Initialize the lock
		prevPage = currPage;
		currPage += 4096;
		unaccountedMemory -= 4096;
	}
	
	register PhysPageHeader*	pageHeader = firstFreePhysicalPage.next;
	while(pageHeader)
	{
		freeMem += 4096;
		pageHeader = pageHeader->next;
	}
	
	// Fix data relocation
	SetMSR(oldMSR);
}

void* NKDequeueFreePhysicalPage()
{
	NKSpinLocker	locker(physPageLock);
	
	void*	retVal = (void*)-1;
	if(firstFreePhysicalPage.next)
	{
		retVal = firstFreePhysicalPage.next;
		register PhysPageHeader*	physPage = firstFreePhysicalPage.next;
		register UInt32				oldMSR = DisableDR();
		register PhysPageHeader*	nextPhysPage = physPage->next;
		SetMSR(oldMSR);
		firstFreePhysicalPage.next = nextPhysPage;
	}
	Assert((UInt32)retVal != -1);
	freeMem -= 4096;
	return retVal;
}

void NKEnqueueFreePhysicalPage(void* thePage)
{
	NKSpinLocker	locker(physPageLock);
	register PhysPageHeader*	newPhysPage = (PhysPageHeader*)thePage;
	
	if(firstFreePhysicalPage.next)
	{
		register PhysPageHeader*	currPhysPage = firstFreePhysicalPage.next;
		register PhysPageHeader*	prevPhysPage = nil;
		register UInt32				oldMSR = DisableDR();
		while(currPhysPage < newPhysPage)
		{
			prevPhysPage = currPhysPage;
			currPhysPage = currPhysPage->next;
			if(!currPhysPage)
				break;
		}
		if(prevPhysPage)
			prevPhysPage->next = newPhysPage;
		newPhysPage->next = currPhysPage;
		SetMSR(oldMSR);
		if(!prevPhysPage)
			firstFreePhysicalPage.next = newPhysPage;
	}
	else
	{
		firstFreePhysicalPage.next = newPhysPage;
		register UInt32		 oldMSR = DisableDR();
		newPhysPage->next = nil;
		SetMSR(oldMSR);
	}
	freeMem += 4096;
}

MemoryTOC::MemoryTOC(void* _tocBase,UInt32 _tocLen,UInt32 usedLen)
{
	tocBase = _tocBase;
	tocLen = _tocLen;
	processID = 0;	// Process::Process() will fill this in for us
	firstPage = nil;
	
	Assert(usedLen < _tocLen);
	nextUnmappedPage = (UInt32)_tocBase + usedLen;
	nextUnmappedPage = ROUND_UP(4096,nextUnmappedPage);
	mappedStart = (void*)nextUnmappedPage;
}

MemoryTOC::~MemoryTOC()
{
	for(UInt32 i=(UInt32)mappedStart;i<nextUnmappedPage;i+=4096)
		unMapPage((void*)i);
}

TOCPage* MemoryTOC::mapNewTOCPage()
{
	FatalAssert(processID != 0);
	nkVideo << " mapping new TOC page...";
	void*	physicalAddr = NKDequeueFreePhysicalPage();
	TOCPage*	thePage = (TOCPage*)nextUnmappedPage;
	nextUnmappedPage += 4096;
	
	NKMapPage((void*)thePage,physicalAddr,WIMG_COHERENT,PP_READ_WRITE,processID);
	
	if(!firstPage)
		firstPage = thePage;
	else
	{
		TOCPage*	currPage = firstPage;
		while(currPage->nextPage)
			currPage = currPage->nextPage;
		currPage->nextPage = thePage;
	}
	
	nkVideo << "Logical: " << (UInt32)thePage << ", Physical: " << (UInt32)physicalAddr << "\n";
	
	thePage->nextPage = nil;
	thePage->rsrv1 = thePage->rsrv2 = thePage->rsrv3 = 0;
	thePage->entry[0].entryAddr = (void*)thePage;
	thePage->entry[0].entryLen = 4096;
	thePage->entry[0].entryPP = PP_READ_WRITE;
	thePage->entry[0].entryType = tocEntryType;
	for(UInt32 i=1;i<127;i++)
		thePage->entry[i].entryType = unusedEntryType;
	
	return thePage;
}

void MemoryTOC::unMapPage(void* thePage)
{
	FatalAssert(processID != 0);
	void* physPage = NKUnmapPage(thePage,processID);
	NKEnqueueFreePhysicalPage(physPage);
}

TOCEntry* MemoryTOC::searchTOC(UInt32 entryType,UInt32 len)
{
	TOCPage*		currPage = firstPage;
	TOCEntry*	goodEntry = nil;
	
	while(currPage)
	{
		for(UInt32 i=0;i<127;i++)
		{
			if(currPage->entry[i].entryType == entryType)
			{
				if(entryType == unusedEntryType)
					return &currPage->entry[i];
				else if(currPage->entry[i].entryLen >= len)
				{
					if(goodEntry)
						goodEntry = (currPage->entry[i].entryLen < goodEntry->entryLen) ? &currPage->entry[i] : goodEntry;
					else
						goodEntry = &currPage->entry[i];
				}
			}
		}
		currPage = currPage->nextPage;
	}
	
	if(!goodEntry && entryType == unusedEntryType)
		return &mapNewTOCPage()->entry[1];
	
	return goodEntry;
}

TOCEntry* MemoryTOC::searchTOC(void* addr)
{
	TOCPage*		currPage = firstPage;
	while(currPage)
	{
		for(UInt32 i=0;i<127;i++)
		{
			if(currPage->entry[i].entryType != unusedEntryType && currPage->entry[i].entryAddr == addr)
				return &currPage->entry[i];
		}
		currPage = currPage->nextPage;
	}
	
	return nil;
}

void* MemoryTOC::allocateBlock(UInt32 len,UInt32 /*pp*/)
{
	if(!len)
		return nil;
	
	Assert(len != 0);
	len = ROUND_UP(1024,len);
	CriticalSection	critical(tocLock);
	
	// Search the TOC for a free block of the correct length
	void*		retVal = nil;
	TOCEntry*	freeEntry = searchTOC(freeEntryType,len);
	
	if(freeEntry)
	{
		// We found a free block that can hold this one
		if(freeEntry->entryLen > len)
		{
			TOCEntry*	unusedEntry = searchTOC(unusedEntryType,0);
			unusedEntry->entryAddr = (void*)((UInt32)freeEntry->entryAddr + len);
			unusedEntry->entryLen = freeEntry->entryLen - len;
			unusedEntry->entryPP = freeEntry->entryPP;
			unusedEntry->entryType = freeEntryType;
			freeEntry->entryLen = len;
		}
		freeEntry->entryType = allocatedEntryType;
		retVal = freeEntry->entryAddr;
	}
	else
	{
		FatalAssert(processID != 0);
		
		// We need to map some memory to get this block
		UInt32		numPages = ROUND_UP(4096,len)/4096;
		
		// Find an unused entry
		TOCEntry*	unusedEntry1 = searchTOC(unusedEntryType,0);
		unusedEntry1->entryType = allocatedEntryType;
		TOCEntry*	unusedEntry2 = ((4096*numPages > len) ? searchTOC(unusedEntryType,0) : nil);
		
		for(UInt32 i=0;i<numPages;i++)
		{
			void*	physPage = NKDequeueFreePhysicalPage();
			NKMapPage((void*)nextUnmappedPage,physPage,WIMG_COHERENT,PP_READ_WRITE,processID);
			nextUnmappedPage += 4096;
		}
		
		unusedEntry1->entryAddr = (void*)(nextUnmappedPage - 4096*numPages);
		unusedEntry1->entryLen = len;
		unusedEntry1->entryPP = PP_READ_WRITE;
		if(unusedEntry2)
		{
			UInt32	 freeLen = 4096*numPages - len;
			unusedEntry2->entryAddr = (void*)(nextUnmappedPage - freeLen);
			unusedEntry2->entryLen = freeLen;
			unusedEntry2->entryPP = PP_READ_WRITE;
			unusedEntry2->entryType = freeEntryType;
		}
		
		retVal = unusedEntry1->entryAddr;
	}

#if ZAP_NEW
	//nkVideo << "Zapping new block at logical: " << (UInt32)retVal << ", physical: " << (UInt32)NKGetPhysical(retVal,PROCESS_CURRENT) << ", len: " << len << "...";
	if(retVal)
	{
		UInt8*	theChar = (UInt8*)retVal;
		for(UInt32 i=0;i<len;i++)
			*theChar++ = ZAP_NEW_VALUE;
	}
	//nkVideo << "Done!\n";
#endif

	return retVal;
}

Boolean MemoryTOC::releaseBlock(void* blockAddr)
{
	CriticalSection		critical(tocLock);
	
	TOCEntry*	entry = searchTOC(blockAddr);
	if(!entry)
	{
		nkVideo << "released bad pointer: " << (UInt32)blockAddr << "\n";
		return false;
	}
#if ZAP_NEW
	//nkVideo << "Zapping released block logical: " << (UInt32)blockAddr << ", physical: " << (UInt32)NKGetPhysical(blockAddr,PROCESS_CURRENT) << "...";
	for(UInt32 i=0;i<entry->entryLen;i++)
		((UInt8*)entry->entryAddr)[i] = ZAP_NEW_VALUE;
	//nkVideo << "Done!\n";
#endif
	entry->entryType = freeEntryType;
	return true;
}

void* operator new(UInt32 len)
{
	return operator new(len,CurrProcess::process());
}

void* operator new(UInt32 len,Process* process)
{
	return process->toc.allocateBlock(len,PP_READ_WRITE);
}

void* operator new(UInt32,void* addr)
{
	return addr;
}

#if ARRAY_NEW_DELETE_SUPPORTED
void* operator new[](UInt32 len)
{
	return operator new(len,CurrProcess::process());
}

void* operator new[](UInt32 len,Process* process)
{
	return operator new(len,process);
}
#endif

void operator delete(void* p)
{
	if(p)
	{
		if((UInt32)p < 0x10000000)	// This is a kernel memory block
			kernelProcess->toc.releaseBlock(p);
		else
		{
			Process*	theProcess = CurrProcess::process();
			if((UInt32)p < 0x20000000)	// This is an OS memory block
			{
				if(theProcess->parent)	// We are currently running an OS sub-process
					theProcess->parent->toc.releaseBlock(p);
				else	// We are currently running the OS process
					theProcess->toc.releaseBlock(p);
			}
			else	// This is an OS sub-process memory block
				theProcess->toc.releaseBlock(p);
		}
	}
}

#if ARRAY_NEW_DELETE_SUPPORTED
void operator delete[](void* p)
{
	operator delete(p);
}
#endif

KernelObject::KernelObject()
{
	UInt32	KernelObjectAddr = reinterpret_cast<UInt32>(this);
	FatalAssert(KernelObjectAddr < 0x10000000);	// Big time-saver here...
}

KernelObject::~KernelObject()
{
}

void* KernelObject::operator new(UInt32 len)
{
	return ::operator new(len,kernelProcess);
}

void* KernelObject::operator new(UInt32,void* addr)
{
	FatalAssert((UInt32)addr < 0x10000000);
	return addr;
}

#if ARRAY_NEW_DELETE_SUPPORTED
void* KernelObject::operator new[](UInt32 len)
{
	return ::operator new(len,kernelProcess);
}
#endif

UInt32 GetFreeMem(void)
{
	return freeMem;
}

void MemStat(void)
{
	UInt32			usedMem,percentUsed1000;
	usedMem = machine.memSize-freeMem;
	percentUsed1000 = (usedMem*1000)/machine.memSize;
	cout << "\tFree Memory: " << freeMem/1024 << " K, " << (1000-percentUsed1000)/10 << "."
		<< (1000-percentUsed1000)%10 << "\n";
	cout << "\tUsed Memory: " << usedMem/1024 << " K, " << percentUsed1000/10 << "."
		<< percentUsed1000%10 << "\n";
}