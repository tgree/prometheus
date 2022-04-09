/*
	B*-TreePlus.h
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
	Patrick Varilly		-	Monday, 29 March 99	-	Creation of file
*/
#ifndef __B_TREEPLUS__
#define __B_TREEPLUS__

#include "Kernel Types.h"

#pragma options align=mac68k

typedef struct BTNodeDescriptor
{
	UInt32			fLink;		// Forward link
	UInt32			bLink;		// Backward link
	Int8				type;		// Node type
	UInt8			height;		// Node level
	UInt16			numRecords;	// Number of records
	UInt16			reserved;		// Reserved
} BTNodeDescriptor;

enum
{
	// Node types
	kLeafNode			=	-1,		// Leaf node
	kIndexNode		=	0,		// Index node
	kHeaderNode		=	1,		// Header node
	kMapNode			=	2		// Map node
};

typedef struct HeaderRec
{
	BTNodeDescriptor	node;
	UInt16			treeDepth;
	UInt32			rootNode;
	UInt32			leafRecords;
	UInt32			firstLeafNode;
	UInt32			lastLeafNode;
	UInt16			nodeSize;
	UInt16			maxKeyLength;
	UInt32			totalNodes;
	UInt32			freeNodes;
	UInt16			reserved1;
	UInt32			clumpSize;	// mis-aligned
	UInt8			btreeType;
	UInt8			reserved2;
	UInt32			attributes;	// long aligned again
	UInt32			reserved3[16];
} HeaderRec, *HeaderPtr;

typedef enum
{
	kBTBadCloseMask	= 0x00000001,	// BTree not closed properly (unused in volume format)
	kBTBigKeysMask	= 0x00000002,	// Keys use 16-bit key length (always set of HFS+)
	kBTVariableIndexKeysMask = 0x00000004 // Keys are variable-sized (size is keyLength)
} BTreeAttributes;

typedef struct LargeBTreeKey
{
	UInt16		keyLength;
	UInt8		data[];
} LargeBTreeKey;

#pragma options align=reset

union CatalogRecord;

class LargeBTreeNode
{
public:
					LargeBTreeNode();
					LargeBTreeNode( Ptr nodeData, UInt32 nodeSize );
	virtual			~LargeBTreeNode();
	
	void				setData( Ptr nodeData, UInt32 nodeSize );
	void*			getRecord(UInt16 n);			// Returns a pointer to the record data (starts at key)
	union CatalogRecord* getCatalogRecord(UInt16 n);	// Returns a pointer to the catalog record data
	UInt16			numRecords();				// Returns the number of records
	
	Ptr				nodeData;
	UInt32			nodeSize;
};

typedef struct LargeBTreeRecord
{
	UInt16	keyLen;
	UInt8	keyBytes[];
} LargeBTreeRecord;

class LargeBTree
{
private:
			Boolean	searchForNode(LargeBTreeKey* key,UInt32 currNodeNum,UInt32* outNodeNum,UInt32* outRecord,Boolean partialCompare,LargeBTreeNode* outNodeData);

protected:
	HeaderRec			headerRec;
	
	LargeBTree();
	
public:
	virtual ~LargeBTree();
	
	virtual	Boolean	readNode(UInt32 n,LargeBTreeNode* outNode) = 0;	// Read the nth node, returns true if successful - always call the parent function here!  (It stores node 0)
			Boolean	search(LargeBTreeKey* key,UInt32* outNode,UInt32* outRecord,Boolean partialCompare,LargeBTreeNode* outNodeData = nil);
	virtual	Int8		compareKeys(LargeBTreeKey* key1,LargeBTreeKey* key2,Boolean partialCompare) = 0;
	
	friend class LargeBTreeIterator;
};

class LargeBTreeCached	:	public LargeBTree
{
	// This is a BTree which also caches nodes automatically
	UInt32			numCachedNodes;
	Ptr				cachedNodes;
	UInt32*			cachedNodeNumber;
	Float64*			cachedNodeTime;
	UInt32			nodeSize;
protected:
	LargeBTreeCached(UInt32 numCachedNodes);
	virtual ~LargeBTreeCached();
	
			void		initCachedNodes(UInt32 nodeSize);
			Boolean	readCachedNode(UInt32 n,LargeBTreeNode* outNode);	// Returns true if succesful, false otherwise
			void		cacheNode(UInt32 n,LargeBTreeNode* cacheThisNode);	// Call it even if already cached, updates time stamp on the node
};

#endif /* __B_TREEPLUS__ */