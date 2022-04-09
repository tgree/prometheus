/*
	Apple HFS.cp
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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Patrick Varilly		-	Monday, 29 March 99	-	Added support for recognising HFS+ volumes
*/
#include "NKDebuggerNub.h"
#include "Assembly.h"
#include "NKVideo.h"
#include "Macros.h"
#include "Apple HFS.h"
#include "Apple HFSPlus.h"
#include "Kernel Console.h"
#include "Memory Utils.h"
#include "ANSI.h"
#include "Time.h"
#include "NKThreads.h"
#include "NKMachineInit.h"
#include "Time.h"

static AppleHFSFileSystemManager	appleHFSFileSystemManager;

AppleHFSFileSystemManager::AppleHFSFileSystemManager()
{
}

FileSystem* AppleHFSFileSystemManager::tryToBuildFileSystem(BlockDevicePartition* partition)
{
	if(partition->partitionType() == appleHFSPartitionType)
	{
		if( IsHFSPlusPartition( partition ) )
		{
			AppleHFSPlusFileSystem*		retVal;
			retVal = new AppleHFSPlusFileSystem(partition);
			return retVal;
		}
		else
		{
			AppleHFSFileSystem*	retVal;
			retVal = new AppleHFSFileSystem(partition);
			return retVal;
		}
	}
	
	return nil;
}

AppleHFSFileSystem::AppleHFSFileSystem(BlockDevicePartition* partition):
	FileSystem(partition)
{
	IOCommand*	cmd1 = partition->readSectorsAsync((Int8*)&bootBlock,0,1);
	IOCommand*	cmd2 = partition->readSectorsAsync((Int8*)&masterDirectoryBlock,2,1);
	CurrThread::blockForIO(cmd2);
	delete cmd1;
	delete cmd2;
	
	if(bootBlock.bbID != bbID && bootBlock.bbID != 0)
	{
		// If bootBlock.bbID == bbid, then this is a bootable system disk
		// If bootBlock.bbID == 0, this is not a bootable system disk
		// Any other value is illegal
		cout << "Corrupt boot block: id = " << (UInt32)bootBlock.bbID << "\n";
		debuggerNub->debugger();
	}
	if(masterDirectoryBlock.drSigWord != dirSigWord)
	{
		cout << "Corrupt MDB: id = " << (UInt32)masterDirectoryBlock.drSigWord << "\n";
		debuggerNub->debugger();
	}
	
	MemCopy(masterDirectoryBlock.drVN + 1,_name,masterDirectoryBlock.drVN[0]);
	_name[masterDirectoryBlock.drVN[0]] = '\0';
	
	catalogBTree = new AppleHFSCatalogBTree(this);
	extentsBTree = new AppleHFSExtentsBTree(this);
	
	_root = new AppleHFSDirectoryDescriptor(this,rootDirID);
	
	if(partition->computeMDBChecksum(2) == machine.dirInfo.checkSum)
	{
		cout << greenMsg << "\tChecksum test passed\n";
		BTreeNode	node;
		UInt32		myNode;
		UInt32		myRec;
		if(catalogBTree->findNode(machine.dirInfo.dirID,&myNode,&myRec,&node))
		{
			CatDataRec*	dataRec = node.getCatDataRec(myRec);
			if(dataRec->cdrType == cdrThdRec)
			{
				kernelDirectory = new AppleHFSDirectoryDescriptor(this,machine.dirInfo.dirID);
				cout << redMsg << "\tBooter is on this file system in folder \"" << kernelDirectory->name() << "\"\n" << whiteMsg;
			}
		}
	}
}

UInt32 AppleHFSFileSystem::getAllocationBlockSector(UInt32 allocBlock)
{
	return (masterDirectoryBlock.drAlBlSt + (masterDirectoryBlock.drAlBlkSiz/512)*allocBlock);
}
	
ConstASCII8Str AppleHFSFileSystem::name()
{
	return _name;
}

DirectoryDescriptor* AppleHFSFileSystem::root()
{
	return new AppleHFSDirectoryDescriptor(*_root);
}

AppleHFSCatalogBTree::AppleHFSCatalogBTree(AppleHFSFileSystem* _fileSystem):
	BTreeCached(NUM_CAT_CACHED_NODES)
{
	fileSystem = _fileSystem;
	
	fileSystem->partition()->readSectors((Int8*)&headerNode,fileSystem->getAllocationBlockSector(fileSystem->masterDirectoryBlock.drCTExtRec[0].xdrStABN),1);
}

Boolean AppleHFSCatalogBTree::readNode(UInt32 n,BTreeNode* outNode)
{
	Assert(n <= headerNode.headerRec.bthNNodes);
	
	if(n == 0)
	{
		MemCopy((Int8*)&headerNode,(Int8*)&outNode->nodeData,512);
		return true;
	}
	else if(readCachedNode(n,outNode))
		return true;
	else
	{
		// Find the node on disk then
		UInt32	nodeOffset = 512*n;
		UInt32	fileABNOffset = ROUND_DOWN(fileSystem->masterDirectoryBlock.drAlBlkSiz,nodeOffset);
		UInt32	extentOffsetSector = (nodeOffset - fileABNOffset)/512;
		UInt32	fileABN = fileABNOffset/fileSystem->masterDirectoryBlock.drAlBlkSiz;
		UInt32	diskABN;
		UInt32	diskNumABNs;
		if(fileSystem->extentsBTree->getFileAllocationBlock(catalogFileID,xkrForkData,fileABN,&diskABN,&diskNumABNs))
		{
			fileSystem->partition()->readSectors((Int8*)&outNode->nodeData,fileSystem->getAllocationBlockSector(diskABN) + extentOffsetSector,1);
			cacheNode(n,outNode);
			return true;
		}
	}
	
	return false;
}

Boolean AppleHFSCatalogBTree::findNode(Int32 dirID,UInt32* outNode,UInt32 *outRec,BTreeNode* outNodeData)
{
	CatKeyRec	rec = {7,0,dirID,{0,0}};
	return search((BTreeKey*)&rec,outNode,outRec,false,outNodeData);
}

Int8 AppleHFSCatalogBTree::compareKeys(BTreeKey* _key1,BTreeKey* _key2,Boolean partialCompare)
{
	CatKeyRec*	key1 = (CatKeyRec*)_key1;
	CatKeyRec*	key2 = (CatKeyRec*)_key2;
	
	if(!key1->ckrKeyLen)
		return -1;
	if(!key2->ckrKeyLen)
		return 1;
	
	// Compare the Parent ID fields first
	if(key1->ckrParID > key2->ckrParID)
		return 1;
	if(key1->ckrParID < key2->ckrParID)
		return -1;
	
	// Compare the names (the length byte isn't part of the comparison)
	Int32 key1Len = key1->ckrCName[0];
	Int32 key2Len = key2->ckrCName[0];
	UInt32 compareLen = (partialCompare ? MIN(key1Len,key2Len) : MAX(key1Len,key2Len));
	
	for(Int32 i=0;i<compareLen;i++,key1Len--,key2Len--)
	{
		if(key1Len < 0)
		{
			if(key2->ckrCName[i+1])
				return -1;
		}
		else if(key2Len < 0)
		{
			if(key1->ckrCName[i+1])
				return 1;
		}
		else
		{
			ASCII8	letter1 = key1->ckrCName[i+1];
			ASCII8	letter2 = key2->ckrCName[i+1];
			
			// Make it all capitals
			if(letter1 >= 'a' && letter1 <= 'z')
				letter1 -= 0x20;
			if(letter2 >= 'a' && letter2 <= 'z')
				letter2 -= 0x20;
			
			if(letter1 > letter2)
				return 1;
			if(letter1 < letter2)
				return -1;
		}
	}
	
	return 0;
}

AppleHFSExtentsBTree::AppleHFSExtentsBTree(AppleHFSFileSystem* _fileSystem)
{
	fileSystem = _fileSystem;
	
	fileSystem->partition()->readSectors((Int8*)&headerNode,fileSystem->getAllocationBlockSector(fileSystem->masterDirectoryBlock.drXTExtRec[0].xdrStABN),1);
}

Boolean AppleHFSExtentsBTree::readNode(UInt32 n,BTreeNode* outNode)
{
	IOCommand*	cmd = readNodeAsync(n,outNode);
	if(cmd)
	{
		CurrThread::blockForIO(cmd);
		delete cmd;
		return true;
	}
	
	return false;
}

IOCommand* AppleHFSExtentsBTree::readNodeAsync(UInt32 n,BTreeNode* outNode)
{
	Assert(n <= headerNode.headerRec.bthNNodes);
	
	if(n == 0)
	{
		MemCopy((Int8*)&headerNode,(Int8*)&outNode->nodeData,512);
		return new DummyIOCommand;
	}
	
	// Find the node on disk then
	UInt32	nodeOffset = 512*n;
	UInt32	fileABNOffset = ROUND_DOWN(fileSystem->masterDirectoryBlock.drAlBlkSiz,nodeOffset);
	UInt32	fileABN = fileABNOffset/fileSystem->masterDirectoryBlock.drAlBlkSiz;
	UInt32	extentOffsetSector = (nodeOffset - fileABNOffset)/512;
	UInt32	diskABN;
	UInt32	diskNumABNs;
	if(fileSystem->extentsBTree->getFileAllocationBlock(extentsFileID,xkrForkData,fileABN,&diskABN,&diskNumABNs))
		return fileSystem->partition()->readSectorsAsync((Int8*)&outNode->nodeData,fileSystem->getAllocationBlockSector(diskABN) + extentOffsetSector,1);
	
	return nil;
}

Int8 AppleHFSExtentsBTree::compareKeys(BTreeKey* _key1,BTreeKey* _key2,Boolean partialCompare)
{
	ExtKeyRec*	key1 = (ExtKeyRec*)_key1;
	ExtKeyRec*	key2 = (ExtKeyRec*)_key2;
	
	Assert(key1->xkrKeyLen == 7);
	Assert(key2->xkrKeyLen == 7);
	
	// Compare the file number fields next
	if(key1->xkrFNum > key2->xkrFNum)
		return 1;
	if(key1->xkrFNum < key2->xkrFNum)
		return -1;
	
	// If this is a partial compare, the starting allocation block number doesn't matter
	if(partialCompare)
		return 0;
	
	// Compare the starting allocation block numbers last
	if(key1->xkrFABN > key2->xkrFABN)
		return 1;
	if(key1->xkrFABN < key2->xkrFABN)
		return -1;
	
	return 0;
}

Boolean AppleHFSExtentsBTree::getFileAllocationBlock(UInt32 fileID,Int8 fork,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs)
{
	if(fileID == catalogFileID)
	{
		if(searchExtentsRecord(fileSystem->masterDirectoryBlock.drCTExtRec,0,fileABN,outABN,outNumABNs))
			return true;
	}
	else if(fileID == extentsFileID)
	{
		if(searchExtentsRecord(fileSystem->masterDirectoryBlock.drXTExtRec,0,fileABN,outABN,outNumABNs))
			return true;
	}
	
	// OK, it's not in memory.  Search the extents tree for the first record of this file
	ExtKeyRec	rec = {7,xkrForkData,fileID,fileABN};
	BTreeNode	node;
	UInt32		nodeNum;
	UInt32		recNum;
	if(search((BTreeKey*)&rec,&nodeNum,&recNum,true,&node))
	{
		ExtKeyRec*	desc = (ExtKeyRec*)node.getRecord(recNum);
		while(desc->xkrFNum == fileID)
		{
			if(desc->xkrFkType == fork)
			{
				if(searchExtentsRecord(desc->xkrDescriptors,desc->xkrFABN,fileABN,outABN,outNumABNs))
					return true;
			}
			if(++recNum == node.numRecords())
			{
				Assert(node.nodeData.desc.ndFLink != 0);
				if(!readNode(node.nodeData.desc.ndFLink,&node))
					break;
				recNum = 0;
			}
			desc = (ExtKeyRec*)node.getRecord(recNum);
		}
	}
	
	Panic("Failed to find an extents node in the extents tree!\n");
	
	return false;
}

Boolean AppleHFSExtentsBTree::searchExtentsRecord(ExtDescriptor* rec,UInt32 recStartABN,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs)
{
	for(Int32 i=0;i<3;i++)
	{
		if(fileABN >= recStartABN && fileABN < recStartABN + rec->xdrNumABlks)
		{
			*outABN = rec->xdrStABN + fileABN - recStartABN;
			*outNumABNs = rec->xdrNumABlks + fileABN - recStartABN;
			return true;
		}
		recStartABN += rec->xdrNumABlks;
		rec++;
	}
	
	return false;
}

AppleHFSDirectoryDescriptor::AppleHFSDirectoryDescriptor(AppleHFSFileSystem* fileSystem,BTreeNode* node,UInt32 record)
{
	_fileSystem = fileSystem;
	
	CatKeyRec*	keyRec = (CatKeyRec*)node->getRecord(record);
	_parID = keyRec->ckrParID;
	
	CatDataRec*	dataRec = (CatDataRec*)node->getCatDataRec(record);
	Assert(dataRec->cdrType == cdrDirRec);
	
	MemCopy(keyRec->ckrCName + 1,_name,keyRec->ckrCName[0]);
	_name[keyRec->ckrCName[0]] = '\0';
	_dirID = dataRec->cdrDirRec.dirDirID;
}

AppleHFSDirectoryDescriptor::AppleHFSDirectoryDescriptor(AppleHFSFileSystem* fileSystem,Int32 dirID)
{
	_dirID = dirID;
	_fileSystem = fileSystem;
	
	BTreeNode	node;
	UInt32		myNode;
	UInt32		myRec;
	if(fileSystem->catalogBTree->findNode(dirID,&myNode,&myRec,&node))
	{
		CatDataRec*	dataRec = node.getCatDataRec(myRec);
		Assert(dataRec->cdrType == cdrThdRec);
		
		MemCopy(dataRec->cdrThdRec.thdCName + 1,_name,dataRec->cdrThdRec.thdCName[0]);
		_name[dataRec->cdrThdRec.thdCName[0]] = '\0';
		_parID = dataRec->cdrThdRec.thdParID;
	}
	else
		Panic("Couldn't find a node in AppleHFSDirectoryDescriptor(AppleHFSFileSystem*,Int32)!\n");
}

AppleHFSDirectoryDescriptor::AppleHFSDirectoryDescriptor(class AppleHFSDirectoryDescriptor& parentDir,ConstASCII8Str dirName)
{
	_parID = parentDir._dirID;
	_fileSystem = parentDir._fileSystem;
	
	UInt32		strLen = strlen(dirName);
	CatKeyRec	searchRec = {6+strLen,0,_parID};
	Assert(strLen <= 31);
	MemCopy(dirName,searchRec.ckrCName + 1,(strLen > 31 ? 31 : strLen));
	searchRec.ckrCName[0] = strLen;
	
	BTreeNode	node;
	UInt32		myNode;
	UInt32		myRec;
	if(_fileSystem->catalogBTree->search((BTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		CatKeyRec*	keyRec = (CatKeyRec*)node.getRecord(myRec);
		CatDataRec*	dataRec = node.getCatDataRec(myRec);
		Assert(dataRec->cdrType == cdrDirRec);
		
		MemCopy(keyRec->ckrCName + 1,_name,keyRec->ckrCName[0]);
		_name[keyRec->ckrCName[0]] = '\0';
		_dirID = dataRec->cdrDirRec.dirDirID;
	}
	else
		Panic("Couldn't find a subdirectory!\n");
}

AppleHFSDirectoryDescriptor::~AppleHFSDirectoryDescriptor()
{
}

ConstASCII8Str AppleHFSDirectoryDescriptor::name()
{
	return _name;
}

DirectoryDescriptor* AppleHFSDirectoryDescriptor::parent()
{
	if(_parID != rootParentDirID)
		return new AppleHFSDirectoryDescriptor(_fileSystem,_parID);
	
	return nil;
}

FileDescriptor* AppleHFSDirectoryDescriptor::subFile(ConstASCII8Str name)
{
	BTreeNode	node;
	UInt32		record;
	if(fileExists(name,&node,&record))
		return new AppleHFSFileDescriptor(_fileSystem,&node,record);
	
	return nil;
}

DirectoryDescriptor* AppleHFSDirectoryDescriptor::subDir(ConstASCII8Str name)
{
	BTreeNode	node;
	UInt32		record;
	if(dirExists(name,&node,&record))
		return new AppleHFSDirectoryDescriptor(_fileSystem,&node,record);
	
	return nil;
}

FileIterator* AppleHFSDirectoryDescriptor::newFileIterator()
{
	return new AppleHFSFileIterator(this);
}

DirectoryIterator* AppleHFSDirectoryDescriptor::newDirectoryIterator()
{
	return new AppleHFSDirectoryIterator(this);
}

Boolean AppleHFSDirectoryDescriptor::operator==(class DirectoryDescriptor&)
{
	return false;
}

Boolean AppleHFSDirectoryDescriptor::fileExists(ConstASCII8Str name)
{
	BTreeNode	node;
	UInt32		record;
	return fileExists(name,&node,&record);
}

Boolean AppleHFSDirectoryDescriptor::fileExists(ConstASCII8Str name,BTreeNode* outNode,UInt32* outRecord)
{
	UInt32		strLen = strlen(name);
	CatKeyRec	searchRec = {6+strLen,0,_dirID};
	Assert(strLen <= 31);
	MemCopy(name,searchRec.ckrCName + 1,(strLen > 31 ? 31 : strLen));
	searchRec.ckrCName[0] = strLen;
	
	UInt32		myNode;
	if(_fileSystem->catalogBTree->search((BTreeKey*)&searchRec,&myNode,outRecord,false,outNode))
	{
		CatKeyRec*	keyRec = (CatKeyRec*)outNode->getRecord(*outRecord);
		CatDataRec*	dataRec = outNode->getCatDataRec(*outRecord);
		return (dataRec->cdrType == cdrFilRec);
	}
	else
		return false;
}

Boolean AppleHFSDirectoryDescriptor::dirExists(ConstASCII8Str name)
{
	BTreeNode	node;
	UInt32		record;
	return dirExists(name,&node,&record);
}

Boolean AppleHFSDirectoryDescriptor::dirExists(ConstASCII8Str name,BTreeNode* outNode,UInt32* outRecord)
{
	UInt32		strLen = strlen(name);
	CatKeyRec	searchRec = {6+strLen,0,_dirID};
	Assert(strLen <= 31);
	MemCopy(name,searchRec.ckrCName + 1,(strLen > 31 ? 31 : strLen));
	searchRec.ckrCName[0] = strLen;
	
	UInt32		myNode;
	if(_fileSystem->catalogBTree->search((BTreeKey*)&searchRec,&myNode,outRecord,false,outNode))
	{
		CatKeyRec*	keyRec = (CatKeyRec*)outNode->getRecord(*outRecord);
		CatDataRec*	dataRec = outNode->getCatDataRec(*outRecord);
		return (dataRec->cdrType == cdrDirRec);
	}
	else
		return false;
}

AppleHFSFileDescriptor::AppleHFSFileDescriptor(AppleHFSFileSystem* fileSystem,BTreeNode* node,UInt32 record)
{
	_fileSystem = fileSystem;
	
	CatKeyRec*	keyRec = (CatKeyRec*)node->getRecord(record);
	_parID = keyRec->ckrParID;
	
	CatDataRec*	dataRec = (CatDataRec*)node->getCatDataRec(record);
	Assert(dataRec->cdrType == cdrFilRec);
	
	MemCopy(keyRec->ckrCName + 1,_name,keyRec->ckrCName[0]);
	_name[keyRec->ckrCName[0]] = '\0';
	_fileID = dataRec->cdrFilRec.filFlNum;
}

AppleHFSFileDescriptor::AppleHFSFileDescriptor(AppleHFSDirectoryDescriptor& parentDir,ConstASCII8Str fileName)
{
	_parID = parentDir._dirID;
	_fileSystem = parentDir._fileSystem;
	
	UInt32		strLen = strlen(fileName);
	CatKeyRec	searchRec = {6+strLen,0,_parID};
	Assert(strLen <= 31);
	
	MemCopy(fileName,searchRec.ckrCName + 1,(strLen > 31 ? 31 : strLen));
	searchRec.ckrCName[0] = strLen;
	
	BTreeNode	node;
	UInt32		myNode;
	UInt32		myRec;
	if(_fileSystem->catalogBTree->search((BTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		CatKeyRec*	keyRec = (CatKeyRec*)node.getRecord(myRec);
		CatDataRec*	dataRec = node.getCatDataRec(myRec);
		Assert(dataRec->cdrType == cdrFilRec);
		
		MemCopy(keyRec->ckrCName + 1,_name,keyRec->ckrCName[0]);
		_name[keyRec->ckrCName[0]] = '\0';
		_fileID = dataRec->cdrFilRec.filFlNum;
	}
	else
		Panic("Couldn't find a file!\n");
}

ConstASCII8Str AppleHFSFileDescriptor::name()
{
	return _name;
}

DirectoryDescriptor* AppleHFSFileDescriptor::parent()
{
	return new AppleHFSDirectoryDescriptor(_fileSystem,_parID);
}

FileIStream* AppleHFSFileDescriptor::openForRead()
{
	return new AppleHFSFileIStream(this);
}

Boolean AppleHFSFileDescriptor::operator==(class FileDescriptor&)
{
	return false;
}

AppleHFSFileIterator::AppleHFSFileIterator(AppleHFSDirectoryDescriptor* dir):
	FileIterator(dir)
{
	CatKeyRec	rec = {5,0,dir->_dirID};
	key = rec;
	
	UInt32		myNode;
	if(dir->_fileSystem->catalogBTree->search((BTreeKey*)&key,&myNode,&record,false,&node))
		advance();
	else
		Panic("Non-existant directory!\n");
}

AppleHFSFileIterator::~AppleHFSFileIterator()
{
}

void AppleHFSFileIterator::advance()
{
	AppleHFSDirectoryDescriptor*	parDir = static_cast<AppleHFSDirectoryDescriptor*>(dir);
	AppleHFSCatalogBTree*		tree = parDir->_fileSystem->catalogBTree;
	delete currFile;
	currFile = nil;
	CatDataRec*	dataRec;
	
	do
	{
		if(++record == node.numRecords())
		{
			if(node.nodeData.desc.ndFLink)
			{
				tree->readNode(node.nodeData.desc.ndFLink,&node);
				record = 0;
			}
			else
				break;
		}
		
		CatKeyRec* key1 = (CatKeyRec*)node.getRecord(record);
		if(key.ckrParID == key1->ckrParID)
		{
			dataRec = node.getCatDataRec(record);
			if(dataRec->cdrType == cdrFilRec)
				currFile = new AppleHFSFileDescriptor(parDir->_fileSystem,&node,record);
		}
		else
			break;
	}while(dataRec->cdrType != cdrFilRec);
}

AppleHFSDirectoryIterator::AppleHFSDirectoryIterator(AppleHFSDirectoryDescriptor* dir):
	DirectoryIterator(dir)
{
	CatKeyRec	rec = {5,0,dir->_dirID};
	key = rec;
	
	UInt32		myNode;
	if(dir->_fileSystem->catalogBTree->search((BTreeKey*)&key,&myNode,&record,false,&node))
		advance();
	else
		Panic("Non-existant directory!\n");
}

AppleHFSDirectoryIterator::~AppleHFSDirectoryIterator()
{
}

void AppleHFSDirectoryIterator::advance()
{
	AppleHFSDirectoryDescriptor*	parDir = static_cast<AppleHFSDirectoryDescriptor*>(dir);
	AppleHFSCatalogBTree*		tree = parDir->_fileSystem->catalogBTree;
	delete currDir;
	currDir = nil;
	CatDataRec*	dataRec;
	
	do
	{
		if(++record == node.numRecords())
		{
			if(node.nodeData.desc.ndFLink)
			{
				tree->readNode(node.nodeData.desc.ndFLink,&node);
				record = 0;
			}
			else
				break;
		}
		
		CatKeyRec* key1 = (CatKeyRec*)node.getRecord(record);
		if(key.ckrParID == key1->ckrParID)
		{
			dataRec = node.getCatDataRec(record);
			if(dataRec->cdrType == cdrDirRec)
				currDir = new AppleHFSDirectoryDescriptor(parDir->_fileSystem,&node,record);
		}
		else
			break;
	}while(dataRec->cdrType != cdrDirRec);
}

AppleHFSFileIStream::AppleHFSFileIStream(AppleHFSFileDescriptor* file):
	Stream(file->name())
{
	fileDesc = file;
	fileSystem = file->_fileSystem;
	currPos = 0;
	
	UInt32		strLen = strlen(file->_name);
	CatKeyRec	searchRec = {6+strLen,0,file->_parID};
	MemCopy(file->_name,searchRec.ckrCName + 1,strLen);
	searchRec.ckrCName[0] = strLen;
	
	BTreeNode	node;
	UInt32		myNode;
	UInt32		myRec;
	if(fileSystem->catalogBTree->search((BTreeKey*)&searchRec,&myNode,&myRec,false,&node))
	{
		CatKeyRec*	keyRec = (CatKeyRec*)node.getRecord(myRec);
		CatDataRec*	dataRec = node.getCatDataRec(myRec);
		Assert(dataRec->cdrType == cdrFilRec);
		
		MemCopy((Int8*)&dataRec->cdrFilRec,(Int8*)&fileRec,sizeof(CatFileRec));
		Assert(fileRec.filFlNum == file->_fileID);
	}
	else
		Panic("Couldn't find a file!\n");
	
	bufferFileSector = -1;
}

AppleHFSFileIStream::~AppleHFSFileIStream()
{
}

void AppleHFSFileIStream::read(Ptr data,UInt32 len)
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

void AppleHFSFileIStream::setPos(UInt64 newPos)
{
	Assert(newPos < eos());
	currPos = (newPos >= eos() ? eos() - 1 : newPos);
}

UInt64 AppleHFSFileIStream::getPos()
{
	return currPos;
}

UInt64 AppleHFSFileIStream::eos()
{
	return fileRec.filLgLen;
}

FileDescriptor* AppleHFSFileIStream::file()
{
	return new AppleHFSFileDescriptor(*fileDesc);
}

DirectoryDescriptor* AppleHFSFileIStream::directory()
{
	return fileDesc->parent();
}

void AppleHFSFileIStream::readSectors(Ptr dest,UInt32 numSectors)
{
	UInt32 maxSectorTransfer = fileSystem->partition()->maxSectorTransfer();
	while(numSectors)
	{
		UInt32	fileABN = currPos / fileSystem->masterDirectoryBlock.drAlBlkSiz;
		UInt32	diskABNSectorOffset = (currPos % fileSystem->masterDirectoryBlock.drAlBlkSiz) / 512;
		UInt32	outDiskABN;
		UInt32	outNumABNs;
		if(!AppleHFSExtentsBTree::searchExtentsRecord(fileRec.filExtRec,0,fileABN,&outDiskABN,&outNumABNs))
			if(!fileSystem->extentsBTree->getFileAllocationBlock(fileRec.filFlNum,xkrForkData,fileABN,&outDiskABN,&outNumABNs))
				break;
		
		UInt32	numContigSectors = outNumABNs*(fileSystem->masterDirectoryBlock.drAlBlkSiz/512) - diskABNSectorOffset;
		UInt32	numReadSectors = (numContigSectors >= numSectors ? numSectors : numContigSectors);
		if(numReadSectors > maxSectorTransfer)
			numReadSectors = maxSectorTransfer;
		fileSystem->partition()->readSectors(dest,fileSystem->getAllocationBlockSector(outDiskABN) + diskABNSectorOffset,numReadSectors);
		numSectors -= numReadSectors;
		currPos += numReadSectors*512;
		dest += numReadSectors*512;
	}
}
