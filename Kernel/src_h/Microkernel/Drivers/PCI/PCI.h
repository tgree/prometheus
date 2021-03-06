#ifndef __PCI__
#define __PCI__

#include "Kernel Types.h"

class PCIBridgeDevice
{
protected:
	// These are logical addresses, thanks.
	UReg32LE*				configAddr;
	UReg8*					configData;
	UInt32					ioLen;
	UInt32					busNumber;
	UInt32					maxBus;
	class OpenFirmwarePCINode*	pciNode;
	
	PCIBridgeDevice(OpenFirmwarePCINode* pciNode,UReg32BE* configAddr,UReg8* configData,Ptr ioBase,UInt32 ioLen,UInt32 busNumber,UInt32 maxBus);	// Pass physical addresses please
	
	virtual	UInt8	readConfig8(UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	virtual	UInt16	readConfig16(UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	virtual	UInt32	readConfig32(UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	
	virtual	void		writeConfig8(UInt8 data,UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	virtual	void		writeConfig16(UInt16 data,UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	virtual	void		writeConfig32(UInt32 data,UInt8 theBus,UInt8 devFN,UInt8 offset) = 0;
	
	virtual	UInt8	firstSlot() = 0;	// PCIBus::probe() will probe slot numbers between this...
	virtual	UInt8	lastSlot() = 0;	// ...and this, inclusive
public:
	Ptr				ioBase;
	
	virtual ~PCIBridgeDevice();
	
			void		probe();
	
	friend class PCIBus;
	friend class PCIDevice;
};

class PCIBus
{
	UInt32			busID;
protected:
	PCIBus(PCIBridgeDevice* driver,UInt32 busID);
	
	UInt8	readConfig8(UInt8 devFN,UInt8 offset);
	UInt16	readConfig16(UInt8 devFN,UInt8 offset);
	UInt32	readConfig32(UInt8 devFN,UInt8 offset);
	
	void		writeConfig8(UInt8 data,UInt8 devFN,UInt8 offset);
	void		writeConfig16(UInt16 data,UInt8 devFN,UInt8 offset);
	void		writeConfig32(UInt32 data,UInt8 devFN,UInt8 offset);
	
	void			probe();
public:
	PCIBridgeDevice*	driver;
	friend class PCISlot;
	friend class PCIBridgeDevice;
	friend class PCIDevice;
};

class PCISlot
{
	UInt32		slotID;
protected:
	PCISlot(PCIBus* driver,UInt32 slotID);
	~PCISlot();
	
	UInt8	readConfig8(UInt8 func,UInt8 offset);
	UInt16	readConfig16(UInt8 func,UInt8 offset);
	UInt32	readConfig32(UInt8 func,UInt8 offset);
	
	void		writeConfig8(UInt8 data,UInt8 func,UInt8 offset);
	void		writeConfig16(UInt16 data,UInt8 func,UInt8 offset);
	void		writeConfig32(UInt32 data,UInt8 func,UInt8 offset);
public:
	PCIBus*		myBus;
	class PCIDevice*	device[8];	// A device is nil if it doesn't exist or if this is not a multifunction device
	
	Boolean	probe();
	
	friend class PCIBus;
	friend class PCIDevice;
};

class PCIDevice
{
	UInt32			func;
	class PCIDevice*	_next;
protected:
	UInt32			baseAddr[6];
	UInt32			baseLen[6];
	
	PCIDevice(PCIDevice* replaceThisDevice);	// Call this when you are installing a real driver to replace this dummy one
	PCIDevice(PCISlot* _mySlot,UInt32 _func);

	Boolean			probe();		// Returns true if a device is present.  Called by PCISlot when first testing a device
	
public:
	PCISlot*			mySlot;
	
	UInt8	readConfig8(UInt8 offset);
	UInt16	readConfig16(UInt8 offset);
	UInt32	readConfig32(UInt8 offset);
	
	void		writeConfig8(UInt8 data,UInt8 offset);
	void		writeConfig16(UInt16 data,UInt8 offset);
	void		writeConfig32(UInt32 data,UInt8 offset);
	
	// Config space write accessors
	void		writeCommand(UInt16 data);	// Command register, offset 4
	void		writeLineSize(UInt8 data);	// Cache line size, offset 12
	void		writeLatency(UInt8 data);		// Latency timer, offset 13
	void		writeBaseAddr(UInt32 data,UInt8 n);	// Base addresses 0-5.
	void		writeInterruptLine(UInt8 data);	// Interrupt line, offset 0x3C
	void		writeInterruptPin(UInt8 data);	// Interrupt pin, offset 0x3D
	
	virtual ~PCIDevice();
	
	PCIDevice*		next();		// Returns the next registered PCI device (not necessarily the same bus!)
	
	// Config space read accessors
	UInt16	readVendorID();		// Vendor ID, offset 0
	UInt16	readDeviceID();			// Device ID, offset 2
	UInt16	readCommand();		// Command register, offset 4
	UInt16	readStatus();			// Status register, offset 6
	UInt16	readClass();			// Class/revision stuff, offset 8
	UInt8	readRegLevel();		// Class/revision stuff, offset 8
	UInt8	readRevision();			// Class/revision stuff, offset 8
	UInt8	readLineSize();			// Cache line size, offset 12
	UInt8	readLatency();			// Latency timer, offset 13
	UInt8	readHeaderType();		// Header type, offset 14
	UInt8	readBIST();			// BIST, offst 15
	UInt32	readBaseAddr(UInt8 n);	// Base addresses 0-5.
	UInt32	readBaseLen(UInt8 n);	// Base address length 0-5 (writes 0xFFFFFFFF to addr and reads back to get length)
	UInt8	readInterruptLine();		// Interrupt line, offset 0x3C
	UInt8	readInterruptPin();		// Interrupt pine, offset 0x3D
	
	// Useful stuff
	ConstASCII8Str		vendorName();	// Returns nil if unrecognized
	ConstASCII8Str		deviceName();	// Returns nil if unrecognized
	ConstASCII8Str		className();	// Returns nil if unrecognized
	UInt32			busID();		// Returns the bus ID
	UInt32			slotID();		// Returns the slot ID
	UInt32			funcID();		// Returns the function ID
	
	// Baseaddr stuff
	Ptr				mapBaseAddr(UInt8 n);	// Maps the particular baseAddr for you, and returns the logical address
	UInt32			getBaseAddr(UInt8 n);	// Gets baseAddr without probing device registers
	UInt32			getBaseLen(UInt8 n);		// Gets baseLen without probing device registers
	
	friend class PCISlot;
};

// For each PCI driver you write, make a global subclass of this.  It will register itself and be called at PCI probe time to find
// a driver for the device.
class PCIProber
{
	class PCIProber*	next;
protected:
	PCIProber();
	virtual ~PCIProber();
	
	virtual	Boolean	probe(PCIDevice* pciDevice) = 0;	// Return true if you you recognize the device.
	
	friend void InitPCI();
};

void InitPCI();

#endif /* __PCI__ */