#include "PCIGrackleDevice.h"
#include "NKMachineInit.h"
#include "Compiler.h"
#include "Kernel Console.h"
#include "NKInterruptVectors.h"

// To select between different address maps.  On my B&W, it comes configured for address map B.  I have been unable
// to find a way to determine the current address map programmatically, so we assume map B for now.
#define	ADDRESS_MAP_A	1
#define	ADDRESS_MAP_B	2
#define	USE_ADDRESS_MAP	ADDRESS_MAP_B
#if (USE_ADDRESS_MAP != ADDRESS_MAP_A) && (USE_ADDRESS_MAP != ADDRESS_MAP_B)
#error Don't now which MPC106 address map to use!!!
#endif

// This will catch machine check exceptions from reads/writes to the MPC106 and print useful information
#define	CATCH_MACHINE_CHECK	1

static PCIGrackleDevice*	grackleBridge = nil;
static UInt32			grackleConfigAddr;

void InitGrackle()
{
	UInt32			numGrackles = 0;
	OpenFirmwareNode*	grackleNode = machine.openFirmwareTree->getNode("pci");
	while(grackleNode)
	{
		numGrackles++;
		cout << "\nConstructing Grackle chip " << numGrackles << "\n";
		grackleBridge = new PCIGrackleDevice(static_cast<OpenFirmwarePCINode*>(grackleNode));
		grackleBridge->probe();
		grackleNode = machine.openFirmwareTree->getNextNode("pci",grackleNode);
	}
}

PCIGrackleDevice::PCIGrackleDevice(OpenFirmwarePCINode* grackleDevice):
#if USE_ADDRESS_MAP == ADDRESS_MAP_A
	PCIBridgeDevice(grackleDevice,
		(UReg32LE*)0x80000CF8,	// CONFIG_ADDR is the word at 0x80000CF8
		(UReg8*)0x80000CFC,		// CONFIG_DATA is the word at 0x80000CFC
		(Ptr)0x80000000,			// IO space goes from 0x80000000-0x80800000...
		0x00020000,				// ...but we only map it from 0x80000000-0x80020000, because that's what LinuxPPC would do
		grackleDevice->busRange(0),
		grackleDevice->busRange(1)
#elif USE_ADDRESS_MAP == ADDRESS_MAP_B
	PCIBridgeDevice(grackleDevice,
		(UReg32LE*)0xFEC00000,	// CONFIG_ADDR is any word-aligned word in 0xFEC00000-0xFEE00000
		(UReg8*)0xFEE00000,		// CONFIG_DATA is any word-aligned word in 0xFEE00000-0xFEF00000
		(Ptr)0xFE000000,			// IO space goes from 0xFE000000-0xFE800000...
		0x00020000,				// ...but we only map it from 0xFE000000-0xFE020000, because that's what LinuxPPC does
		grackleDevice->busRange(0),
		grackleDevice->busRange(1)
#endif
	)
{
	// Disable master abort reporting on the various buses.  This allows us to probe empty slots without getting
	// machine check exceptions...
	// Bus 1 (PCI Bridge) - slot 13
	UInt8 controlReg = readConfig16(0,(13 << 3),0x3E);
	controlReg &= ~0x0020;
	writeConfig16(controlReg,0,(13 << 3),0x3E);
	readConfig16(0,0x68,0x3E);
	
	// Bus 0 (Grackle) - slot 0
	UInt8 errEnR1 = readConfig8(0,0,0xC0);
	errEnR1 &= ~0x02;
	writeConfig8(errEnR1,0,0,0xC0);
	readConfig8(0,0,0xC0);
	
	cout << "Done.\n";
}

PCIGrackleDevice::~PCIGrackleDevice()
{
}

// To access a PCI configuration register, you do the following:
//
//	0x80000000	-	base address of the registers
//	0x00nn0000	-	nn is the bus number to access
//	0x0000nn00	-	nn is the devFN to access
//	0x000000nn	-	nn is the configuration register number (offset)
//
UInt8 PCIGrackleDevice::readConfig8(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	UInt32 retVal = ReadUReg8(configData + (offset & 0x00000003));
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
	return retVal;
}

UInt16 PCIGrackleDevice::readConfig16(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000001) == 0);
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	UInt32 retVal = ReadUReg16LE((UReg16LE*)(configData + (offset & 0x00000003)));
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
	return retVal;
}

UInt32 PCIGrackleDevice::readConfig32(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000003) == 0);
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	UInt32 retVal = ReadUReg32LE((UReg32LE*)configData);
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
	return retVal;
}

void PCIGrackleDevice::writeConfig8(UInt8 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	WriteUReg8(data,configData + (offset & 0x00000003));
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
}

void PCIGrackleDevice::writeConfig16(UInt16 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000001) == 0);
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	WriteUReg16LE(data,(UReg16LE*)(configData + (offset & 0x00000003)));
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
}

void PCIGrackleDevice::writeConfig32(UInt32 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000003) == 0);
#if CATCH_MACHINE_CHECK
	NKExceptionHandler prevHandler = NKInstallExceptionHandler(PCIGrackleMachineCheckHandler,machineCheckException);
#endif
	grackleConfigAddr = 0x80000000 | (theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC);
	WriteUReg32LE(grackleConfigAddr,configAddr);
	WriteUReg32LE(data,(UReg32LE*)configData);
#if CATCH_MACHINE_CHECK
	NKInstallExceptionHandler(prevHandler,machineCheckException);
#endif
}

UInt8 PCIGrackleDevice::firstSlot()
{
	return 0;
}

UInt8 PCIGrackleDevice::lastSlot()
{
	return 30;
}

Boolean PCIGrackleDevice::PCIGrackleMachineCheckHandler(PPCRegisters*)
{
	cout << redMsg << hexMsg << "Grackle machine check exception\n";
	cout << "\tWas accessing config register " << grackleConfigAddr << "\n";
	cout << "\tPCI Command Register = " << grackleBridge->readConfig16(0,0,0x04) << "\n";
	cout << "\tPCI Status Register = " << grackleBridge->readConfig8(0,0,0x06) << "\n";
	cout << "\tErrDR1 = " << grackleBridge->readConfig8(0,0,0xC1) << "\n";
	cout << "\tErrDR2 = " << grackleBridge->readConfig8(0,0,0xC5) << "\n";
	cout << "\t60x Bus Error Status register = " << grackleBridge->readConfig8(0,0,0xC3) << "\n";
	cout << "\tPCI Bus Error Status register = " << grackleBridge->readConfig8(0,0,0xC7) << "\n";
	cout << "\t60x/PCI Error Address register = " << grackleBridge->readConfig32(0,0,0xC8) << "\n";
	return false;
}
