#include "Apple HFS.h"
#include "Floppy Block Device.h"
#include "NKThreads.h"

static FloppyBlockDeviceManager	floppyBlockDeviceManager;

FloppyBlockDeviceManager::FloppyBlockDeviceManager()
{
}

FloppyBlockDeviceManager::~FloppyBlockDeviceManager()
{
}

Boolean FloppyBlockDeviceManager::tryToBuildPartitionList(BlockDevice* device)
{
	if(device->bus() != floppyBus)
		return false;
	
	if(device->sectorSize() != 512)
		return false;
	
	BootBlockHeader		bootBlock;
	MasterDirectoryBlock	mdb;
	
	IOCommand*			cmd1 = device->readSectorsAsync((Int8*)&bootBlock,0,1);
	IOCommand*			cmd2 = device->readSectorsAsync((Int8*)&mdb,2,1);
	CurrThread::blockForIO(cmd2);
	delete cmd1;
	delete cmd2;
	
	if(bootBlock.bbID != bbID && bootBlock.bbID != 0)
		return false;
	
	if(mdb.drSigWord != dirSigWord)
		return false;
	
	BlockDevicePartition*	newPartition = new FloppyBlockDevicePartition(device);
	BlockDeviceManager::addPartitionToDevice(device,newPartition);
	
	return true;
}

FloppyBlockDevicePartition::FloppyBlockDevicePartition(BlockDevice* myDevice):
	BlockDevicePartition(0,2880,myDevice)
{
}

UInt32 FloppyBlockDevicePartition::partitionType()
{
	return appleHFSPartitionType;
}
