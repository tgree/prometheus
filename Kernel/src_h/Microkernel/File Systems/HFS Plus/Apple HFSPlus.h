/*
	Apple HFSPlus.h
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
#ifndef __APPLE_HFS_PLUS__
#define __APPLE_HFS_PLUS__

#include "Kernel Types.h"
#include "File Systems.h"
#include "Apple HFS.h"
#include "B*-TreePlus.h"

#define NUM_HPLUS_CAT_CACHED_NODES	15

#pragma options align=mac68k			// I'm not sure this is needed

enum
{
	kHFSPlusSigWord		= 0x482B,	// 'H+' in ASCII
	kHFSPlusVersion		= 0x0003,	// Actually, MacOS 8.5 produces v4 disks, but I think the changes are
									// in the attributes file, which we don't use (yet, at least)
	kHFSPlusMountVersion	= '8.01'		// Uniquely identifies a particular implementation
};

typedef struct UniStr255
{
	UInt16				length;		// Number of unicode characters
	UniChar				unicode[255];	// Unicode characters
} UniStr255;
typedef const UniStr255 *ConstUniStr255Param;

typedef UInt32 CatalogNodeID;

typedef struct LargeCatalogKey
{
	UInt16				keyLength;	// Key length (excluding the length field)
	CatalogNodeID			parentID;		// Parent folder ID
	UniStr255				nodeName;	// Catalog node name
} LargeCatalogKey;

// Large extent descriptor
typedef struct LargeExtentDescriptor
{
	UInt32				startBlock;	// First allocation block
	UInt32				blockCount;	// Number of allocation blocks
} LargeExtentDescriptor;

// Large extent record
typedef LargeExtentDescriptor LargeExtentRecord[8];

// Permission info - 16 bytes
typedef struct Permissions
{
	UInt32				ownerID;		// User or group ID of file/folder owner
	UInt32				groupID;		// Additional user of group ID
	UInt32				permissions;	// Permissions (bytes: unused, owner, group, everyone)
	UInt32				specialDevice;	// UNIX: device for character or block special file
} Permission;

// Fork data info - 80 bytes
typedef struct ForkData
{
	UInt64				logicalSize;	// Fork's logical size in bytes
	UInt32				clumpSize;	// Fork's clump size in bytes
	UInt32				totalBlocks;	// Total allocation blocks used by this fork
	LargeExtentRecord		extents;		// Initial set of extents
} ForkData;

// Catalog record types
enum
{
	kCatalogFolderType		= 1,			// Folder record
	kCatalogFileType		= 2,			// File record
	kCatalogFolderThreadType	= 3,			// Folder thread record
	kCatalogFileThreadType	= 4			// File thread record
};

// Large catalog file record - 248 bytes
typedef struct LargeCatalogFile
{
	Int16				recordType;	// Record type - HFS Plus file record
	UInt16				flags;		// File flags
	UInt32				reserved;		// Reserved - set to zero
	CatalogNodeID			fileID;		// File ID
	UInt32				createDate;	// Date and time of creation
	UInt32				contentModDate; // Date and time of last content (fork) modification
	UInt32				modifyDate;	// Date and time of last modification
	UInt32				accessDate;	// Date and time of last access (Rhapsody [MacOS X] only)
	UInt32				backupDate;	// Date and time of last backup
	Permissions			permissions;	// Permissions (for Rhapsody [MacOS X])
	FInfo					userInfo;		// Finder information
	FXInfo				finderInfo;	// Additional Finder information
	UInt32				textEncoding;	// Hint for name conversions
	UInt32				reserved2;	// Reserved - set to zero
	
									// Start of double long (64-bit) boundary
	ForkData				dataFork;		// Size and block data for data fork
	ForkData				resourceFork;	// Size and block data for resource fork
} LargeCatalogFile;

// Large catalog folder record - 88 bytes
typedef struct LargeCatalogFolder
{
	Int16				recordType;	// Record type - HFS Plus file record
	UInt16				flags;		// File flags
	UInt32				valence;		// Folder's valence (limited to 2^16 in Mac OS)
	CatalogNodeID			folderID;		// Folder ID
	UInt32				createDate;	// Date and time of creation
	UInt32				contentModDate; // Date and time of last content (fork) modification
	UInt32				modifyDate;	// Date and time of last modification
	UInt32				accessDate;	// Date and time of last access (Rhapsody [MacOS X] only)
	UInt32				backupDate;	// Date and time of last backup
	Permissions			permissions;	// Permissions (for Rhapsody [MacOS X])
	DInfo					userInfo;		// Finder information
	DXInfo				finderInfo;	// Additional Finder information
	UInt32				textEncoding;	// Hint for name conversions
	UInt32				reserved;		// Reserved - set to zero
} LargeCatalogFolder;

// Catalog file record flags
enum
{
	kFileLockedBit			= 0x0000,	// File is locked and cannot be written to
	kFileLockedMask		= 0x0001,
	kFileThreadExistsBit		= 0x0001,	// A file thread record exists for this file
	kFileThreadExistsMask	= 0x0002
};

// Large catalog thread record - 264 bytes
// This structure is used for both file and folder thread records
typedef struct LargeCatalogThread
{
	Int16				recordType;	// Record type
	Int16				reserved;		// Reserved - set to zero
	CatalogNodeID			parentID;		// Parent ID for this catalog node
	UniStr255				nodeName;	// name of this catalog node (variable length)
} LargeCatalogThread;	

typedef union CatalogRecord
{
	Int16				recordType;	// Record type
	
	LargeCatalogFolder		largeFolder;
	LargeCatalogFile		largeFile;
	LargeCatalogThread		largeThread;
} CatalogRecord;

// Large Extent Key
typedef struct LargeExtentKey
{
	UInt16				keyLength;	// Length of key, excluding this key
	UInt8				forkType;		// 0x00 = data fork, 0xFF = resource fork
	UInt8				pad;			// Make the other fields align on a 32-bit boundary
	CatalogNodeID			fileID;		// File ID
	UInt32				startBlock;	// First file allocation block number in this extent
} LargeExtentKey;

// VolumeHeader - 512 bytes
// Stored at sector #2 (3rd sector) and second to last sector
typedef struct VolumeHeader
{
	UInt16				signature;		// Volume signature == 'H+'
	UInt16				version;		// Current version is kHFSPlusVersion (Pat: not, see comments above)
	UInt32				attributes;	// Volume attributes
	UInt32				lastMountedVersion; // Implementation version which last mounted volume
	UInt32				reserved;		// Reserved - set to zero
	
	UInt32				createDate;	// Date and time of volume creation
	UInt32				modifyDate;	// Date and time of last modification
	UInt32				backupDate;	// Date and time of last backup
	UInt32				checkedDate;	// Date and time of last disk check
	
	UInt32				fileCount;		// Number of files in volume
	UInt32				folderCount;	// Number of directories in volume
	
	UInt32				blockSize;		// Size (in bytes) of allocation blocks
	UInt32				totalBlocks;	// Number of allocation blocks in volume (includes this header and VBM)
	UInt32				freeBlocks;	// Number of unused allocation blocks
	
	UInt32				nextAllocation;	// Start of next allocation search
	UInt32				rsrcClumpSize;	// Default resource fork clump size
	UInt32				dataClumpSize;	// Default data fork clump size
	CatalogNodeID			nextCatalogID;	// Next unused catalog ID
	
	UInt32				writeCount;	// Volume write count
	UInt64				encodingsBitmap; // Which encodings have been used on this volume
	
	UInt8				finderInfo[32];	// Information used by the Finder
	
	ForkData				alloctionFile;	// Allocation bitmap file
	ForkData				extentsFile;	// Extents B-tree file
	ForkData				catalogFile;	// Catalog B-tree file
	ForkData				attributesFile;	// Extended attributes B-tree file
	ForkData				startupFile;	// Boot file
} VolumeHeader;

// Attribute bits
enum
{
	// Bits 0-6 are reserved [MacOS Docs: always cleared by MountVol call]
	kVolumeHardwareLockBit	= 7,			// Volume is locked by hardware
	kVolumeUnmountedBit	= 8,			// Volume was successfully unmounted
	kVolumeSparedBlocksBit	= 9,			// Volume has bad blocks spared
	kVolumeNoCacheRequiredBit = 10,		// Don't cache volume blocks
	kBootVolumeInconsistentBit = 11,		// Boot volume is inconsistent (System 7.6 ->)
	
	// Bits 12-14 are reserved for future use
	
	kVolumeSoftwareLockBit	= 15			// Volume is locked by software
};
#pragma options align=reset

class AppleHFSPlusDirectoryDescriptor	:	public DirectoryDescriptor
{
	UInt32					_dirID;
	UInt32					_parID;
	UniStr255					_name;
	ASCII8					_transName[256];
	class AppleHFSPlusFileSystem* _fileSystem;
	
	AppleHFSPlusDirectoryDescriptor(AppleHFSPlusFileSystem* fileSystem,LargeBTreeNode* node,UInt32 record);
protected:
	AppleHFSPlusDirectoryDescriptor(AppleHFSPlusFileSystem* fileSystem,CatalogNodeID dirID);
public:
	AppleHFSPlusDirectoryDescriptor(class AppleHFSPlusDirectoryDescriptor& parentDir,ConstASCII8Str dirName);
	virtual ~AppleHFSPlusDirectoryDescriptor();
	
	virtual	ConstASCII8Str			name();
	virtual	DirectoryDescriptor*	parent();
	virtual	FileDescriptor*			subFile(ConstASCII8Str name);
	virtual	DirectoryDescriptor*	subDir(ConstASCII8Str name);
	virtual	FileIterator*			newFileIterator();
	virtual	DirectoryIterator*		newDirectoryIterator();
	virtual	Boolean				operator==(class DirectoryDescriptor& dd);
	
	virtual	Boolean				fileExists(ConstASCII8Str name);
			Boolean				fileExists(ConstASCII8Str name,LargeBTreeNode* outNode,UInt32* outRecord);
	virtual	Boolean				dirExists(ConstASCII8Str name);
			Boolean				dirExists(ConstASCII8Str name,LargeBTreeNode* outNode,UInt32* outRecord);
			
	friend class AppleHFSPlusFileDescriptor;
	friend class AppleHFSPlusFileSystem;
	friend class AppleHFSPlusFileIterator;
	friend class AppleHFSPlusDirectoryIterator;
};

class AppleHFSPlusFileDescriptor	:	public FileDescriptor
{
	UInt32					_parID;
	UInt32					_fileID;
	UniStr255					_name;
	ASCII8					_transName[256];
	class AppleHFSPlusFileSystem* _fileSystem;

	AppleHFSPlusFileDescriptor(class AppleHFSPlusFileSystem* fileSystem,LargeBTreeNode* node,UInt32 record);
public:
	AppleHFSPlusFileDescriptor(AppleHFSPlusDirectoryDescriptor& parentDir,ConstASCII8Str fileName);
	
	virtual	ConstASCII8Str			name();
	virtual	DirectoryDescriptor*	parent();
	virtual	FileIStream*			openForRead();
	virtual	Boolean				operator==(class FileDescriptor& dd);
	
	friend class AppleHFSPlusDirectoryDescriptor;
	friend class AppleHFSPlusFileIterator;
	friend class AppleHFSPlusFileIStream;
};

class AppleHFSPlusFileSystem	:	public FileSystem
{
protected:
	VolumeHeader					header;
	AppleHFSPlusDirectoryDescriptor*	_root;
public:
	class AppleHFSPlusCatalogBTree*	catalogBTree;
	class AppleHFSPlusExtentsBTree*	extentsBTree;
	
	AppleHFSPlusFileSystem(BlockDevicePartition* partition);
	
	virtual	ConstASCII8Str			name();
	virtual	DirectoryDescriptor*	root();
	virtual	UInt32				getAllocationBlockSector(UInt32 allocBlock);	// Gets the first sector in the given allocation block
	virtual	void					readAllocationBlock(void* p, UInt32 allocBlock);
	
	friend class AppleHFSPlusCatalogBTree;
	friend class AppleHFSPlusExtentsBTree;
	friend class AppleHFSPlusDirectoryDescriptor;
	friend class AppleHFSPlusFileIStream;
};

class AppleHFSPlusCatalogBTree	:	public LargeBTreeCached
{
	LargeBTreeNode		headerNode;
	AppleHFSPlusFileSystem*	fileSystem;

			Boolean		physReadNode(UInt32 n,LargeBTreeNode* outNode);
protected:
	AppleHFSPlusCatalogBTree(AppleHFSPlusFileSystem* fileSystem);
	
	virtual	Boolean		readNode(UInt32 n,LargeBTreeNode* outNode);
	virtual	Int8			compareKeys(LargeBTreeKey* key1,LargeBTreeKey* key2,Boolean partialCompare);	// A partial compare compares the smaller of the two names.
	
			Boolean		findNode(CatalogNodeID dirID,UInt32* outNode,UInt32* outRec,LargeBTreeNode* outNodeData = nil);
	
	friend class AppleHFSPlusFileSystem;
	friend class AppleHFSPlusDirectoryDescriptor;
	friend class AppleHFSPlusFileDescriptor;
	friend class AppleHFSPlusFileIterator;
	friend class AppleHFSPlusDirectoryIterator;
	friend class AppleHFSPlusFileIStream;
};

class AppleHFSPlusExtentsBTree	:	public LargeBTree
{
	LargeBTreeNode		headerNode;
	AppleHFSPlusFileSystem*	fileSystem;
	
	static	Boolean		searchExtentsRecord(LargeExtentDescriptor* rec,UInt32 recStartABN,UInt32 fileABN,UInt32* outABN,UInt32* outNumABNs);
			Boolean		physReadNode(UInt32 n,LargeBTreeNode* outNode);
protected:
	AppleHFSPlusExtentsBTree(AppleHFSPlusFileSystem* fileSystem);
	
	virtual	Boolean		readNode(UInt32 n,LargeBTreeNode* outNode);
	virtual	Int8			compareKeys(LargeBTreeKey* key1,LargeBTreeKey* key2,Boolean partialCompare);	// A partial compare doesn't compare the starting block number
public:
			Boolean		getFileAllocationBlock(CatalogNodeID fileID,Int8 fork,UInt32 fileABN,UInt32* outDiskABN,UInt32* outNumABNs);
	
	friend class AppleHFSPlusFileSystem;
	friend class AppleHFSPlusFileIStream;
};

class AppleHFSPlusFileIterator	:	public FileIterator
{
	LargeBTreeNode	node;
	UInt32			record;
	LargeCatalogKey	key;
public:
	AppleHFSPlusFileIterator(AppleHFSPlusDirectoryDescriptor* dir);
	virtual ~AppleHFSPlusFileIterator();
	
	virtual void				advance();
};

class AppleHFSPlusDirectoryIterator	:	public DirectoryIterator
{
	LargeBTreeNode	node;
	UInt32			record;
	LargeCatalogKey	key;
public:
	AppleHFSPlusDirectoryIterator(AppleHFSPlusDirectoryDescriptor* dir);
	virtual ~AppleHFSPlusDirectoryIterator();
	
	virtual	void				advance();
};

class AppleHFSPlusFileIStream	:	public FileIStream
{
	AppleHFSPlusFileDescriptor*	fileDesc;
	AppleHFSPlusFileSystem*		fileSystem;
	Int8						buffer[512];
	UInt32					bufferFileSector;
	UInt64					currPos;
	LargeCatalogFile			fileRec;
	
	AppleHFSPlusFileIStream(AppleHFSPlusFileDescriptor* file);
			void					readSectors(Ptr dest,UInt32 numSectors);
public:
	virtual ~AppleHFSPlusFileIStream();
	
	// For IStream
	virtual	void					read(Ptr data,UInt32 len);
	
	// For SeekableIStream
	virtual	void					setPos(UInt64 newPos);
	virtual	UInt64				getPos();
	virtual	UInt64				eos();
	
	// For FileIStream
	virtual	FileDescriptor*			file();
	virtual	DirectoryDescriptor*	directory();
	
	friend class AppleHFSPlusFileDescriptor;
};

class AppleHFSPlusFileSystemManager	:	public FileSystemManager
{
public:
	AppleHFSPlusFileSystemManager();
	
protected:
	virtual	FileSystem*	tryToBuildFileSystem(BlockDevicePartition* partition);
};

Boolean IsHFSPlusPartition( BlockDevicePartition* partition );

#endif /* __APPLE_HFS_PLUS__ */