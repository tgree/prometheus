#ifndef __PCI_GRACKLE__
#define __PCI_GRACKLE__

#include "PCI.h"
#include "OpenFirmware.h"

class PCIGrackleDevice	:	public PCIBridgeDevice
{
	PCIGrackleDevice(OpenFirmwarePCINode* grackleDevice);
	
	virtual	UInt8	readConfig8(UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	UInt16	readConfig16(UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	UInt32	readConfig32(UInt8 theBus,UInt8 devFN,UInt8 offset);
	
	virtual	void		writeConfig8(UInt8 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	void		writeConfig16(UInt16 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	virtual	void		writeConfig32(UInt32 data,UInt8 theBus,UInt8 devFN,UInt8 offset);
	
	virtual	UInt8	firstSlot();
	virtual	UInt8	lastSlot();
	
	static	Boolean	PCIGrackleMachineCheckHandler(class PPCRegisters*);
public:
	virtual ~PCIGrackleDevice();
	
	friend	void		InitGrackle();
};

#endif /* __PCI_GRACKLE__ */