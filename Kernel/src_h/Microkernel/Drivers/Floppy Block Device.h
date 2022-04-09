#ifndef __FLOPPY_BLOCK_DEVICE__
#define __FLOPPY_BLOCK_DEVICE__

class FloppyBlockDevicePartition	:	public BlockDevicePartition
{
protected:
	FloppyBlockDevicePartition(BlockDevice* myDevice);
	
	virtual	UInt32	partitionType();
	friend class FloppyBlockDeviceManager;
};

class FloppyBlockDeviceManager	:	public BlockDeviceManager
{
public:
	FloppyBlockDeviceManager();
	virtual ~FloppyBlockDeviceManager();
protected:
	virtual	Boolean	tryToBuildPartitionList(BlockDevice* device);
};

#endif /* __FLOPPY_BLOCK_DEVICE__ */