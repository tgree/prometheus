/*
	B*-Tree.cp
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
#include "NKDebuggerNub.h"
#include "B*-Tree.h"
#include "Macros.h"
#include "Kernel Console.h"
#include "Memory Utils.h"
#include "Time.h"

void* BTreeNode::getRecord(UInt16 n)
{
	Assert(n < nodeData.desc.ndNRecs);
	
	return (nodeData.data + nodeData.nodeOffsets[-n] - sizeof(NodeDescriptor));
}

UInt16 BTreeNode::numRecords()
{
	return nodeData.desc.ndNRecs;
}

CatDataRec* BTreeNode::getCatDataRec(UInt32 record)
{
	UInt8*	keyRec = (UInt8*)getRecord(record);
	if(keyRec)
	{
		UInt32	fileOffset = nodeData.nodeOffsets[-record] + keyRec[0] + 1;
		return (CatDataRec*)((Int32)&nodeData + ROUND_UP(2,fileOffset));
	}
	
	return nil;
}

BTree::BTree()
{
}

BTree::~BTree()
{
}

Boolean BTree::search(BTreeKey* key,UInt32* outNode,UInt32* outRecord,Boolean partialCompare,BTreeNode* outNodeData)
{
	return searchForNode(key,headerNode.headerRec.bthRoot,outNode,outRecord,partialCompare,outNodeData);
}

Boolean BTree::searchForNode(BTreeKey* key,UInt32 currNodeNum,UInt32* outNodeNum,UInt32* outRecord,Boolean partialCompare,BTreeNode* outNodeData)
{
	BTreeNode	currNode;
	
	if(!readNode(currNodeNum,&currNode))
		Panic("Failed to read node in searchForNode!\n");
	
	UInt16		numRecs = currNode.numRecords();
	UInt8*		lastRecord = nil;
	Int8			compare;
	UInt8*		record;
	
	for(Int32 i=0;i<numRecs;i++)
	{
		record = (UInt8*)currNode.getRecord(i);
		if(*record)	// Make sure we aren't testing a deleted record!
		{
			compare = compareKeys(key,(BTreeKey*)record,partialCompare);
			if(!compare)	// We have a match
			{
				if(currNode.nodeData.desc.ndType == ndLeafNode)
				{
					// This is a leaf node, so we should return the node number
					*outRecord = i;
					*outNodeNum = currNodeNum;
					if(outNodeData)
						MemCopy((Int8*)&currNode.nodeData,(Int8*)&outNodeData->nodeData,sizeof(GenericNodeData));
					return true;
				}
				else
					return searchForNode(key,*(UInt32*)(record + record[0] + 1),outNodeNum,outRecord,partialCompare,outNodeData);
			}
			else if(compare < 0 && currNode.nodeData.desc.ndType == ndIndxNode)	// Go back to the previous node and follow the link
			{
				if(lastRecord)
					return searchForNode(key,*(UInt32*)(lastRecord + lastRecord[0] + 1),outNodeNum,outRecord,partialCompare,outNodeData);
				else
					break;
			}
			lastRecord = record;
		}
	}
	
	if(currNode.nodeData.desc.ndType == ndIndxNode && compare > 0)
		return searchForNode(key,*(UInt32*)(record + record[0] + 1),outNodeNum,outRecord,partialCompare,outNodeData);
	
	return false;
}

BTreeCached::BTreeCached(UInt32 _numCachedNodes)
{
	numCachedNodes = _numCachedNodes;
	cachedNodes = new BTreeNode[numCachedNodes];
	cachedNodeNumber = new UInt32[numCachedNodes];
	cachedNodeTime = new Float64[numCachedNodes];
	
	for(Int32 i=0;i<numCachedNodes;i++)
		cachedNodeNumber[i] = 0;
}

BTreeCached::~BTreeCached()
{
	delete [] cachedNodeTime;
	delete [] cachedNodeNumber;
	delete [] cachedNodes;
}
	
Boolean BTreeCached::readCachedNode(UInt32 n,BTreeNode* outNode)
{
	Float64	thisNodeTime = GetTime_ns();
	for(UInt32 i=0;i<numCachedNodes;i++)
	{
		if(cachedNodeNumber[i] == 0)
			return false;
		else if(cachedNodeNumber[i] == n)
		{
			MemCopy((Int8*)&cachedNodes[i].nodeData,(Int8*)&outNode->nodeData,512);
			cachedNodeTime[i] = thisNodeTime;
			return true;
		}
	}
	
	return false;
}

void BTreeCached::cacheNode(UInt32 n,BTreeNode* cacheThisNode)
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
	
	MemCopy((Int8*)&cacheThisNode->nodeData,(Int8*)&cachedNodes[nextFreeCacheNode].nodeData,512);
	cachedNodeNumber[nextFreeCacheNode] = n;
}
