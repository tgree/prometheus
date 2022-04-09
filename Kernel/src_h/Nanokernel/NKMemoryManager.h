/*
	NKMemoryManager.h
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
	Terry Greeniaus	-	Friday, 19 June 98	-	Added MemSet function
	Terry Greeniaus	-	Sunday, 9 August 98	-	Completely re-wrote for a table-of-contents based memory system.
*/
#ifndef __NK_MEMORY_MANAGER__
#define __NK_MEMORY_MANAGER__

#include "NKAtomicOperations.h"

struct KernelObject
{
	void*	operator new(UInt32 len);
	void*	operator new(UInt32,void* addr);
#if ARRAY_NEW_DELETE_SUPPORTED
	void*	operator new[](UInt32 len);
#endif
	KernelObject();
	virtual ~KernelObject();
};

class		MemoryTOC
{
	SpinLock			tocLock;	// A lock on this toc - only one person can allocate at a time!
	struct TOCPage*	firstPage;	// The first page in this TOC
	void*			tocBase;	// The base address of the logical memory that this TOC manages
	UInt32			tocLen;	// The length of logical memory that this TOC manages
	void*			mappedStart;	// The start of memory that the TOC will map
	UInt32			nextUnmappedPage;	// The next logical page that will be mapped by this TOC
	UInt32			processID;			
	
	struct TOCPage*	mapNewTOCPage();
	void				unMapPage(void* thePage);
protected:
	void*			addBlock(void* blockAddr,UInt32 len,UInt32 pp);	// Adds an already created block to the TOC
	struct TOCEntry*	searchTOC(UInt32 entryType,UInt32 blockLen);
	struct TOCEntry*	searchTOC(void* addr);
	
	// Constructor for a MemoryTOC.  All addresses are logical.
	//	tocBase	-	the base address of memory that this TOC manages
	//	tocLen	-	the length of memory that this TOC manages
	//	usedLen	-	the amount of memory, from tocBase, that is already used in this TOC _BEFORE_ the MemoryTOC is constructed
	MemoryTOC(void* tocBase,UInt32 tocLen,UInt32 usedLen);	// Whoever constructs us MUST fill in the processID field themselves.
	~MemoryTOC();
public:
	void*			allocateBlock(UInt32 len,UInt32 pp);				// pp is ignored for now - returns the start of the block
	Boolean			releaseBlock(void* blockAddr);					// Releases the block - returns true if it was a valid block
	
	friend void*	operator new(UInt32 len);
	friend void*	operator new(UInt32 len,class Process* process);
	friend void*	operator new(UInt32 len,void* addr);
#if ARRAY_NEW_DELETE_SUPPORTED
	friend void*	operator new[](UInt32 len,class Process* process);
#endif
	friend void	operator delete(void*);
	
	friend class Process;
};

void		NKInitMemoryManager(void);
UInt32	GetFreeMem(void);
void		MemStat(void);

#endif /* __NK_MEMORY_MANAGER__ */