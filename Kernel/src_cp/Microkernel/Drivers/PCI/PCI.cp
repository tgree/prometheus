#include "NKVirtualMemory.h"
#include "PCI.h"
#include "PCIBanditDevice.h"
#include "PCIGrackleDevice.h"
#include "Kernel Console.h"
#include "NKMachineInit.h"

static PCIProber*	proberList = nil;

void InitPCI()
{
	// First, construct the PCI device list
	machine.pciDeviceList = nil;
	InitGrackle();
	InitBandit();
	
	// Now print info on the PCI devices
	PCIDevice*	theDev = machine.pciDeviceList;
	cout << greenMsg << "PCI Device List\n---------------\n" << whiteMsg;
	
	while(theDev)
	{
		ConstASCII8Str		vendorName = theDev->vendorName();
		ConstASCII8Str		deviceName = theDev->deviceName();
		ConstASCII8Str		className = theDev->className();
		
		cout << decMsg << greenMsg << theDev->busID() << ":" << theDev->slotID() << "." << theDev->funcID() << "\t";
		if(vendorName)
			cout << vendorName;
		else
			cout << "Vendor = " << hexMsg << theDev->readVendorID() << decMsg;
		if(deviceName)
			cout << " " << deviceName;
		else
			cout << ", deviceID = " << hexMsg << theDev->readDeviceID() << decMsg;
		
		cout << whiteMsg << " - ";
		if(className)
			cout << className;
		else
			cout << hexMsg << theDev->readClass() << decMsg;
		cout << "\n";
		if(theDev->readInterruptPin())
			cout << "\t\tInterrupts: pin = " << theDev->readInterruptPin() << ", line = " << theDev->readInterruptLine() << "\n";
		cout << "\t\tCommand register: " << hexMsg << theDev->readCommand() << decMsg << "\n";
		cout << "\t\tdevFN: " << hexMsg << (UInt8)((theDev->slotID() << 3) | theDev->funcID()) << decMsg << "\n";
		
		UInt32 numAddrs = 0;
		switch(theDev->readHeaderType() & 0x7F)
		{
			case 0:	// Normal
				numAddrs = 6;
			break;
			case 1:	// Bridge
				numAddrs = 2;
			break;
			case 2:	// Cardbus bridge
				numAddrs = 1;
			break;
			default:	// ???
				cout << redMsg << "Unrecognized device type in InitPCI()\n" << whiteMsg;
			break;
		}
		for(UInt32 i=0;i<numAddrs;i++)
		{
			UInt32	baseAddr = theDev->getBaseAddr(i);
			UInt32	baseLen = theDev->getBaseLen(i);
			
			if(baseLen == 0)
				continue;
			if(baseAddr == 0xFFFFFFFF)
				baseAddr = 0;
			
			Boolean	ioSpace = (baseAddr & 0x00000001);
			if(ioSpace) // Clear the IO space bit
				baseAddr &= 0xFFFFFFFE;
			
			FatalAssert((!ioSpace) || (ioSpace && baseLen <= 0x00010000));	// I guess you can't have more than 0x10000 bytes of IO space or something... LinuxPPC sources.
				
			if(baseLen)
			{
				cout << "\t\tBase addr[" << i << hexMsg << "]: " << baseAddr;
				cout << " - " << (baseAddr + baseLen) << decMsg << (ioSpace ? " (IO Space)" : " (Memory Space)") << "\n";
			}
		}
		theDev = theDev->next();
	}
	cout << greenMsg << "---------------\n" << whiteMsg;
	
	// Now probe for drivers
	theDev = machine.pciDeviceList;
	while(theDev)
	{
		PCIProber*	prober = proberList;
		while(prober)
		{
			if(prober->probe(theDev))
				break;
			prober = prober->next;
		}
		theDev = theDev->next();
	}
}

PCIProber::PCIProber()
{
	next = proberList;
	proberList = this;
}

PCIProber::~PCIProber()
{
}

PCIBridgeDevice::PCIBridgeDevice(OpenFirmwarePCINode* node,UReg32BE* _configAddr,UReg8* _configData,Ptr _ioBase,UInt32 _ioLen,UInt32 _busNumber,UInt32 _maxBus)
{
	pciNode = node;
	configAddr = (UReg32BE*)NKIOMap((void*)_configAddr,0x1000,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	configData = (UReg8*)NKIOMap((void*)_configData,0x1000,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	ioBase = (Ptr)NKIOMap((void*)_ioBase,_ioLen,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	ioLen = _ioLen;
	busNumber = _busNumber;
	maxBus = _maxBus;
}

PCIBridgeDevice::~PCIBridgeDevice()
{
}

void PCIBridgeDevice::probe()
{
	for(UInt32 i=busNumber;i<=(Int32)maxBus;i++)
	{
		cout << "Probing PCI bus " << i << "...\n";
		(new PCIBus(this,i))->probe();
	}
}

PCIBus::PCIBus(PCIBridgeDevice* _driver,UInt32 _busID)
{
	busID = _busID;
	driver = _driver;
}

UInt8 PCIBus::readConfig8(UInt8 devFN,UInt8 offset)
{
	return driver->readConfig8(busID,devFN,offset);
}

UInt16 PCIBus::readConfig16(UInt8 devFN,UInt8 offset)
{
	return driver->readConfig16(busID,devFN,offset);
}

UInt32 PCIBus::readConfig32(UInt8 devFN,UInt8 offset)
{
	return driver->readConfig32(busID,devFN,offset);
}

void PCIBus::writeConfig8(UInt8 data,UInt8 devFN,UInt8 offset)
{
	driver->writeConfig8(data,busID,devFN,offset);
}

void PCIBus::writeConfig16(UInt16 data,UInt8 devFN,UInt8 offset)
{
	driver->writeConfig16(data,busID,devFN,offset);
}

void PCIBus::writeConfig32(UInt32 data,UInt8 devFN,UInt8 offset)
{
	driver->writeConfig32(data,busID,devFN,offset);
}

void PCIBus::probe()
{
	UInt8	firstSlot = driver->firstSlot();
	UInt8	lastSlot = driver->lastSlot();
	for(UInt8 i=firstSlot;i<=lastSlot;i++)
	{
		PCISlot* slot = new PCISlot(this,i);
		if(!slot->probe())
			delete slot;
	}
}

PCISlot::PCISlot(PCIBus* _myBus,UInt32 _slotID)
{
	slotID = _slotID;
	myBus = _myBus;
}

PCISlot::~PCISlot()
{
}

UInt8 PCISlot::readConfig8(UInt8 func,UInt8 offset)
{
	return myBus->readConfig8((slotID << 3) | (func & 0x07),offset);
}

UInt16 PCISlot::readConfig16(UInt8 func,UInt8 offset)
{
	return myBus->readConfig16((slotID << 3) | (func & 0x07),offset);
}

UInt32 PCISlot::readConfig32(UInt8 func,UInt8 offset)
{
	return myBus->readConfig32((slotID << 3) | (func & 0x07),offset);
}

void PCISlot::writeConfig8(UInt8 data,UInt8 func,UInt8 offset)
{
	myBus->writeConfig8(data,(slotID << 3) | (func & 0x07),offset);
}

void PCISlot::writeConfig16(UInt16 data,UInt8 func,UInt8 offset)
{
	myBus->writeConfig16(data,(slotID << 3) | (func & 0x07),offset);
}

void PCISlot::writeConfig32(UInt32 data,UInt8 func,UInt8 offset)
{
	myBus->writeConfig32(data,(slotID << 3) | (func & 0x07),offset);
}

Boolean PCISlot::probe()
{
	Boolean	retVal = false;
	
	for(UInt32 i=0;i<8;i++)
	{
		device[i] = new PCIDevice(this,i);
		if(device[i]->probe())
			retVal = true;
		else
		{
			delete device[i];
			device[i] = nil;
		}
	}
	return retVal;
}

PCIDevice::PCIDevice(PCIDevice* replaceThisDevice)
{
	FatalAssert(replaceThisDevice);
	
	PCIDevice*	prev = nil;
	PCIDevice*	currDev = machine.pciDeviceList;
	while(currDev && currDev != replaceThisDevice)
	{
		prev = currDev;
		currDev = currDev->_next;
	}
	
	FatalAssert(currDev == replaceThisDevice);
	if(!prev)
		machine.pciDeviceList = this;
	else
		prev->_next = this;
	
	func = replaceThisDevice->func;
	mySlot = replaceThisDevice->mySlot;
	_next = replaceThisDevice->_next;
	
	for(UInt32 i=0;i<6;i++)
	{
		baseAddr[i] = replaceThisDevice->baseAddr[i];
		baseLen[i] = replaceThisDevice->baseLen[i];
	}
	
	delete replaceThisDevice;
}

PCIDevice::PCIDevice(PCISlot* _mySlot,UInt32 _func)
{
	func = _func;
	mySlot = _mySlot;
	_next = nil;
	
	PCIDevice*	prev = nil;
	PCIDevice*	currDev = machine.pciDeviceList;
	while(currDev)
	{
		prev = currDev;
		currDev = currDev->_next;
	}
	
	if(prev)
		prev->_next = this;
	else
		machine.pciDeviceList = this;
	
	for(UInt32 i=0;i<6;i++)
		baseAddr[i] = baseLen[i] = 0;
}

PCIDevice::~PCIDevice()
{
	// Remove ourselves from the machine list if we are in it
	PCIDevice*	prev = nil;
	PCIDevice*	currDev = machine.pciDeviceList;
	while(currDev && currDev != this)
	{
		prev = currDev;
		currDev = currDev->_next;
	}
	
	if(currDev == this)
	{
		if(!prev)
			machine.pciDeviceList = _next;
		else
			prev->_next = _next;
	}
}

PCIDevice* PCIDevice::next()
{
	return _next;
}

UInt8 PCIDevice::readConfig8(UInt8 offset)
{
	return mySlot->readConfig8(func,offset);
}

UInt16 PCIDevice::readConfig16(UInt8 offset)
{
	return mySlot->readConfig16(func,offset);
}

UInt32 PCIDevice::readConfig32(UInt8 offset)
{
	return mySlot->readConfig32(func,offset);
}

void PCIDevice::writeConfig8(UInt8 data,UInt8 offset)
{
	mySlot->writeConfig8(data,func,offset);
}

void PCIDevice::writeConfig16(UInt16 data,UInt8 offset)
{
	mySlot->writeConfig16(data,func,offset);
}

void PCIDevice::writeConfig32(UInt32 data,UInt8 offset)
{
	mySlot->writeConfig32(data,func,offset);
}

UInt16 PCIDevice::readVendorID()
{
	return readConfig16(0);
}

UInt16 PCIDevice::readDeviceID()
{
	return readConfig16(2);
}

UInt16 PCIDevice::readCommand()
{
	return readConfig16(4);
}

UInt16 PCIDevice::readStatus()
{
	return readConfig16(6);
}

UInt16 PCIDevice::readClass()
{
	return ((readConfig32(8) >> 16) & 0x0000FFFF);
}

UInt8 PCIDevice::readRegLevel()
{
	return ((readConfig32(8) >> 8) & 0x000000FF);
}

UInt8 PCIDevice::readRevision()
{
	return (readConfig32(8) & 0x000000FF);
}

UInt8 PCIDevice::readLineSize()
{
	return readConfig8(12);
}

UInt8 PCIDevice::readLatency()
{
	return readConfig8(13);
}

UInt8 PCIDevice::readHeaderType()
{
	return readConfig8(14);
}

UInt8 PCIDevice::readBIST()
{
	return readConfig8(15);
}

UInt32 PCIDevice::readBaseAddr(UInt8 n)
{
	FatalAssert(n <= 5);
	return readConfig32(16 + n*4);
}

UInt32 PCIDevice::readBaseLen(UInt8 n)
{
	FatalAssert(n <= 5);
	writeBaseAddr(0xFFFFFFFF,n);
	return readConfig32(16 + n*4);
}

UInt8 PCIDevice::readInterruptLine()
{
	return readConfig8(0x3C);
}

UInt8 PCIDevice::readInterruptPin()
{
	return readConfig8(0x3D);
}

void PCIDevice::writeCommand(UInt16 data)
{
	writeConfig16(data,4);
}

void PCIDevice::writeLineSize(UInt8 data)
{
	writeConfig8(data,12);
}

void PCIDevice::writeLatency(UInt8 data)
{
	writeConfig8(data,13);
}

void PCIDevice::writeBaseAddr(UInt32 data,UInt8 n)
{
	FatalAssert(n <= 5);
	writeConfig32(data,16 + n*4);
}

void PCIDevice::writeInterruptLine(UInt8 data)
{
	writeConfig8(data,0x3C);
}

void PCIDevice::writeInterruptPin(UInt8 data)
{
	writeConfig8(data,0x3D);
}

Boolean PCIDevice::probe()
{
	Boolean	retVal = false;
	
	// devFN 0xFF doesn't exist
	if(mySlot->slotID == 31 && func == 7)
		return false;
	
	// Make sure there's really a device there
	UInt16	slotVendorID = mySlot->readConfig16(0,0);
	UInt16	slotDeviceID = mySlot->readConfig16(0,2);
	if(slotVendorID == 0 || slotVendorID == 0x0000FFFF || slotDeviceID == 0 || slotDeviceID == 0x0000FFFF)
		return false;
	
	// Find out if this is a multi device
	Boolean	isMulti = ((mySlot->readConfig8(0,0x0E) & 0x80) != 0);	// Read the header type field from the PCI device
	
	// If this is not a multi device, we can't touch it unless we are the first function
	if(func == 0 || isMulti)
	{
		// Read the vendor and device id.  This should return either 0 or -1 if there is no board in the slot (or function)
		UInt16	vendorID = readVendorID();
		UInt16	deviceID = readDeviceID();
		UInt16	deviceClass = readClass();
		if(vendorID != 0 && vendorID != 0x0000FFFF && deviceID != 0 && deviceID != 0x0000FFFF)
		{
			// OK, we found a function.  Set up IRQ's if it has one
			if(readInterruptPin())
			{
				UInt32	irq = mySlot->myBus->driver->pciNode->irq(mySlot->myBus->busID,(mySlot->slotID << 3) | func);
				if(irq != 0xFFFFFFFF)
					writeInterruptLine(irq);
			}
			
			UInt32	numAddrs = 0;
			UInt32	headerType = (readHeaderType() & 0x7F);
			switch(headerType)
			{
				case 0:	// Normal PCI device
					numAddrs = 6;
					if(deviceClass == 0x0604)	// PCI bridge class
					{
						cout << redMsg << "\t\t\tA normal PCI header had a PCI bridge class\n" << whiteMsg;
						numAddrs = 0;
					}
				break;
				case 1:	// PCI bridge
					numAddrs = 2;
					if(deviceClass != 0x0604)		// PCI bridge class
					{
						cout << redMsg << "\t\t\tA PCI bridge header had a non-PCI bridge class\n" << whiteMsg;
						numAddrs = 0;
					}
				break;
				case 2:	// Cardbus bridge
					numAddrs = 1;
					if(deviceClass != 0x0607)		// Cardbus bridge class
					{
						cout << redMsg << "\t\t\tA Cardbus bridge header had a non-Cardbus bridge class\n" << whiteMsg;
						numAddrs = 0;
					}
				break;
				default:
					cout << redMsg << "\t\t\tUnrecognized device type (" << headerType << ")\n" << whiteMsg;
				break;
			}
			
			for(UInt32 i=0;i < numAddrs;i++)
			{
				// First, block access to IO and Memory spaces
				UInt16	cmd = readCommand();
				writeCommand(cmd & ~(0x0003));
				
				// Read the base address and length
				UInt32	base = readBaseAddr(i);
				UInt32	len = readBaseLen(i);
				writeBaseAddr(base,i);	// Reset the base address to what it was, so as not to confuse the device
				
				// Restore access to IO and Memory spaces
				writeCommand(cmd);
				
				// Print the results
				if(len == 0)
					continue;
				if(base == 0xFFFFFFFF)
					base = 0;
				
				Boolean	ioSpace = (base & 1);
				if(ioSpace)
					len &= 0xFFFFFFFE;
				len = -len;
				FatalAssert((!ioSpace) || (ioSpace && len <= 0x00010000));
				
				baseAddr[i] = base;
				baseLen[i] = len;
			}
			
			retVal = true;
		}
	}
	
	// Done
	return retVal;
}

// This is a partial table.  I got tired of copying it - will finish it later.
typedef struct PCIVendorDeviceTable
{
	UInt16		vendorID;
	UInt16		deviceID;
	ConstASCII8Str		vendorName;
	ConstASCII8Str		deviceName;
}PCIVendorDeviceTable;
PCIVendorDeviceTable	pciVendorDeviceTable[]	=	{	{	0x0E11,	0x0508,	"Compaq",	"Tokenring"},
												{	0x0E11,	0x3033,	"Compaq",	"1280"},
												{	0x0E11,	0x4000,	"Compaq",	"Triflex"},
												{	0x0E11,	0x6010,	"Compaq",	"6010"},
												{	0x0E11,	0xAE10,	"Compaq",	"Smart 2P"},
												{	0x0E11,	0xAE32,	"Compaq",	"Netel 100"},
												{	0x0E11,	0xAE34,	"Compaq",	"Netel 10"},
												{	0x0E11,	0xAE35,	"Compaq",	"Netflex 31"},
												{	0x0E11,	0xAE40,	"Compaq",	"Netel 100D"},
												{	0x0E11,	0xAE43,	"Compaq",	"Netel 100PI"},
												{	0x0E11,	0xB011,	"Compaq",	"Netel 100I"},
												{	0x0E11,	0xF130,	"Compaq",	"Thunder"},
												{	0x0E11,	0xF150,	"Compaq",	"Netflex 3B"},
												{	0x0D11,	0,		"Compaq",	"Unrecognized"},
												
												{	0x1000,	0x0001,	"NCR",		"53C810"},
												{	0x1000,	0x0002,	"NCR",		"53C820"},
												{	0x1000,	0x0003,	"NCR",		"53C825"},
												{	0x1000,	0x0004,	"NCR",		"53C815"},
												{	0x1000,	0x0006,	"NCR",		"53C860"},
												{	0x1000,	0x000B,	"NCR",		"53C896"},
												{	0x1000,	0x000C,	"NCR",		"53C895"},
												{	0x1000,	0x000D,	"NCR",		"53C885"},
												{	0x1000,	0x000F,	"NCR",		"53C875"},
												{	0x1000,	0x0010,	"NCR",		"53C1510"},
												{	0x1000,	0x008F,	"NCR",		"53C875J"},
												{	0x1000,	0,		"NCR",		"Unrecognized"},
												
												{	0x1002,	0x4158,	"ATI",		"68800"},
												{	0x1002,	0x4354,	"ATI",		"215CT222"},
												{	0x1002,	0x4358,	"ATI",		"210888CX"},
												{	0x1002,	0x4742,	"ATI",		"215GB"},
												{	0x1002,	0x4744,	"ATI",		"215GD"},
												{	0x1002,	0x4749,	"ATI",		"215GI"},
												{	0x1002,	0x4750,	"ATI",		"215GP"},
												{	0x1002,	0x4751,	"ATI",		"215GQ"},
												{	0x1002,	0x4754,	"ATI",		"215GT"},
												{	0x1002,	0x4755,	"ATI",		"215GTB"},
												{	0x1002,	0x4756,	"ATI",		"RagePro C"},
												{	0x1002,	0x4758,	"ATI",		"210888GX"},
												{	0x1002,	0x4C47,	"ATI",		"215LG"},
												{	0x1002,	0x4C54,	"ATI",		"264LT"},
												{	0x1002,	0x5245,	"ATI",		"Rage 128"},
												{	0x1002,	0x5654,	"ATI",		"264VT"},
												{	0x1002,	0,		"ATI",		"Unrecognized"},
												
												{	0x1004,	0x0005,	"VLSI",		"82C592"},
												{	0x1004,	0x0006,	"VLSI",		"82C593"},
												{	0x1004,	0x0007,	"VLSI",		"82C594"},
												{	0x1004,	0x0009,	"VLSI",		"82C597"},
												{	0x1004,	0x000C,	"VLSI",		"82C541"},
												{	0x1004,	0x000D,	"VLSI",		"82C543"},
												{	0x1004,	0x0101,	"VLSI",		"82C532"},
												{	0x1004,	0x0102,	"VLSI",		"82C534"},
												{	0x1004,	0x0104,	"VLSI",		"82C535"},
												{	0x1004,	0x0105,	"VLSI",		"82C147"},
												{	0x1004,	0x0702,	"VLSI",		"VAS96011"},
												{	0x1004,	0,		"VLSI",		"Unrecognized"},
												
												{	0x1005,	0x2301,	"ADL",		"2301"},
												{	0x1005,	0,		"ADL",		"Unrecognized"},
												
												{	0x100B,	0x0002,	"NS",		"87415"},
												{	0x100B,	0xD001,	"NS",		"87410"},
												{	0x100B,	0,		"NS",		"Unrecognized"},
												
												{	0x100C,	0x3202,	"Tseng",		"W32P 2"},
												{	0x100C,	0x3205,	"Tseng",		"W32P B"},
												{	0x100C,	0x3206,	"Tseng",		"W32P C"},
												{	0x100C,	0x3207,	"Tseng",		"W32P D"},
												{	0x100C,	0x3208,	"Tseng",		"ET6000"},
												{	0x100C,	0,		"Tseng",		"Unrecognized"},
												
												{	0x100E,	0x9001,	"Weitek",		"P9000"},
												{	0x100E,	0x9100,	"Weitek",		"P9100"},
												{	0x100E,	0,		"Weitek",		"Unrecognized"},
												
												{	0x1011,	0x0001,	"DEC",		"BRD"},
												{	0x1011,	0x0002,	"DEC",		"Tulip"},
												{	0x1011,	0x0004,	"DEC",		"TGA"},
												{	0x1011,	0x0009,	"DEC",		"Tulip Fast"},
												{	0x1011,	0x000D,	"DEC",		"TGA 2"},
												{	0x1011,	0x000F,	"DEC",		"FDDI"},
												{	0x1011,	0x0014,	"DEC",		"Tulip+"},
												{	0x1011,	0x0019,	"DEC",		"21142"},
												{	0x1011,	0x0021,	"DEC",		"21052"},
												{	0x1011,	0x0022,	"DEC",		"21150"},
												{	0x1011,	0x0024,	"DEC",		"21152"},
												{	0x1011,	0x0025,	"DEC",		"21153"},
												{	0x1011,	0x0026,	"DEC",		"21154"},
												{	0x1011,	0x1065,	"DEC",		"21285"},
												{	0x1011,	0x0046,	"DEC",		"Compaq 42xx"},
												{	0x1011,	0,		"DEC",		"Unrecognized"},
												
												{	0x1013,	0x0038,	"Cirrus",		"7548"},
												{	0x1013,	0x00A0,	"Cirrus",		"5430"},
												{	0x1013,	0x00A4,	"Cirrus",		"5434 4"},
												{	0x1013,	0x00A8,	"Cirrus",		"5434 8"},
												{	0x1013,	0x00AC,	"Cirrus",		"5436"},
												{	0x1013,	0x00B8,	"Cirrus",		"5446"},
												{	0x1013,	0x00BC,	"Cirrus",		"5480"},
												{	0x1013,	0x00D0,	"Cirrus",		"5462"},
												{	0x1013,	0x00D4,	"Cirrus",		"5464"},
												{	0x1013,	0x00D6,	"Cirrus",		"5465"},
												{	0x1013,	0x1100,	"Cirrus",		"6729"},
												{	0x1013,	0x1110,	"Cirrus",		"6832"},
												{	0x1013,	0x1200,	"Cirrus",		"7542"},
												{	0x1013,	0x1202,	"Cirrus",		"7543"},
												{	0x1013,	0x1204,	"Cirrus",		"7541"},
												{	0x1013,	0,		"Cirrus",		"Unrecognized"},
												
												{	0x1014,	0x000A,	"IBM",		"Fire Coral"},
												{	0x1014,	0x0018,	"IBM",		"TR"},
												{	0x1014,	0x001D,	"IBM",		"82G2675"},
												{	0x1014,	0x0020,	"IBM",		"MCA"},
												{	0x1014,	0x0022,	"IBM",		"82351"},
												{	0x1014,	0x002D,	"IBM",		"Python"},
												{	0x1014,	0x002E,	"IBM",		"Server Aid"},
												{	0x1014,	0x003E,	"IBM",		"TR Wake"},
												{	0x1014,	0x0046,	"IBM",		"MPic"},
												{	0x1014,	0x007D,	"IBM",		"3780IDSP"},
												{	0x1014,	0xFFFF,	"IBM",		"MPic 2"},
												{	0x1014,	0,		"IBM",		"Unrecognized"},
												
												{	0x1045,	0xC178,	"Opti",		"92C178"},
												{	0x1045,	0xC557,	"Opti",		"82C557"},
												{	0x1045,	0xC558,	"Opti",		"82C558"},
												{	0x1045,	0xC621,	"Opti",		"82C621"},
												{	0x1045,	0xC700,	"Opti",		"82C700"},
												{	0x1045,	0xC701,	"Opti",		"82C701"},
												{	0x1045,	0xC814,	"Opti",		"82C814"},
												{	0x1045,	0xC822,	"Opti",		"82C822"},
												{	0x1045,	0xC861,	"Opti",		"82C861"},
												{	0x1045,	0xD568,	"Opti",		"82C825"},
												{	0x1045,	0,		"Opti",		"Unrecognized"},
												
												{	0x104C,	0x3D04,	"TI",			"TVP4010"},
												{	0x104C,	0x3D07,	"TI",			"TVP4020"},
												{	0x104C,	0xAC12,	"TI",			"PCI 1130"},
												{	0x104C,	0xAC13,	"TI",			"PCI 1031"},
												{	0x104C,	0xAC15,	"TI",			"PCI 1131"},
												{	0x104C,	0xAC16,	"TI",			"PCI 1250"},
												{	0x104C,	0xAC17,	"TI",			"PCI 1220"},
												{	0x104C,	0,		"TI",			"Unrecognized"},
												
												{	0x1057,	0x1507,	"Motorola",	"OOPS"},
												{	0x1057,	0x0001,	"Motorola",	"MPC105"},
												{	0x1057,	0x0002,	"Motorola",	"MPC106"},
												{	0x1057,	0x4801,	"Motorola",	"Raven"},
												{	0x1057,	0x4802,	"Motorola",	"Falcon"},
												{	0x1057,	0x4806,	"Motorola",	"CPX8216"},
												{	0x1057,	0,		"Motorola",	"Unrecognized"},
												
												{	0x106B,	0x0001,	"Apple",		"Bandit"},
												{	0x106B,	0x0002,	"Apple",		"Grand Central"},
												{	0x106B,	0x0003,	"Apple",		"Control"},
												{	0x106B,	0x0004,	"Apple",		"Plan B"},
												{	0x106B,	0x000E,	"Apple",		"Hydra"},
												{	0x106B,	0x0017,	"Apple",		"Paddington"},
												{	0x106B,	0,		"Apple",		"Unrecognized"},
												
												{	0x1095,	0x0640,	"CMD",		"640"},
												{	0x1095,	0x0643,	"CMD",		"643"},
												{	0x1095,	0x0646,	"CMD",		"646"},
												{	0x1095,	0x0647,	"CMD",		"647"},
												{	0x1095,	0x0670,	"CMD",		"670"},
												{	0x1095,	0,		"CMD",		"Unrecognized"}
											};

ConstASCII8Str PCIDevice::vendorName()
{
	UInt16	vendorID = readVendorID();
	
	for(UInt32 i=0;i<sizeof(pciVendorDeviceTable)/sizeof(pciVendorDeviceTable[0]);i++)
	{
		if(vendorID == pciVendorDeviceTable[i].vendorID)
			return pciVendorDeviceTable[i].vendorName;
	}
	
	return nil;
}

ConstASCII8Str PCIDevice::deviceName()
{
	UInt16	vendorID = readVendorID();
	UInt16	deviceID = readDeviceID();
	
	for(UInt32 i=0;i<sizeof(pciVendorDeviceTable)/sizeof(pciVendorDeviceTable[0]);i++)
	{
		if(vendorID == pciVendorDeviceTable[i].vendorID && deviceID == pciVendorDeviceTable[i].deviceID)
			return pciVendorDeviceTable[i].deviceName;
	}
	
	return nil;
}

typedef struct PCIClassTable
{
	UInt16			classID;
	UInt16			mask;
	ConstASCII8Str		className;
}PCIClassTable;
PCIClassTable	pciClassTable[]	=	{	{0x0000,	0xFFFF,	"Undefined"},
								{0x0001,	0xFFFF,	"Undefined VGA"},
								
								{0x0100,	0xFFFF,	"SCSI storage"},
								{0x0101,	0xFFFF,	"IDE storage"},
								{0x0102,	0xFFFF,	"Floppy storage"},
								{0x0103,	0xFFFF,	"IPI storage"},
								{0x0104,	0xFFFF,	"RAID storage"},
								{0x0180, 0xFFFF,	"Other storage"},
								{0x0100,	0xFF00,	"Unrecognized storage"},
								
								{0x0200,	0xFFFF,	"Ethernet"},
								{0x0201,	0xFFFF,	"Token ring"},
								{0x0202,	0xFFFF,	"FDDI"},
								{0x0203,	0xFFFF,	"ATM"},
								{0x0280,	0xFFFF,	"Other network"},
								{0x0200,	0xFF00,	"Unrecognized network"},
								
								{0x0300,	0xFFFF,	"VGA display"},
								{0x0301,	0xFFFF,	"XGA display"},
								{0x0380,	0xFFFF,	"Other display"},
								{0x0300,	0xFF00,	"Unrecognized display"},
								
								{0x0400,	0xFFFF,	"Multimedia video"},
								{0x0401,	0xFFFF,	"Multimedia audio"},
								{0x0480,	0xFFFF,	"Other multimedia"},
								{0x0400,	0xFF00,	"Unrecognized multimedia"},
								
								{0x0500,	0xFFFF,	"RAM memory"},
								{0x0501,	0xFFFF,	"Flash memory"},
								{0x0580,	0xFFFF,	"Other memory"},
								{0x0500,	0xFF00,	"Unrecognized memory"},
								
								{0x0600,	0xFFFF,	"Host bridge"},
								{0x0601,	0xFFFF,	"ISA bridge"},
								{0x0602,	0xFFFF,	"EISA bridge"},
								{0x0603,	0xFFFF,	"MC bridge"},
								{0x0604,	0xFFFF,	"PCI bridge"},
								{0x0605,	0xFFFF,	"PCMCIA bridge"},
								{0x0606,	0xFFFF,	"Nubus bridge"},
								{0x0607,	0xFFFF,	"Cardbus bridge"},
								{0x0680,	0xFFFF,	"Other bridge"},
								{0x0600,	0xFF00,	"Unrecognized bridge"},
								
								{0x0700,	0xFFFF,	"Serial communication"},
								{0x0701,	0xFFFF,	"Parallel communication"},
								{0x0780,	0xFFFF,	"Other communication"},
								{0x0700,	0xFF00,	"Unrecognized communication"},
								
								{0x0800,	0xFFFF,	"PIC system"},
								{0x0801,	0xFFFF,	"DMA system"},
								{0x0802,	0xFFFF,	"Timer system"},
								{0x0803,	0xFFFF,	"RTC system"},
								{0x0880,	0xFFFF,	"Other system"},
								{0x0800,	0xFF00,	"Unrecognized system"},
								
								{0x0900,	0xFFFF,	"Keyboard input"},
								{0x0901,	0xFFFF,	"Pen input"},
								{0x0902,	0xFFFF,	"Mouse input"},
								{0x0980,	0xFFFF,	"Other input"},
								{0x0900,	0xFF00,	"Unrecognized input"},
								
								{0x0A00,	0xFFFF,	"Generic docking"},
								{0x0A01,	0xFFFF,	"Other docking"},
								{0x0A00,	0xFF00,	"Unrecognized docking"},
								
								{0x0B00,	0xFFFF,	"386 processor"},
								{0x0B01,	0xFFFF,	"486 processor"},
								{0x0B02,	0xFFFF,	"Pentium processor"},
								{0x0B10,	0xFFFF,	"Alpha processor"},
								{0x0B20,	0xFFFF,	"PowerPC processor"},
								{0x0B40,	0xFFFF,	"Co-processor"},
								{0x0B00,	0xFF00,	"Unrecognized processor"},
								
								{0x0C00,	0xFFFF,	"Firewire serial"},
								{0x0C01,	0xFFFF,	"Access serial"},
								{0x0C02,	0xFFFF,	"SSA serial"},
								{0x0C03,	0xFFFF,	"USB serial"},
								{0x0C04,	0xFFFF,	"Fiber serial"},
								{0x0C00,	0xFF00,	"Unrecognized serial"},
								
								{0x0E00,	0xFFFF,	"Intelligent I20"},
								{0x0E00,	0xFF00,	"Unrecognized intelligent"},
								
								{0xFF00,	0xFFFF,	"Apple I/O"},
								{0xFF00,	0xFF00,	"Hot swap controller"}
							};

ConstASCII8Str PCIDevice::className()
{
	UInt16	classID = readClass();
	
	for(UInt32 i=0;i<sizeof(pciClassTable)/sizeof(pciClassTable[0]);i++)
	{
		if((classID & pciClassTable[i].mask) == pciClassTable[i].classID)
			return pciClassTable[i].className;
	}
	
	return nil;
}

UInt32 PCIDevice::busID()
{
	return mySlot->myBus->busID;
}

UInt32 PCIDevice::slotID()
{
	return mySlot->slotID;
}

UInt32 PCIDevice::funcID()
{
	return func;
}

Ptr PCIDevice::mapBaseAddr(UInt8 n)
{
	FatalAssert(n <= 5);
	if(baseAddr[n] & 1)	// I/O space
		return (Ptr)((baseAddr[n] & 0xFFFFFFFE) + mySlot->myBus->driver->ioBase);
	else // Memory space, not mapped yet
	{
		// This could probably be mapped as cacheable non-guarded, but better safe than sorry for now
		return (Ptr)NKIOMap((void*)baseAddr[n],baseLen[n],WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	}
}

UInt32 PCIDevice::getBaseAddr(UInt8 n)
{
	FatalAssert(n <= 5);
	return baseAddr[n];
}

UInt32 PCIDevice::getBaseLen(UInt8 n)
{
	FatalAssert(n <= 5);
	return baseLen[n];
}