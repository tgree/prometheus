/*
	File Systems.cp
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
#include "Kernel Types.h"
#include "File Systems.h"
#include "NKMachineInit.h"
#include "Kernel Console.h"

DirectoryDescriptor* kernelDirectory;

static FileSystemManager*	fileSystemManagerList = nil;

DirectoryDescriptor::~DirectoryDescriptor()
{
}

FileDescriptor::~FileDescriptor()
{
}

FileSystem::FileSystem(BlockDevicePartition* _partition)
{
	this->_partition = _partition;
	_next = machine.fileSystems;
	machine.fileSystems = this;
}

FileSystem::~FileSystem()
{
}

FileSystem* FileSystem::next()
{
	return _next;
}

BlockDevicePartition* FileSystem::partition()
{
	return _partition;
}

FileSystemManager::FileSystemManager()
{
	next = fileSystemManagerList;
	fileSystemManagerList = this;
}

FileSystemManager::~FileSystemManager()
{
	Panic("Fill this in later!\n");
}

FileSystem* FileSystemManager::buildFileSystem(BlockDevicePartition* partition)
{
	FileSystemManager*	currManager = fileSystemManagerList;
	while(currManager)
	{
		FileSystem*	fileSystem = currManager->tryToBuildFileSystem(partition);
		if(fileSystem)
			return fileSystem;
		currManager = currManager->next;
	}
	
	return nil;
}

FileIterator::FileIterator(DirectoryDescriptor* _dir)
{
	currFile = nil;
	dir = _dir;
}

FileIterator::~FileIterator()
{
}

FileDescriptor& FileIterator::operator*()
{
	return *currFile;
}

FileDescriptor* FileIterator::operator->()
{
	return currFile;
}

FileIterator::operator FileDescriptor*()
{
	return currFile;
}

DirectoryIterator::DirectoryIterator(DirectoryDescriptor* _dir)
{
	currDir = nil;
	dir = _dir;
}

DirectoryIterator::~DirectoryIterator()
{
}

DirectoryDescriptor& DirectoryIterator::operator*()
{
	return *currDir;
}

DirectoryDescriptor* DirectoryIterator::operator->()
{
	return currDir;
}

DirectoryIterator::operator DirectoryDescriptor*()
{
	return currDir;
}

FileIStream::FileIStream()
{
}

FileIStream::~FileIStream()
{
}
