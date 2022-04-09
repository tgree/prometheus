/*
	B*-Tree.h
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
#ifndef __B_TREE__
#define __B_TREE__

#include "Kernel Types.h"

#pragma options align=mac68k

typedef struct NodeDescriptor
{
	Int32	ndFLink;		// Forward link
	Int32	ndBLink;		// Backward link
	Int8		ndType;		// Node type
	Int8		ndNHeight;	// Node level
	Int16	ndNRecs;		// Number of records in node
	Int16	ndResv2;		// Reserved
}NodeDescriptor;

enum
{
	// Node types
	ndIndxNode	=	0,	// Index node
	ndHdrNode		=	1,	// Header node
	ndMapNode	=	2,	// Map node
	ndLeafNode	=	-1	// Leaf node
};

typedef struct BTHdrRec
{
	Int16	bthDepth;		// Current depth of tree
	Int32	bthRoot;		// Number of root node
	Int32	bthNRecs;		// Number of leaf records in tree
	Int32	bthFNode;		// Number of first leaf node
	Int32	bthLNode;		// Number of last leaf node
	Int16	bthNodeSize;	// Size of a node
	Int16	bthKeyLen;	// Maximum length of a key
	Int32	bthNNodes;	// Total number of nodes in tree
	Int32	bthFree;		// Number of free nodes
	Int8		bthResv[76];
}BTHdrRec;

typedef struct HeaderNode
{
	NodeDescriptor		desc;		// Descriptor of header node
	
	// Records
	BTHdrRec			headerRec;	// First record
	Int8				rsrvRec[128];	// Second record, reserved
	UInt8			mapRec[256];	// Third record, B*-Tree bitmap
	
	// Record offsets
	Int16			freeSpaceOffset;	// Offset to free space
	Int16			headerRecOffset;	// Offset to first record
	Int16			rsrvRecOffset;		// Offset to second record
	Int16			mapRecOffset;		// Offset to third record
}HeaderNode;

typedef struct BTreeKey
{
	UInt8		keyLen;
	UInt8		data[];
}BTreeKey;

typedef struct GenericNodeData
{
	NodeDescriptor		desc;
	Int8				data[496];
	Int16			nodeOffsets[1];	// nodeOffsets[0] = Record 0, nodeOffsets[-1] = Record 1, etc.
}GenericNode;

#pragma options align=reset

struct BTreeNode
{
	GenericNode	nodeData;
	
	void*			getRecord(UInt16 n);			// Returns a pointer to the record data
	UInt16			numRecords();						// Returns the number of records
	struct CatDataRec*	getCatDataRec(UInt32 record);	// Returns a CatDataRec* for the record
};

typedef struct BTreeRecord
{
	UInt8	keyLen;
	UInt8	keyBytes[];
}BTreeRecord;

class BTree
{
			Boolean	searchForNode(BTreeKey* key,UInt32 currNodeNum,UInt32* outNodeNum,UInt32* outRecord,Boolean partialCompare,BTreeNode* outNodeData);
protected:
	HeaderNode	headerNode;
	
	BTree();
	
public:
	virtual ~BTree();
	
	virtual	Boolean	readNode(UInt32 n,BTreeNode* outNode) = 0;	// Read the nth node, returns true if successful - always call the parent function here!  (It stores node 0)
			Boolean	search(BTreeKey* key,UInt32* outNode,UInt32* outRecord,Boolean partialCompare,BTreeNode* outNodeData = nil);
	virtual	Int8		compareKeys(BTreeKey* key1,BTreeKey* key2,Boolean partialCompare) = 0;
	
	friend class BTreeIterator;
};

class BTreeCached	:	public BTree
{
	// This is a BTree which also caches nodes automatically
	UInt32			numCachedNodes;
	BTreeNode*		cachedNodes;
	UInt32*			cachedNodeNumber;
	Float64*			cachedNodeTime;
protected:
	BTreeCached(UInt32 numCachedNodes);
	virtual ~BTreeCached();
	
			Boolean	readCachedNode(UInt32 n,BTreeNode* outNode);	// Returns true if succesful, false otherwise
			void		cacheNode(UInt32 n,BTreeNode* cacheThisNode);	// Call it even if already cached, updates time stamp on the node
};

#endif /* __B_TREE__ */