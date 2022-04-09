/*
	B*-TreePlus.cp
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
#include "NKDebuggerNub.h"
#include "B*-TreePlus.h"
#include "Apple HFSPlus.h"
#include "Macros.h"
#include "Memory Utils.h"
#include "Time.h"

LargeBTreeNode::LargeBTreeNode()
{
	this->nodeData = nil;
	this->nodeSize = 0;
}

LargeBTreeNode::LargeBTreeNode( Ptr nodeData, UInt32 nodeSize )
{
	this->nodeData = nil;
	this->nodeSize = 0;
	setData( nodeData, nodeSize );
}

LargeBTreeNode::~LargeBTreeNode()
{
	delete this->nodeData;
}

void LargeBTreeNode::setData( Ptr nodeData, UInt32 nodeSize )
{
	if( this->nodeData && (this->nodeSize != nodeSize) )
		delete this->nodeData;
	this->nodeSize = nodeSize;
	this->nodeData = (Ptr)new Int8[nodeSize];
	Assert( this->nodeData != nil );
	MemCopy( nodeData, this->nodeData, nodeSize );
}

void* LargeBTreeNode::getRecord(UInt16 n)
{
	if( nodeSize == 0 )
		return nil;
	
	BTNodeDescriptor		*desc = (BTNodeDescriptor*)nodeData;
	UInt16				*offsets = (UInt16*)(nodeData + nodeSize - 2);
	Assert(n < desc->numRecords);
	
	return (nodeData + offsets[-n]);
}

CatalogRecord* LargeBTreeNode::getCatalogRecord(UInt16 n)
{
	LargeCatalogKey		*keyRec = (LargeCatalogKey*)getRecord(n);
	return (CatalogRecord*)((((UInt32)keyRec + keyRec->keyLength + 2) + 1) & 0xFFFFFFFE);
}

UInt16 LargeBTreeNode::numRecords()
{
	if( nodeSize == 0 )
		return 0;
	
	BTNodeDescriptor		*desc = (BTNodeDescriptor*)nodeData;
	return desc->numRecords;
}

LargeBTree::LargeBTree()
{
}

LargeBTree::~LargeBTree()
{
}

Boolean LargeBTree::search(LargeBTreeKey* key,UInt32* outNode,UInt32* outRecord,Boolean partialCompare,LargeBTreeNode* outNodeData)
{
	return searchForNode(key,headerRec.rootNode,outNode,outRecord,partialCompare,outNodeData);
}

Boolean LargeBTree::searchForNode(LargeBTreeKey* key,UInt32 currNodeNum,UInt32* outNodeNum,UInt32* outRecord,Boolean partialCompare,LargeBTreeNode* outNodeData)
{
	LargeBTreeNode	currNode;
	
	if(!readNode(currNodeNum,&currNode))
		Panic("Failed to read node in searchForNode!\n");
	
	UInt16		numRecs = currNode.numRecords();
	UInt16*		lastRecord = nil;
	Int8			compare;
	UInt16*		record;
	
	for(Int32 i=0;i<numRecs;i++)
	{
		record = (UInt16*)currNode.getRecord(i);
		FatalAssert(record != nil);
		if(*record)	// Make sure we aren't testing a deleted record!
		{
			compare = compareKeys(key,(LargeBTreeKey*)record,partialCompare);
			if(!compare)	// We have a match
			{
				if(((BTNodeDescriptor*)currNode.nodeData)->type == kLeafNode)
				{
					// This is a leaf node, so we should return the node number
					*outRecord = i;
					*outNodeNum = currNodeNum;
					if(outNodeData)
						outNodeData->setData( currNode.nodeData, headerRec.nodeSize );
					return true;
				}
				else
					return searchForNode(key,*(UInt32*)((UInt32)record + record[0] + 2),outNodeNum,outRecord,partialCompare,outNodeData);
			}
			else if(compare < 0 && ((BTNodeDescriptor*)currNode.nodeData)->type == kIndexNode)	// Go back to the previous node and follow the link
			{
				if(lastRecord)
					return searchForNode(key,*(UInt32*)((UInt32)lastRecord + lastRecord[0] + 2),outNodeNum,outRecord,partialCompare,outNodeData);
				else
					break;
			}
			lastRecord = record;
		}
	}
	
	if(((BTNodeDescriptor*)currNode.nodeData)->type == kIndexNode && compare > 0)
		return searchForNode(key,*(UInt32*)((UInt32)record + record[0] + 2),outNodeNum,outRecord,partialCompare,outNodeData);
	
	return false;
}

LargeBTreeCached::LargeBTreeCached(UInt32 _numCachedNodes)
{
	numCachedNodes = _numCachedNodes;
	cachedNodes = nil;
	nodeSize = 0;
	cachedNodeNumber = new UInt32[numCachedNodes];
	cachedNodeTime = new Float64[numCachedNodes];
	
	for(Int32 i=0;i<numCachedNodes;i++)
		cachedNodeNumber[i] = 0;
}

LargeBTreeCached::~LargeBTreeCached()
{
	delete [] cachedNodeTime;
	delete [] cachedNodeNumber;
	delete [] cachedNodes;
}
	
void LargeBTreeCached::initCachedNodes(UInt32 nodeSize)
{
	this->nodeSize = nodeSize;
	cachedNodes = new Int8[nodeSize * numCachedNodes];
}

Boolean LargeBTreeCached::readCachedNode(UInt32 n,LargeBTreeNode* outNode)
{
	Float64	thisNodeTime = GetTime_ns();
	for(UInt32 i=0;i<numCachedNodes;i++)
	{
		if(cachedNodeNumber[i] == 0)
			return false;
		else if(cachedNodeNumber[i] == n)
		{
			outNode->setData( (Int8*)&cachedNodes[i*nodeSize], nodeSize );
			cachedNodeTime[i] = thisNodeTime;
			return true;
		}
	}
	
	return false;
}

void LargeBTreeCached::cacheNode(UInt32 n,LargeBTreeNode* cacheThisNode)
{
	Float64			thisNodeTime = GetTime_ns();
	Float64			replaceCacheNodeTime = thisNodeTime;
	UInt32			nextFreeCacheNode = 0;
	
	for(UInt32 i=0;i<numCachedNodes;i++)
	{
		if(cachedNodeNumber[i] == 0)
		{
			nextFreeCacheNode = i;
			break;
		}
		else if((thisNodeTime - cachedNodeTime[i]) > (thisNodeTime - replaceCacheNodeTime))
			nextFreeCacheNode = i;
	}
	
	MemCopy((Int8*)cacheThisNode->nodeData,(Int8*)&cachedNodes[nextFreeCacheNode*nodeSize],nodeSize);
	cachedNodeNumber[nextFreeCacheNode] = n;
}
