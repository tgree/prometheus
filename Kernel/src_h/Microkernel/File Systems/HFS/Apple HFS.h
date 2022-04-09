/*
	Apple HFS.h
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
#ifndef __APPLE_HFS__
#define __APPLE_HFS__

#include "Kernel Types.h"
#include "File Systems.h"
#include "B*-Tree.h"

#define NUM_CAT_CACHED_NODES	100

#pragma options align=mac68k

enum
{
	bbID	=	'LK'
};

typedef struct BootBlockHeader
{
	UInt16		bbID;				// Boot block's signature
	UInt32		bbEntry;			// Entry point to boot code
	UInt16		bbVersion;		// Boot block's version number
	UInt16		bbPageFlags;		// Used internally
	ASCII8		bbSysName[16];	// System filename
	ASCII8		bbShellName[16];	// Finder filename
	ASCII8		bbDbg1Name[16];	// Debugger filename
	ASCII8		bbDbg2Name[16];	// Debugger filename
	ASCII8		bbScreenName[16];	// Name of startup screen
	ASCII8		bbHelloName[16];	// Name of startup program
	ASCII8		bbScrapName[16];	// Name of system scrap file
	UInt16		bbCntFCBs;		// Number of FCBs to allocate
	UInt16		bbCntEvts;		// Number of event queue elements
	UInt32		bb128KSHeap;		// System heap size on 128K Mac
	UInt32		bb256KSHeap;		// Used internally
	UInt32		bbSysHeapSize;		// System heap size on all machines
	UInt16		filler;			// Reserved
	UInt32		bbSysHeapExtra;	// Additional system heap space
	UInt32		bbSysHeapFract;	// Fraction of RAM for system heap
	UInt8		pad[364];
}BootBlkHeader;

typedef struct ExtDescriptor
{
	UInt16		xdrStABN;	// First allocation block
	UInt16		xdrNumABlks;	// Number of allocation blocks
}ExtDescriptor;

typedef struct MasterDirectoryBlock
{
	UInt16		drSigWord;		// Volume signature
	UInt32		drCrDate;			// Date and time of volume creation
	UInt32		drLsMod;			// Date and time of last modification
	UInt16		drAtrb;			// Volume attributes
	UInt16		drNmFls;			// Number of files in root directory
	UInt16		drVBMSt;			// First block of volume bitmap
	UInt16		drAllocPtr;		// Start of next allocation search
	UInt16		drNmAlBlks;		// Number of allocation blocks in volume
	UInt32		drAlBlkSiz;		// Size (in bytes) of allocation blocks
	UInt32		drClpSiz;			// Default clump size
	UInt16		drAlBlSt;			// First allocation block in volume
	UInt32		drNxtCNID;		// Next unused catalog node ID
	UInt16		drFreeBks;		// Number of unused allocation blocks
	ASCII8		drVN[28];			// Volume name
	UInt32		drVolBkUp;		// Date and time of last backup
	UInt16		drVSeqNum;		// Volume backup sequence number
	UInt32		drWrCnt;			// Volume write count
	UInt32		drXTClpSiz;		// Clump size for extents overflow file
	UInt32		drCTClpSize;		// Clump size for catalog file
	UInt16		drNmRtDirs;		// Number of directories in root directory
	UInt32		drFilCnt;			// Number of files  in volume
	UInt32		drDirCnt;			// Number of directories in volume
	UInt32		drFndrInfo[8];		// Information used by the Finder
	UInt16		drVCSize;			// Size (in blocks) of volume cache
	UInt16		drVBMCSize;		// Size (in blocks) of volume bitmap cache
	UInt16		drCtlCSize;		// Size (in blocks) of common volume cache
	UInt32		drXTFlSize;		// Size of extents overflow file
	ExtDescriptor	drXTExtRec[3];		// Extent record for extents overflow file
	UInt32		drCTFlSize;		// Size of catalog file
	ExtDescriptor	drCTExtRec[3];		// Extent record for catalog file
	UInt8		pad[350];
}MasterDirectoryBlock;

enum
{
	dirSigWord	=	'BD'
};

typedef struct CatKeyRec
{
	UInt8	ckrKeyLen;
	Int8		ckrResrv1;
	UInt32	ckrParID;
	ASCII8	ckrCName[32];
	Int8		data[];
}CatKeyRec;

typedef struct DInfo
{
	MacOSRect			frRect;
	UInt16				frFlags;
	MacOSPoint			frLocation;
	Int16				frView;
}DInfo;

typedef struct DXInfo
{
	MacOSPoint			frScroll;
	Int32				frOpenChain;
	SInt8				frScript;
	SInt8				frXFlags;
	Int16				frComment;
	Int32				frPutAway;
}DXInfo;

typedef struct CatDirRec
{
	Int16		dirFlags;
	Int16		dirVal;
	UInt32		dirDirID;
	Int32		dirCrDat;
	Int32		dirMdDat;
	Int32		dirBkDat;
	DInfo			dirUsrInfo;
	DXInfo		dirFndrInfo;
	Int32		dirResrv[4];
}CatDirRec;

typedef struct FInfo
{
	OSType		fdType;
	OSType		fdCreator;
	UInt16		fdFlags;
	MacOSPoint	fdLocation;
	Int16		fdFldr;
}FInfo;

typedef struct FXInfo
{
	Int16		fdIconID;
	Int16		fdUnused[3];
	SInt8		fdScript;
	SInt8		fdXFlags;
	Int16		fdComment;
	Int32		fdPutAway;
}FXInfo;

typedef struct CatFileRec
{
	Int8			filFlags;
	Int8			filType;
	FInfo			filUsrWds;
	UInt32		filFlNum;
	UInt16		filStBlk;
	UInt32		filLgLen;
	UInt32		filPyLen;
	UInt16		filRStBlk;
	UInt32		filRLgLen;
	UInt32		filRPyLen;
	Int32		filCrDat;
	Int32		filMdDat;
	Int32		filBkDat;
	FXInfo		filFndrInfo;
	UInt16		filClpSize;
	ExtDescriptor	filExtRec[3];
	ExtDescriptor	filRExtRec[3];
	Int32		filResrv;
}CatFileRec;

typedef struct CatThdRec
{
	Int32		thdResrv[2];
	UInt32		thdParID;
	ASCII8		thdCName[32];
}CatThdRec;

typedef struct CatFThdRec
{
	Int32		fthdResrv[2];
	UInt32		fthdParID;
	ASCII8		fthdCName[32];
}CatFThdRec;

typedef struct CatDataRec
{
	Int8			cdrType;
	Int8			cdrResrv2;
	union
	{
		CatDirRec		cdrDirRec;
		CatFileRec		cdrFilRec;
		CatThdRec		cdrThdRec;
		CatFThdRec	cdrFThdRec;
	};
}CatDataRec;

enum
{
	cdrDirRec		=	1,
	cdrFilRec		=	2,
	cdrThdRec		=	3,
	cdrFThdRec	=	4
};

typedef struct ExtKeyRec
{
	UInt8	xkrKeyLen;
	Int8		xkrFkType;
	UInt32		xkrFNum;
	UInt16		xkrFABN;
	ExtDescriptor	xkrDescriptors[];
}ExtKeyRec;

enum
{
	xkrForkData		=	0,
	xkrForkResource	=	0xFF
};

enum
{
	rootParentDirID	=	1,
	rootDirID			=	2,
	extentsFileID		=	3,
	catalogFileID		=	4,
	badAllocationFileID	=	5
};

#pragma options align=reset

class AppleHFSDirectoryDescriptor	:	public DirectoryDescriptor
{
	UInt32					_dirID;
	UInt32					_parID;
	ASCII8					_name[32];
	class AppleHFSFileSystem*	_fileSystem;
	
	AppleHFSDirectoryDescriptor(AppleHFSFileSystem* fileSystem,BTreeNode* node,UInt32 record);
protected:
	AppleHFSDirectoryDescriptor(AppleHFSFileSystem* fileSystem,Int32 dirID);
public:
	AppleHFSDirectoryDescriptor(class AppleHFSDirectoryDescriptor& parentDir,ConstASCII8Str dirName);
	virtual ~AppleHFSDirectoryDescriptor();
	
	virtual	ConstASCII8Str		name();
	virtual	DirectoryDescriptor*	parent();
	virtual	FileDescriptor*			subFile(ConstASCII8Str name);
	virtual	DirectoryDescriptor*	subDir(ConstASCII8Str name);
	virtual	FileIterator*			newFileIterator();
	virtual	DirectoryIterator*		newDirectoryIterator();
	virtual	Boolean				operator==(class DirectoryDescriptor& dd);
	
	virtual	Boolean				fileExists(ConstASCII8Str name);
			Boolean				fileExists(ConstASCII8Str name,BTreeNode* outNode,UInt32* outRecord);
	virtual	Boolean				dirExists(ConstASCII8Str name);
			Boolean				dirExists(ConstASCII8Str name,BTreeNode* outNode,UInt32* outRecord);
			
	friend class AppleHFSFileDescriptor;
	friend class AppleHFSFileSystem;
	friend class AppleHFSFileIterator;
	friend class AppleHFSDirectoryIterator;
};

class AppleHFSFileDescriptor	:	public FileDescriptor
{
	UInt32					_parID;
	UInt32					_fileID;
	ASCII8					_name[32];
	class AppleHFSFileSystem*	_fileSystem;

	AppleHFSFileDescriptor(class AppleHFSFileSystem* fileSystem,BTreeNode* node,UInt32 record);
public:
	AppleHFSFileDescriptor(AppleHFSDirectoryDescriptor& parentDir,ConstASCII8Str fileName);
	
	virtual	ConstASCII8Str			name();
	virtual	DirectoryDescriptor*	parent();
	virtual	FileIStream*			openForRead();
	virtual	Boolean				operator==(class FileDescriptor& dd);
	
	friend class AppleHFSDirectoryDescriptor;
	friend class AppleHFSFileIterator;
	friend class AppleHFSFileIStream;
};

class AppleHFSFileSystem	:	public FileSystem
{
protected:
	UInt32					buffer1[4];
	BootBlockHeader			bootBlock;
	MasterDirectoryBlock		masterDirectoryBlock;
	UInt32					buffer2[4];
	ASCII8					_name[28];
	AppleHFSDirectoryDescriptor*	_root;
public:
	class AppleHFSCatalogBTree*	catalogBTree;
	class AppleHFSExtentsBTree*	extentsBTree;
	
	AppleHFSFileSystem(BlockDevicePartition* partition);
	
	virtual	ConstASCII8Str		name();
	virtual	DirectoryDescriptor*	root();
	virtual	UInt32				getAllocationBlockSector(UInt32 allocBlock);	// Gets the first sector in the given allocation block
	
	friend class AppleHFSCatalogBTree;
	friend class AppleHFSExtentsBTree;
	friend class AppleHFSDirectoryDescriptor;
	friend class AppleHFSFileIStream;
};

class AppleHFSFileSystemManager	:	public FileSystemManager
{
public:
	AppleHFSFileSystemManager();
	
protected:
	virtual	FileSystem*	tryToBuildFileSystem(BlockDevicePartition* partition);
};

class AppleHFSCatalogBTree	:	public BTreeCached
{
	AppleHFSFileSystem*	fileSystem;
protected:
	AppleHFSCatalogBTree(AppleHFSFileSystem* fileSystem);
	
	virtual	Boolean		readNode(UInt32 n,BTreeNode* outNode);
	virtual	Int8			compareKeys(BTreeKey* key1,BTreeKey* key2,Boolean partialCompare);	// A partial compare compares the smaller of the two names.
	
			Boolean		findNode(Int32 dirID,UInt32* outNode,UInt32* outRec,BTreeNode* outNodeData = nil);
	
	friend class AppleHFSFileSystem;
	friend class AppleHFSDirectoryDescriptor;
	friend class AppleHFSFileDescriptor;
	friend class AppleHFSFileIterator;
	friend class AppleHFSDirectoryIterator;
	friend class AppleHFSFileIStream;
};

class AppleHFSExtentsBTree	:	public BTree
{
	AppleHFSFileSystem*	fileSystem;
	
	static	Boolean		searchExtentsRecord(ExtDescriptor* rec,UInt32 recStartABN,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs);
protected:
	AppleHFSExtentsBTree(AppleHFSFileSystem* fileSystem);
	
	virtual	Boolean		readNode(UInt32 n,BTreeNode* outNode);
			IOCommand*	readNodeAsync(UInt32 n,BTreeNode* outNode);
	virtual	Int8			compareKeys(BTreeKey* key1,BTreeKey* key2,Boolean partialCompare);	// A partial compare doesn't compare the starting block number
public:
			Boolean		getFileAllocationBlock(UInt32 fileID,Int8 fork,UInt32 fileABN,UInt32* outDiskABN,UInt32* outNumABNs);
	
	friend class AppleHFSFileSystem;
	friend class AppleHFSFileIStream;
};

class AppleHFSFileIterator	:	public FileIterator
{
	BTreeNode	node;
	UInt32		record;
	CatKeyRec	key;
public:
	AppleHFSFileIterator(AppleHFSDirectoryDescriptor* dir);
	virtual ~AppleHFSFileIterator();
	
	virtual void				advance();
};

class AppleHFSDirectoryIterator	:	public DirectoryIterator
{
	BTreeNode	node;
	UInt32		record;
	CatKeyRec	key;
public:
	AppleHFSDirectoryIterator(AppleHFSDirectoryDescriptor* dir);
	virtual ~AppleHFSDirectoryIterator();
	
	virtual	void				advance();
};

class AppleHFSFileIStream	:	public FileIStream
{
	AppleHFSFileDescriptor*		fileDesc;
	AppleHFSFileSystem*		fileSystem;
	Int8						buffer[512];
	UInt32					bufferFileSector;
	UInt64					currPos;
	CatFileRec					fileRec;
	
	AppleHFSFileIStream(AppleHFSFileDescriptor* file);
			void					readSectors(Ptr dest,UInt32 numSectors);
public:
	virtual ~AppleHFSFileIStream();
	
	// For IStream
	virtual	void					read(Ptr data,UInt32 len);
	
	// For SeekableIStream
	virtual	void					setPos(UInt64 newPos);
	virtual	UInt64				getPos();
	virtual	UInt64				eos();
	
	// For FileIStream
	virtual	FileDescriptor*			file();
	virtual	DirectoryDescriptor*	directory();
	
	friend class AppleHFSFileDescriptor;
};

#endif /* __APPLE_HFS__ */