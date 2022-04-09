#ifndef __PCI_BANDIT__
#define __PCI_BANDIT__

#include "PCI.h"
#include "OpenFirmware.h"

class PCIBanditDevice	:	public PCIBridgeDevice
{
	PCIBanditDevice(OpenFirmwarePCINode* banditDevice);
	
	virtual	UInt8	readConfig8(UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	UInt16	readConfig16(UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	UInt32	readConfig32(UInt8 theBus,UInt8 devFN,UInt8 offset);
	
	virtual	void		writeConfig8(UInt8 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	void		writeConfig16(UInt16 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	void		writeConfig32(UInt32 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	
	virtual	UInt8	firstSlot();
	virtual	UInt8	lastSlot();
public:
	virtual ~PCIBanditDevice();
	
	friend	void		InitBandit();
};

#endif /* __PCI_BANDIT__ */