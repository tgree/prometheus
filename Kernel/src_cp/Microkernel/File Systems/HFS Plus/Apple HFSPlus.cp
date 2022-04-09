/*
	Apple HFSPlus.cp
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
	Patrick Varilly		-	Monday, 29 March 99	-	Creation of file (based on Apple HFS.cp, by Terry)
*/
#include "NKDebuggerNub.h"
#include "Macros.h"
#include "Apple HFSPlus.h"
#include "Memory Utils.h"
#include "UnicodeUtils.h"
#include "WrapperPartition.h"
#include "ANSI.h"
#include "NKThreads.h"
#include "NKMachineInit.h"

static AppleHFSPlusFileSystemManager	appleHFSPlusFileSystemManager;

Boolean IsHFSPlusPartition( BlockDevicePartition* partition )
{
	BootBlockHeader			bootBlock;
	MasterDirectoryBlock		masterDirectoryBlock;
	IOCommand*	cmd1 = partition->readSectorsAsync((Int8*)&bootBlock,0,1);
	IOCommand*	cmd2 = partition->readSectorsAsync((Int8*)&masterDirectoryBlock,2,1);
	CurrThread::blockForIO(cmd2);
	delete cmd1;
	delete cmd2;
	
	VolumeHeader				*hfsPlusHeader = (VolumeHeader*)&bootBlock;
	
	if( hfsPlusHeader->signature == kHFSPlusSigWord )
		return true;									// Unwrapped volume (currently don't exist)
	
	// Might be an HFS+ volume inside an HFS wrapper
	if(bootBlock.bbID != bbID && bootBlock.bbID != 0)
		return false;
	if(masterDirectoryBlock.drSigWord != dirSigWord)
		return false;
	if( masterDirectoryBlock.drVCSize == kHFSPlusSigWord )		// drVCSize is really drEmbedSigWord
		return true;
	
	return false;
}

AppleHFSPlusFileSystemManager::AppleHFSPlusFileSystemManager()
{
}

FileSystem* AppleHFSPlusFileSystemManager::tryToBuildFileSystem(BlockDevicePartition* partition)
{
	if((partition->partitionType() == appleHFSPartitionType) && IsHFSPlusPartition( partition ) )
		return new AppleHFSPlusFileSystem(partition);
	
	return nil;
}

AppleHFSPlusFileSystem::AppleHFSPlusFileSystem(BlockDevicePartition* partition):
	FileSystem(partition)
{
	BlockDevicePartition*	oldPartition = partition;
	Int8					buffer[512];
	IOCommand*	cmd1 = partition->readSectorsAsync(buffer,0,1);
	IOCommand*	cmd2 = partition->readSectorsAsync((Int8*)&header,2,1);
	CurrThread::blockForIO(cmd2);
	delete cmd1;
	delete cmd2;
	
	// Check to see if it's unwrapped
	if( header.signature != kHFSPlusSigWord )	
	{
		// It's not, check HFS wrapper and get our real "partition"
		BootBlockHeader			*bootBlock = (BootBlockHeader*)buffer;
		MasterDirectoryBlock		*masterDirectoryBlock = (MasterDirectoryBlock*)&header;
		if(bootBlock->bbID != bbID && bootBlock->bbID != 0)
		{
			// If bootBlock->bbID == bbid, then this is a bootable system disk
			// If bootBlock->bbID == 0, this is not a bootable system disk
			// Any other value is illegal
			cout << "Corrupt boot block: id = " << (UInt32)bootBlock->bbID << "\n";
			debuggerNub->debugger();
		}
		if(masterDirectoryBlock->drSigWord != dirSigWord)
		{
			cout << "Corrupt MDB: id = " << (UInt32)masterDirectoryBlock->drSigWord << "\n";
			debuggerNub->debugger();
		}
		if(masterDirectoryBlock->drVCSize != kHFSPlusSigWord)
		{
			cout << "Detected as HFS Plus but MDB has incorrect embed signature\n";
			cout << "Shouldn't get here!!!";
			debuggerNub->debugger();
		}
		
		UInt16					startAllBlock, blockCount;
		startAllBlock = *(UInt16*)(&masterDirectoryBlock->drVBMCSize);
		blockCount = *(UInt16*)(&masterDirectoryBlock->drCtlCSize);
		if( (startAllBlock > masterDirectoryBlock->drNmAlBlks) ||
			((startAllBlock + blockCount) > masterDirectoryBlock->drNmAlBlks) )
		{
			cout << "HFS Plus \"wrapped\" volume exceeds wrapper space\n";
			for(;;)
				;
			debuggerNub->debugger();
		}
		
		UInt32					firstSector, numSectors;
		firstSector = masterDirectoryBlock->drAlBlSt + (masterDirectoryBlock->drAlBlkSiz/512)*startAllBlock;
		numSectors = (masterDirectoryBlock->drAlBlkSiz/512)*blockCount;
		
		// "Unwrap" HFS+ volume
		partition = _partition = new WrapperPartition( partition, firstSector, numSectors );
		partition->readSectors((Int8*)&header,2,1);
	}
	
	// Check volume consistency
	if( header.signature != kHFSPlusSigWord )
	{
		cout << "Invalid HFS+ signature!\n";
		debuggerNub->debugger();
	}
	if( (header.version != kHFSPlusVersion) && (header.version != 4) )
	{
		cout << "Unrecognized HFS+ version!\n";
		debuggerNub->debugger();
	}
	
	extentsBTree = new AppleHFSPlusExtentsBTree(this);
	catalogBTree = new AppleHFSPlusCatalogBTree(this);
	
	_root = new AppleHFSPlusDirectoryDescriptor(this,rootDirID);
	
	if(oldPartition->computeMDBChecksum(2) == machine.dirInfo.checkSum)
	{
		cout << greenMsg << "\tChecksum test passed\n";
		LargeBTreeNode	node;
		UInt32			myNode;
		UInt32			myRec;
		if(catalogBTree->findNode(machine.dirInfo.dirID,&myNode,&myRec,&node))
		{
			CatalogRecord*	dataRec = (CatalogRecord*)node.getCatalogRecord(myRec);
			if(dataRec->recordType == kCatalogFolderThreadType)
			{
				kernelDirectory = new AppleHFSPlusDirectoryDescriptor(this,machine.dirInfo.dirID);
				cout << redMsg << "\tBooter is on this file system in folder \"" << kernelDirectory->name() << "\"\n" << whiteMsg;
			}
		}
	}
}

UInt32 AppleHFSPlusFileSystem::getAllocationBlockSector(UInt32 allocBlock)
{
	return (header.blockSize/512)*allocBlock;
}

void AppleHFSPlusFileSystem::readAllocationBlock(void *p, UInt32 allocBlock)
{
	partition()->readSectors((Ptr)p,getAllocationBlockSector(allocBlock),(header.blockSize/512));
}
	
ConstASCII8Str AppleHFSPlusFileSystem::name()
{
	if( _root )
		return _root->name();
	else
		return "";
}

DirectoryDescriptor* AppleHFSPlusFileSystem::root()
{
	return new AppleHFSPlusDirectoryDescriptor(*_root);
}

AppleHFSPlusCatalogBTree::AppleHFSPlusCatalogBTree(AppleHFSPlusFileSystem* _fileSystem):
	LargeBTreeCached(NUM_HPLUS_CAT_CACHED_NODES)
{
	fileSystem = _fileSystem;
	
	// Read the header info first
	Ptr				AlBlockBuf = new Int8[fileSystem->header.blockSize];
	fileSystem->readAllocationBlock( AlBlockBuf, fileSystem->header.catalogFile.extents[0].startBlock );
	MemCopy( AlBlockBuf, &headerRec, sizeof(HeaderRec) );
	delete [] AlBlockBuf;
	
	// Initialize node cache
	initCachedNodes( headerRec.nodeSize );
	
	// Now read header node
	physReadNode(0, &headerNode);
}

Boolean AppleHFSPlusCatalogBTree::readNode(UInt32 n,LargeBTreeNode* outNode)
{
	Assert(n <= headerRec.totalNodes);
	
	if(n == 0)
	{
		outNode->setData( headerNode.nodeData, headerRec.nodeSize );
		return true;
	}
	else if(readCachedNode(n,outNode))
		return true;
	else
	{
		// Find the node on disk then
		if(physReadNode(n,outNode))
		{
			cacheNode(n,outNode);
			return true;
		}
	}
	
	return false;
}

Boolean AppleHFSPlusCatalogBTree::physReadNode(UInt32 n, LargeBTreeNode* outNode)
{
	// Find the node on disk then
	UInt32	nodeOffset = headerRec.nodeSize*n;
	UInt32	fileABNOffset = ROUND_DOWN(fileSystem->header.blockSize,nodeOffset);
	UInt32	numABNs = ROUND_UP(fileSystem->header.blockSize,nodeOffset - fileABNOffset + headerRec.nodeSize)/fileSystem->header.blockSize;
	UInt32	fileABN = fileABNOffset/fileSystem->header.blockSize;
	UInt32	diskABN;
	UInt32	diskNumABNs;
	Ptr		blockBuffer, nodeBuffer;
	UInt32	readSoFar = 0, startInBlock = nodeOffset - fileABNOffset;
	blockBuffer = new Int8[fileSystem->header.blockSize];
	nodeBuffer = new Int8[headerRec.nodeSize];
	
	while(numABNs > 0)
	{
		if(fileSystem->extentsBTree->getFileAllocationBlock(catalogFileID,xkrForkData,fileABN,&diskABN,&diskNumABNs))
		{
			while( (diskNumABNs > 0) && (numABNs > 0) )
			{
				fileSystem->readAllocationBlock( blockBuffer, diskABN );
				MemCopy( blockBuffer+startInBlock, nodeBuffer+readSoFar, MIN(fileSystem->header.blockSize - startInBlock,
															headerRec.nodeSize - readSoFar) );
				startInBlock = 0;
				readSoFar += MIN(fileSystem->header.blockSize - startInBlock, headerRec.nodeSize - readSoFar);
				diskABN++; fileABN++;
				diskNumABNs--; numABNs--;
				if( readSoFar >= headerRec.nodeSize )
					break;
			}
		}
		else
		{
			delete [] blockBuffer;
			delete [] nodeBuffer;
			return false;
		}
	}
	
	outNode->setData( nodeBuffer, headerRec.nodeSize );
	delete [] blockBuffer;
	delete [] nodeBuffer;
	return true;
}

Boolean AppleHFSPlusCatalogBTree::findNode(CatalogNodeID dirID,UInt32* outNode,UInt32 *outRec,LargeBTreeNode* outNodeData)
{
	LargeCatalogKey	rec = {6,dirID,{0,0}};
	return search((LargeBTreeKey*)&rec,outNode,outRec,false,outNodeData);
}

Int8 AppleHFSPlusCatalogBTree::compareKeys(LargeBTreeKey* _key1,LargeBTreeKey* _key2,Boolean partialCompare)
{
	LargeCatalogKey*	key1 = (LargeCatalogKey*)_key1;
	LargeCatalogKey*	key2 = (LargeCatalogKey*)_key2;
	
	if(!key1->keyLength)
		return -1;
	if(!key2->keyLength)
		return 1;
	
	// Compare the Parent ID fields first
	if(key1->parentID > key2->parentID)
		return 1;
	if(key1->parentID < key2->parentID)
		return -1;
	
	// Compare the names (the length byte isn't part of the comparison)
	Int32 key1Len = key1->nodeName.length;
	Int32 key2Len = key2->nodeName.length;
	UInt32 compareLen = (partialCompare ? MIN(key1Len,key2Len) : MAX(key1Len,key2Len));
	
	for(Int32 i=0;i<compareLen;i++,key1Len--,key2Len--)
	{
		if(key1Len < 0)
		{
			if(key2->nodeName.unicode[i])
				return -1;
		}
		else if(key2Len < 0)
		{
			if(key1->nodeName.unicode[i])
				return 1;
		}
		else
		{
			UniChar	letter1 = key1->nodeName.unicode[i];
			UniChar	letter2 = key2->nodeName.unicode[i];
			
			// Make both lower case
			letter1 = UnicodeLower( letter1 );
			letter2 = UnicodeLower( letter2 );
			
			if(letter1 > letter2)
				return 1;
			if(letter1 < letter2)
				return -1;
		}
	}
	
	return 0;
}

AppleHFSPlusExtentsBTree::AppleHFSPlusExtentsBTree(AppleHFSPlusFileSystem* _fileSystem)
{
	fileSystem = _fileSystem;
	fileSystem->extentsBTree = this;
	
	// Read the header info first
	Ptr				AlBlockBuf = new Int8[fileSystem->header.blockSize];
	FatalAssert(AlBlockBuf != nil);
	fileSystem->readAllocationBlock( AlBlockBuf, fileSystem->header.extentsFile.extents[0].startBlock );
	MemCopy( AlBlockBuf, &headerRec, sizeof(HeaderRec) );
	delete [] AlBlockBuf;
	
	// Now read header node
	physReadNode(0, &headerNode);
}

Boolean AppleHFSPlusExtentsBTree::readNode(UInt32 n,LargeBTreeNode* outNode)
{
	return physReadNode( n, outNode );
}

Boolean AppleHFSPlusExtentsBTree::physReadNode(UInt32 n, LargeBTreeNode* outNode)
{
	// Find the node on disk then
	UInt32	nodeOffset = headerRec.nodeSize*n;
	UInt32	fileABNOffset = ROUND_DOWN(fileSystem->header.blockSize,nodeOffset);
	UInt32	numABNs = ROUND_UP(fileSystem->header.blockSize,nodeOffset - fileABNOffset + headerRec.nodeSize)/fileSystem->header.blockSize;
	UInt32	fileABN = fileABNOffset/fileSystem->header.blockSize;
	UInt32	diskABN;
	UInt32	diskNumABNs;
	Ptr		blockBuffer, nodeBuffer;
	UInt32	readSoFar = 0, startInBlock = nodeOffset - fileABNOffset;
	blockBuffer = new Int8[fileSystem->header.blockSize];
	nodeBuffer = new Int8[headerRec.nodeSize];
	
	while(numABNs > 0)
	{
		if(fileSystem->extentsBTree->getFileAllocationBlock(extentsFileID,xkrForkData,fileABN,&diskABN,&diskNumABNs))
		{
			while( (diskNumABNs > 0) && (numABNs > 0) )
			{
				fileSystem->readAllocationBlock( blockBuffer, diskABN );
				MemCopy( blockBuffer+startInBlock, nodeBuffer+readSoFar, MIN(fileSystem->header.blockSize - startInBlock,
															headerRec.nodeSize - readSoFar) );
				startInBlock = 0;
				readSoFar += MIN(fileSystem->header.blockSize - startInBlock, headerRec.nodeSize - readSoFar);
				diskABN++; fileABN++;
				diskNumABNs--; numABNs--;
				if( readSoFar >= headerRec.nodeSize )
					break;
			}
		}
		else
		{
			delete [] blockBuffer;
			delete [] nodeBuffer;
			return false;
		}
	}
	
	outNode->setData( nodeBuffer, headerRec.nodeSize );
	delete [] blockBuffer;
	delete [] nodeBuffer;
	return true;
}

Int8 AppleHFSPlusExtentsBTree::compareKeys(LargeBTreeKey* _key1,LargeBTreeKey* _key2,Boolean partialCompare)
{
	LargeExtentKey*	key1 = (LargeExtentKey*)_key1;
	LargeExtentKey*	key2 = (LargeExtentKey*)_key2;
	
	Assert(key1->keyLength == 10);
	Assert(key2->keyLength == 10);
	
	// Compare the file ID fields next
	if(key1->fileID > key2->fileID)
		return 1;
	if(key1->fileID < key2->fileID)
		return -1;
	
	/*// Compare fork types
	if(key1->forkType > key2->forkType)
		return 1;
	if(key1->forkType < key2->forkType)
		return -1;*/
	
	// If this is a partial compare, the starting allocation block number doesn't matter
	if(partialCompare)
		return 0;
	
	// Compare the starting allocation block numbers last
	if(key1->startBlock > key2->startBlock)
		return 1;
	if(key1->startBlock < key2->startBlock)
		return -1;
	
	return 0;
}

Boolean AppleHFSPlusExtentsBTree::getFileAllocationBlock(CatalogNodeID fileID,Int8 fork,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs)
{
	if(fileID == catalogFileID)
	{
		if(searchExtentsRecord(&fileSystem->header.catalogFile.extents[0],0,fileABN,outABN,outNumABNs))
			return true;
	}
	else if(fileID == extentsFileID)
	{
		if(searchExtentsRecord(&fileSystem->header.extentsFile.extents[0],0,fileABN,outABN,outNumABNs))
			return true;
	}
	
	// OK, it's not in memory.  Search the extents tree for the first record of this file
	LargeExtentKey		rec = {10,xkrForkData,0,fileID,fileABN};
	LargeBTreeNode	node;
	UInt32			nodeNum;
	UInt32			recNum;
	if(search((LargeBTreeKey*)&rec,&nodeNum,&recNum,true,&node))
	{
		LargeExtentKey*		desc = (LargeExtentKey*)node.getRecord(recNum);
		LargeExtentDescriptor*	rec = (LargeExtentDescriptor*)((UInt32)desc + desc->keyLength + 2);
		while(desc->fileID == fileID)
		{
			if(desc->forkType == fork)
			{
				if(searchExtentsRecord(rec,desc->startBlock,fileABN,outABN,outNumABNs))
					return true;
			}
			if(++recNum == node.numRecords())
			{
				Assert(((BTNodeDescriptor*)node.nodeData)->fLink != 0);
				if(!readNode(((BTNodeDescriptor*)node.nodeData)->fLink,&node))
					break;
				recNum = 0;
			}
			desc = (LargeExtentKey*)node.getRecord(recNum);
			rec = (LargeExtentDescriptor*)((UInt32)desc + desc->keyLength + 2);
		}
	}
	
	Panic("Failed to find an extents node in the extents tree!\n");
	
	return false;
}

Boolean AppleHFSPlusExtentsBTree::searchExtentsRecord(LargeExtentDescriptor* rec,UInt32 recStartABN,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs)
{
	for(Int32 i=0;i<8;i++)
	{
		if(fileABN >= recStartABN && fileABN < recStartABN + rec[i].blockCount)
		{
			*outABN = rec[i].startBlock + fileABN - recStartABN;
			*outNumABNs = rec[i].blockCount + fileABN - recStartABN;
			return true;
		}
		recStartABN += rec[i].blockCount;
	}
	
	return false;
}

AppleHFSPlusDirectoryDescriptor::AppleHFSPlusDirectoryDescriptor(AppleHFSPlusFileSystem* fileSystem,LargeBTreeNode* node,UInt32 record)
{
	_fileSystem = fileSystem;
	
	LargeCatalogKey*		keyRec = (LargeCatalogKey*)node->getRecord(record);
	_parID = keyRec->parentID;
	
	CatalogRecord*	dataRec = node->getCatalogRecord(record);
	Assert(dataRec->recordType == kCatalogFolderType);
	
	MemCopy((Ptr)&keyRec->nodeName,(Ptr)&_name,(keyRec->nodeName.length+1)*2);
	UnicodeToASCII( keyRec->nodeName.unicode, keyRec->nodeName.length, _transName );
	_transName[keyRec->nodeName.length] = 0;
	_dirID = dataRec->largeFolder.folderID;
}

AppleHFSPlusDirectoryDescriptor::AppleHFSPlusDirectoryDescriptor(AppleHFSPlusFileSystem* fileSystem,CatalogNodeID dirID)
{
	_dirID = dirID;
	_fileSystem = fileSystem;
	
	LargeBTreeNode	node;
	UInt32			myNode;
	UInt32			myRec;
	if(fileSystem->catalogBTree->findNode(dirID,&myNode,&myRec,&node))
	{
		CatalogRecord*	dataRec = node.getCatalogRecord(myRec);
		Assert(dataRec->recordType == kCatalogFolderThreadType);
		
		MemCopy((Ptr)&dataRec->largeThread.nodeName,(Ptr)&_name,(dataRec->largeThread.nodeName.length+1)*2);
		UnicodeToASCII( dataRec->largeThread.nodeName.unicode, dataRec->largeThread.nodeName.length, _transName );
		_transName[dataRec->largeThread.nodeName.length] = 0;
		_parID = dataRec->largeThread.parentID;
	}
	else
		Panic("Couldn't find a node in AppleHFSPlusDirectoryDescriptor(AppleHFSPlusFileSystem*,Int32)!\n");
}

AppleHFSPlusDirectoryDescriptor::AppleHFSPlusDirectoryDescriptor(class AppleHFSPlusDirectoryDescriptor& parentDir,ConstASCII8Str dirName)
{
	_parID = parentDir._dirID;
	_fileSystem = parentDir._fileSystem;
	
	UInt32			strLen = strlen(dirName);
	LargeCatalogKey	searchRec = {6+strLen*2,_parID,{strLen,{0}}};
	Assert(strLen <= 255);
	ASCIIToUnicode( dirName, strLen, searchRec.nodeName.unicode );
	
	LargeBTreeNode	node;
	UInt32			myNode;
	UInt32			myRec;
	if(_fileSystem->catalogBTree->search((LargeBTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		LargeCatalogKey*		keyRec = (LargeCatalogKey*)node.getRecord(myRec);
		CatalogRecord*	dataRec = node.getCatalogRecord(myRec);
		Assert(dataRec->recordType == kCatalogFolderType);
		
		MemCopy((Ptr)&keyRec->nodeName,(Ptr)&_name,(keyRec->nodeName.length+1)*2);
		UnicodeToASCII( keyRec->nodeName.unicode, keyRec->nodeName.length, _transName );
		_transName[keyRec->nodeName.length] = 0;
		_dirID = dataRec->largeFolder.folderID;
	}
	else
		Panic("Couldn't find a subdirectory!\n");
}

AppleHFSPlusDirectoryDescriptor::~AppleHFSPlusDirectoryDescriptor()
{
}

ConstASCII8Str AppleHFSPlusDirectoryDescriptor::name()
{
	return _transName;
}

DirectoryDescriptor* AppleHFSPlusDirectoryDescriptor::parent()
{
	if(_parID != rootParentDirID)
		return new AppleHFSPlusDirectoryDescriptor(_fileSystem,_parID);
	
	return nil;
}

FileDescriptor* AppleHFSPlusDirectoryDescriptor::subFile(ConstASCII8Str name)
{
	LargeBTreeNode	node;
	UInt32			record;
	if(fileExists(name,&node,&record))
		return new AppleHFSPlusFileDescriptor(_fileSystem,&node,record);
	
	return nil;
}

DirectoryDescriptor* AppleHFSPlusDirectoryDescriptor::subDir(ConstASCII8Str name)
{
	LargeBTreeNode	node;
	UInt32			record;
	if(dirExists(name,&node,&record))
		return new AppleHFSPlusDirectoryDescriptor(_fileSystem,&node,record);
	
	return nil;
}

FileIterator* AppleHFSPlusDirectoryDescriptor::newFileIterator()
{
	return new AppleHFSPlusFileIterator(this);
}

DirectoryIterator* AppleHFSPlusDirectoryDescriptor::newDirectoryIterator()
{
	return new AppleHFSPlusDirectoryIterator(this);
}

Boolean AppleHFSPlusDirectoryDescriptor::operator==(class DirectoryDescriptor&)
{
	return false;
}

Boolean AppleHFSPlusDirectoryDescriptor::fileExists(ConstASCII8Str name)
{
	LargeBTreeNode	node;
	UInt32			record;
	return fileExists(name,&node,&record);
}

Boolean AppleHFSPlusDirectoryDescriptor::fileExists(ConstASCII8Str name,LargeBTreeNode* outNode,UInt32* outRecord)
{
	UInt32			strLen = strlen(name);
	LargeCatalogKey	searchRec = {6+strLen*2,_dirID,{strLen,{0}}};
	Assert(strLen <= 255);
	ASCIIToUnicode( name, strLen, searchRec.nodeName.unicode );
	
	UInt32		myNode;
	if(_fileSystem->catalogBTree->search((LargeBTreeKey*)&searchRec,&myNode,outRecord,false,outNode))
	{
		CatalogRecord*	dataRec = outNode->getCatalogRecord(*outRecord);
		return (dataRec->recordType == kCatalogFileType);
	}
	else
		return false;
}

Boolean AppleHFSPlusDirectoryDescriptor::dirExists(ConstASCII8Str name)
{
	LargeBTreeNode	node;
	UInt32			record;
	return dirExists(name,&node,&record);
}

Boolean AppleHFSPlusDirectoryDescriptor::dirExists(ConstASCII8Str name,LargeBTreeNode* outNode,UInt32* outRecord)
{
	UInt32			strLen = strlen(name);
	LargeCatalogKey	searchRec = {6+strLen*2,_dirID,{strLen,{0}}};
	Assert(strLen <= 255);
	ASCIIToUnicode( name, strLen, searchRec.nodeName.unicode );
	
	UInt32		myNode;
	if(_fileSystem->catalogBTree->search((LargeBTreeKey*)&searchRec,&myNode,outRecord,false,outNode))
	{
		CatalogRecord*	dataRec = outNode->getCatalogRecord(*outRecord);
		return (dataRec->recordType == kCatalogFolderType);
	}
	else
		return false;
}

AppleHFSPlusFileDescriptor::AppleHFSPlusFileDescriptor(AppleHFSPlusFileSystem* fileSystem,LargeBTreeNode* node,UInt32 record)
{
	_fileSystem = fileSystem;
	
	LargeCatalogKey*		keyRec = (LargeCatalogKey*)node->getRecord(record);
	_parID = keyRec->parentID;
	
	CatalogRecord*	dataRec = node->getCatalogRecord(record);
	Assert(dataRec->recordType == kCatalogFileType);
	
	MemCopy((Ptr)&keyRec->nodeName,(Ptr)&_name,(keyRec->nodeName.length+1)*2);
	UnicodeToASCII( keyRec->nodeName.unicode, keyRec->nodeName.length, _transName );
	_transName[keyRec->nodeName.length] = 0;
	_fileID = dataRec->largeFile.fileID;
}

AppleHFSPlusFileDescriptor::AppleHFSPlusFileDescriptor(AppleHFSPlusDirectoryDescriptor& parentDir,ConstASCII8Str fileName)
{
	_parID = parentDir._dirID;
	_fileSystem = parentDir._fileSystem;
	
	UInt32			strLen = strlen(fileName);
	LargeCatalogKey	searchRec = {6+strLen*2,_parID,{strLen,{0}}};
	Assert(strLen <= 255);
	ASCIIToUnicode( fileName, strLen, searchRec.nodeName.unicode );
	
	LargeBTreeNode	node;
	UInt32			myNode;
	UInt32			myRec;
	if(_fileSystem->catalogBTree->search((LargeBTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		LargeCatalogKey*		keyRec = (LargeCatalogKey*)node.getRecord(myRec);
		CatalogRecord*	dataRec = node.getCatalogRecord(myRec);
		Assert(dataRec->recordType == kCatalogFileType);
		
		MemCopy((Ptr)&keyRec->nodeName,(Ptr)&_name,(keyRec->nodeName.length+1)*2);
		UnicodeToASCII( keyRec->nodeName.unicode, keyRec->nodeName.length, _transName );
		_transName[keyRec->nodeName.length] = 0;
		_fileID = dataRec->largeFile.fileID;
	}
	else
		Panic("Couldn't find a subfile!\n");
}

ConstASCII8Str AppleHFSPlusFileDescriptor::name()
{
	return _transName;
}

DirectoryDescriptor* AppleHFSPlusFileDescriptor::parent()
{
	return new AppleHFSPlusDirectoryDescriptor(_fileSystem,_parID);
}

FileIStream* AppleHFSPlusFileDescriptor::openForRead()
{
	return new AppleHFSPlusFileIStream(this);
}

Boolean AppleHFSPlusFileDescriptor::operator==(class FileDescriptor&)
{
	return false;
}

AppleHFSPlusFileIterator::AppleHFSPlusFileIterator(AppleHFSPlusDirectoryDescriptor* dir):
	FileIterator(dir)
{
	LargeCatalogKey	rec = {6,dir->_dirID,{0,{0}}};
	key = rec;
	
	UInt32		myNode;
	if(dir->_fileSystem->catalogBTree->search((LargeBTreeKey*)&key,&myNode,&record,false,&node))
		advance();
	else
		Panic("Non-existant directory!\n");
}

AppleHFSPlusFileIterator::~AppleHFSPlusFileIterator()
{
}

void AppleHFSPlusFileIterator::advance()
{
	AppleHFSPlusDirectoryDescriptor*	parDir = static_cast<AppleHFSPlusDirectoryDescriptor*>(dir);
	AppleHFSPlusCatalogBTree*		tree = parDir->_fileSystem->catalogBTree;
	
	delete currFile;
	currFile = nil;
	
	CatalogRecord*			dataRec;
	
	do
	{
		if(++record == node.numRecords())
		{
			if(((BTNodeDescriptor*)node.nodeData)->fLink)
			{
				tree->readNode(((BTNodeDescriptor*)node.nodeData)->fLink,&node);
				record = 0;
			}
			else
				break;
		}
		
		LargeCatalogKey* key1 = (LargeCatalogKey*)node.getRecord(record);
		if(key.parentID == key1->parentID)
		{
			dataRec = node.getCatalogRecord(record);
			if(dataRec->recordType == kCatalogFileType)
				currFile = new AppleHFSPlusFileDescriptor(parDir->_fileSystem,&node,record);
		}
		else
			break;
	}while(dataRec->recordType != kCatalogFileType);
}

AppleHFSPlusDirectoryIterator::AppleHFSPlusDirectoryIterator(AppleHFSPlusDirectoryDescriptor* dir):
	DirectoryIterator(dir)
{
	LargeCatalogKey	rec = {6,dir->_dirID,{0,{0}}};
	key = rec;
	
	UInt32		myNode;
	if(dir->_fileSystem->catalogBTree->search((LargeBTreeKey*)&key,&myNode,&record,false,&node))
		advance();
	else
		Panic("Non-existant directory!\n");
}

AppleHFSPlusDirectoryIterator::~AppleHFSPlusDirectoryIterator()
{
}

void AppleHFSPlusDirectoryIterator::advance()
{
	AppleHFSPlusDirectoryDescriptor*	parDir = static_cast<AppleHFSPlusDirectoryDescriptor*>(dir);
	AppleHFSPlusCatalogBTree*		tree = parDir->_fileSystem->catalogBTree;
	
	delete currDir;
	currDir = nil;
	
	CatalogRecord*			dataRec;
	
	do
	{
		if(++record == node.numRecords())
		{
			if(((BTNodeDescriptor*)node.nodeData)->fLink)
			{
				tree->readNode(((BTNodeDescriptor*)node.nodeData)->fLink,&node);
				record = 0;
			}
			else
				break;
		}
		
		LargeCatalogKey* key1 = (LargeCatalogKey*)node.getRecord(record);
		if(key.parentID == key1->parentID)
		{
			dataRec = node.getCatalogRecord(record);
			if(dataRec->recordType == kCatalogFolderType)
				currDir = new AppleHFSPlusDirectoryDescriptor(parDir->_fileSystem,&node,record);
		}
		else
			break;
	}while(dataRec->recordType != kCatalogFolderType);
}

AppleHFSPlusFileIStream::AppleHFSPlusFileIStream(AppleHFSPlusFileDescriptor* file):
	Stream(file->name())
{
	fileDesc = file;
	fileSystem = file->_fileSystem;
	currPos = 0;
	
	UInt32			strLen = file->_name.length;
	LargeCatalogKey	searchRec = {6+strLen*2,file->_parID,{strLen,{0}}};
	Assert(strLen <= 255);
	MemCopy((Ptr)&file->_name,(Ptr)&searchRec.nodeName,(file->_name.length+1)*2);
	
	LargeBTreeNode	node;
	UInt32			myNode;
	UInt32			myRec;
	if(file->_fileSystem->catalogBTree->search((LargeBTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		LargeCatalogKey*		keyRec = (LargeCatalogKey*)node.getRecord(myRec);
		CatalogRecord*	dataRec = node.getCatalogRecord(myRec);
		Assert(dataRec->recordType == kCatalogFileType);
		
		MemCopy((Int8*)&dataRec->largeFile,(Int8*)&fileRec,sizeof(LargeCatalogFile));
		Assert(fileRec.fileID == file->_fileID);
	}
	else
		Panic("Couldn't find a file!\n");

	bufferFileSector = -1;
}

AppleHFSPlusFileIStream::~AppleHFSPlusFileIStream()
{
}

void AppleHFSPlusFileIStream::read(Ptr data,UInt32 len)
{
	if(!len)
		return;
	
	// Figure out how much we really can read
	UInt32 logLen = (currPos + len > eos() ? (eos() - currPos) : len);
	Assert(logLen > 0);
	if(!logLen)
		return;
	
	// Make sure we have the current sector in memory, if not reading from the start of a sector
	if(bufferFileSector != currPos/512 && ((currPos % 512) || logLen < 512))
	{
		readSectors(buffer,1);
		currPos -= 512;
		bufferFileSector = currPos/512;
	}
	
	// Bump currPos up to the next sector if we can (i.e. we have part of a sector in memory)
	if((currPos % 512) || logLen < 512)
	{
		UInt32 bufferedLen = 512 - (currPos % 512);
		bufferedLen = (bufferedLen > logLen ? logLen : bufferedLen);
		MemCopy(buffer + (currPos % 512),data,bufferedLen);
		logLen -= bufferedLen;
		currPos += bufferedLen;
		data += bufferedLen;
		if(!logLen)
			return;
	}
	
	// Now, we read sectors directly into memory
	readSectors(data,logLen/512);
	data += ROUND_DOWN(512,logLen);
	logLen = logLen % 512;
	
	// If there is any leftover, read it into buffer first and then copy it over
	if(logLen)
	{
		readSectors(buffer,1);
		currPos -= 512;
		MemCopy(buffer,data,logLen);
		currPos += logLen;
		bufferFileSector = currPos/512;
	}
}

void AppleHFSPlusFileIStream::setPos(UInt64 newPos)
{
	Assert(newPos < eos());
	currPos = (newPos >= eos() ? eos() - 1 : newPos);
}

UInt64 AppleHFSPlusFileIStream::getPos()
{
	return currPos;
}

UInt64 AppleHFSPlusFileIStream::eos()
{
	return fileRec.dataFork.logicalSize;
}

FileDescriptor* AppleHFSPlusFileIStream::file()
{
	return new AppleHFSPlusFileDescriptor(*fileDesc);
}

DirectoryDescriptor* AppleHFSPlusFileIStream::directory()
{
	return fileDesc->parent();
}

void AppleHFSPlusFileIStream::readSectors(Ptr dest,UInt32 numSectors)
{
	UInt32 maxSectorTransfer = fileSystem->partition()->maxSectorTransfer();
	while(numSectors)
	{
		UInt32	fileABN = currPos / fileSystem->header.blockSize;
		UInt32	diskABNSectorOffset = (currPos % fileSystem->header.blockSize) / 512;
		UInt32	outDiskABN;
		UInt32	outNumABNs;
		if(!AppleHFSPlusExtentsBTree::searchExtentsRecord(&fileRec.dataFork.extents[0],0,fileABN,&outDiskABN,&outNumABNs))
			if(!fileSystem->extentsBTree->getFileAllocationBlock(fileRec.fileID,xkrForkData,fileABN,&outDiskABN,&outNumABNs))
				break;
		
		UInt32	numContigSectors = outNumABNs*(fileSystem->header.blockSize/512) - diskABNSectorOffset;
		UInt32	numReadSectors = (numContigSectors >= numSectors ? numSectors : numContigSectors);
		if(numReadSectors > maxSectorTransfer)
			numReadSectors = maxSectorTransfer;
		fileSystem->partition()->readSectors(dest,fileSystem->getAllocationBlockSector(outDiskABN) + diskABNSectorOffset,numReadSectors);
		numSectors -= numReadSectors;
		currPos += numReadSectors*512;
		dest += numReadSectors*512;
	}
}