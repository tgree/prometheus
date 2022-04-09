#include "PCIBanditDevice.h"
#include "NKMachineInit.h"
#include "Compiler.h"
#include "Kernel Console.h"
#include "Time.h"

void InitBandit()
{
	UInt32			numBandits = 0;
	PCIBridgeDevice*	theBridge;
	OpenFirmwareNode*	banditNode = machine.openFirmwareTree->getNode("bandit");
	while(banditNode)
	{
		numBandits++;
		cout << "\nConstructing Bandit chip " << numBandits << "\n";
		theBridge = new PCIBanditDevice(static_cast<OpenFirmwarePCINode*>(banditNode));
		theBridge->probe();
		banditNode = machine.openFirmwareTree->getNextNode("bandit",banditNode);
	}
	banditNode = machine.openFirmwareTree->getNode("chaos");
	numBandits = 0;
	while(banditNode)
	{
		numBandits++;
		cout << "\nConstructing Chaos chip " << numBandits << "\n";
		theBridge = new PCIBanditDevice(static_cast<OpenFirmwarePCINode*>(banditNode));
		theBridge->probe();
		banditNode = machine.openFirmwareTree->getNextNode("chaos",banditNode);
	}
}

PCIBanditDevice::PCIBanditDevice(OpenFirmwarePCINode* banditDevice):
	PCIBridgeDevice(banditDevice,
		(UReg32BE*)(banditDevice->reg(0) + 0x00800000),
		(UReg8*)(banditDevice->reg(0) + 0x00C00000),
		(Ptr)banditDevice->reg(0),0x00010000,
		banditDevice->busRange(0),
		banditDevice->busRange(1)
		)
{
}

PCIBanditDevice::~PCIBanditDevice()
{
}

UInt8 PCIBanditDevice::readConfig8(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 0x07) << 8) | (offset & 0xFFFFFFFC),configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC) | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	return ReadUReg8(configData + (offset & 0x00000003));
}

UInt16 PCIBanditDevice::readConfig16(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000001) == 0);
	
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 0x07) << 8) | (offset & 0xFFFFFFFC),configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC) | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	return ReadUReg16LE((UReg16LE*)(configData + (offset & 0x00000003)));
}

UInt32 PCIBanditDevice::readConfig32(UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000003) == 0);
	
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 7) << 8) | offset,configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | offset | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	return ReadUReg32LE((UReg32LE*)configData);
}

void PCIBanditDevice::writeConfig8(UInt8 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 0x07) << 8) | (offset & 0xFFFFFFFC),configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC) | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	WriteUReg8(data,configData + (offset & 0x00000003));
}

void PCIBanditDevice::writeConfig16(UInt16 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000001) == 0);
	
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 0x07) << 8) | (offset & 0xFFFFFFFC),configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | (offset & 0xFFFFFFFC) | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	WriteUReg16LE(data,(UReg16LE*)(configData + (offset & 0x00000003)));
}

void PCIBanditDevice::writeConfig32(UInt32 data,UInt8 theBus,UInt8 devFN,UInt8 offset)
{
	FatalAssert((offset & 0x00000003) == 0);
	
	if(theBus == busNumber)
	{
		FatalAssert((devFN >> 3) >= 11);
		WriteUReg32LE((1UL << (devFN >> 3)) | ((devFN & 7) << 8) | offset,configAddr);
	}
	else
		WriteUReg32LE((theBus << 16) | (devFN << 8) | offset | 1,configAddr);
	//Wait_us(2);
	ReadUReg32LE(configAddr);
	
	WriteUReg32LE(data,(UReg32LE*)configData);
}

UInt8 PCIBanditDevice::firstSlot()
{
	return 11;
}

UInt8 PCIBanditDevice::lastSlot()
{
	return 31;
}