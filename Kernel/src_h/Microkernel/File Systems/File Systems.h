/*
	File Systems.h
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
#ifndef __FILE_SYSTEMS__
#define __FILE_SYSTEMS__

#include "Streams.h"
#include "Block Device.h"

class DirectoryDescriptor
{
public:
	virtual ~DirectoryDescriptor();
	
	virtual	ConstASCII8Str			name() = 0;	// Returns the name of the directory
	virtual	class DirectoryDescriptor*	parent() = 0;	// Returns the parent directory
	virtual	class FileDescriptor*		subFile(ConstASCII8Str name) = 0;	// Returns a file descriptor
	virtual	class DirectoryDescriptor*	subDir(ConstASCII8Str name) = 0;	// Returns a subdirectory
	virtual	class FileIterator*			newFileIterator() = 0;
	virtual	class DirectoryIterator*		newDirectoryIterator() = 0;
	virtual	Boolean					operator==(class DirectoryDescriptor& dd) = 0;
	
	virtual	Boolean					fileExists(ConstASCII8Str name) = 0;	// Returns true if a file with that name is in this directory
	virtual	Boolean					dirExists(ConstASCII8Str name) = 0;	// Returns true if a file with that name is in this directory
};

class FileDescriptor
{
public:
	virtual ~FileDescriptor();
	
	virtual	ConstASCII8Str			name() = 0;	// Returns the name of the file
	virtual	DirectoryDescriptor*		parent() = 0;	// Returns the parent directory
	virtual	class FileIStream*			openForRead() = 0;
	virtual	Boolean					operator==(class FileDescriptor& dd) = 0;
};

class FileSystem
{
protected:
	BlockDevicePartition*	_partition;			// Our partition
	class FileSystem*		_next;				// The next file system
	
	FileSystem(BlockDevicePartition* partition);
public:
	virtual ~FileSystem();
	
	virtual	ConstASCII8Str		name() = 0;		// Get the name of the volume
	virtual	DirectoryDescriptor*	root() = 0;		// Get the root directory
	
			class FileSystem*			next();			// Get the next file system
			class BlockDevicePartition*	partition();	// Get the partition
};

class FileSystemManager
{
	class FileSystemManager*	next;
protected:
	FileSystemManager();
	
	virtual	FileSystem*	tryToBuildFileSystem(BlockDevicePartition* partition) = 0;	// Return nil if you don't recognize the partition
public:
	virtual ~FileSystemManager();
	
	static	FileSystem*	buildFileSystem(BlockDevicePartition* partition);
};

class FileIterator
{
protected:
	FileDescriptor*			currFile;
	DirectoryDescriptor*	dir;
	
	FileIterator(DirectoryDescriptor* dir);
public:
	virtual ~FileIterator();
	
			FileDescriptor&	operator*();			// De-referencing yields a FileDescriptor&
			FileDescriptor*		operator->();
							operator FileDescriptor*();
	
	virtual	void				advance() = 0;
};

class DirectoryIterator
{
protected:
	DirectoryDescriptor*	currDir;
	DirectoryDescriptor*	dir;
	
	DirectoryIterator(DirectoryDescriptor* dir);
public:
	virtual ~DirectoryIterator();
	
			DirectoryDescriptor&	operator*();
			DirectoryDescriptor*	operator->();
								operator DirectoryDescriptor*();
	
	virtual	void					advance() = 0;
};

class FileIStream	:	virtual	public	SeekableIStream
{
protected:
	FileIStream();
public:
	virtual ~FileIStream();
	
	virtual	FileDescriptor*			file() = 0;
	virtual	DirectoryDescriptor*	directory() = 0;
};

extern DirectoryDescriptor* kernelDirectory;

#endif /* __FILE_SYSTEMS__ */