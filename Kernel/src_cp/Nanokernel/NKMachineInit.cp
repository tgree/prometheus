/*
	NKMachineInit.cp
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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Monday, 3 August 98	-	Removed timing stuff, see Time.cp
	Terry Greeniaus	-	Wednesday, 26 Oct 99	-	Change IDE stuff to check device_type property, rather than name property
	Patrick Varilly		-	Wednesday, 3 Nov 99	-	Added USB device to Machine structure (and in OF Tree)
*/
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "Machine Registers.h"
#include "VIA Chip.h"
#include "PDM Interrupts.h"
#include "Mesh.h"
#include "ASC.h"
#include "Mace.h"
#include "MACE DMA.h"
#include "OpenFirmware.h"
#include "ANSI.h"
#include "BootInfo.h"
#include "DMA.h"
#include "ESCC.h"
#include "IBM IDE Driver.h"
#include "SWIM3.h"

MachineInfo		machine;
static ConstASCII8Str	machineName[] = {	"Unknown PowerMacintosh",
									"PowerMac 6100",
									"PowerMac 7100",
									"PowerMac 8100",
									"PowerMac 7200",
									"PowerMac 7300",
									"PowerMac 7500",
									"PowerMac 7600",
									"PowerMac 8500",
									"PowerMac 9500",
									"PowerBook 3400",
									"New World",
									"PowerMac G3",
									"PowerBook G3",
									"Performa 4400",
									"Performa 5200",
									"Performa 5400",
									"Performa 5500",
									"Performa 6200",
									"Performa 6400",
									"Performa 6500",
									"20th Anniversary Mac",
									"PowerBook Duo 2300",
									"PowerBook 5300",
									"Apple Workshare Server 9150",
									"PowerBook 1400",
									"PowerBook 2300",
									"PowerBook 2400"};

void NKMachineInit(void)
{
	if(machine.busClock == 0)
		machine.busClock = 40000000;	// Default is 40 MHz if there is no busClock variable
	
	machine.openFirmwareFlattenedTree = (Int8*)((UInt32)machine.kernelPreStack - machine.openFirmwareFlattenedLen - sizeof(BootInfo));
	
	switch(machine.gestaltMachineType)
	{
		// PDM Machines
		case 75:	// PowerMac 6100_60
		case 100:	// PowerMac 6100_66
		case 101:	// PowerMac 6100_80 (not released)
			machine.kernelMachineType = machine6100;
		break;
		case 47:	// PowerMac 7100_80
		case 112:	// PowerMac 7100_66
		case 111:	// PowerMac 7100_60 (not released)
		case 113:	// PowerMac 7100_80 with pre-rel ROM
		case 114:	// PowerMac 7100_82 (not released)
			machine.kernelMachineType = machine7100;
		break;
		case 40:	// PowerMac 8100_110
		case 55:	// PowerMac 8100_100
		case 61:	// PowerMac 8100_80 at 60 MHz
		case 64:	// PowerMac 8100_60/80 at 66.6 MHz
		case 65:	// PowerMac 8100_80
		case 66:	// PowerMac 8100_100 with pre-rel ROM
			machine.kernelMachineType = machine8100;
		break;
		
		// PCI Machines
		case 108:	// PowerMac 7200
			machine.kernelMachineType = machine7200;
		break;
		case 109:	// PowerMac 7300
			machine.kernelMachineType = machine7300;
		break;
		case 68:	// PowerMac 7500
			machine.kernelMachineType = machine7500;
		break;
		case 110:	// PowerMac 7600 ???
			machine.kernelMachineType = machine7600;
		break;
		case 69:	// PowerMac 8500
			machine.kernelMachineType = machine8500;
		break;
		case 67:	// PowerMac 9500
			machine.kernelMachineType = machine9500;
		break;
		case 306:	// PowerBook 3400
			machine.kernelMachineType = machinePB3400;
		break;
		
		// Performas
		case 515:	// PowerMac 4400
		case 514:	// With a different floppy drive or something
			machine.kernelMachineType = machine4400;
		break;
		case 41:	// PowerMac 5200
			machine.kernelMachineType = machine5200;
		break;
		case 74:	// PowerMac 5400
			machine.kernelMachineType = machine5400;
		break;
		case 512:	// PowerMac 5500 or 20th Anniv Mac
			machine.kernelMachineType = machine5500;
		break;
		case 42:	// PowerMac 6200
			machine.kernelMachineType = machine6200;
		break;
		case 58:	// PowerMac 6400
			machine.kernelMachineType = machine6400;
		break;
		case 513:	// PowerMac 6500
			machine.kernelMachineType = machine6500;
		break;
		case (0x80000000 | 512):	// 20th Anniv Mac [Is this a Performa machine??]
			machine.kernelMachineType = machine20thAnniv;
		break;
		
		// Duos
		case 124:	// PowerBook Duo 2300
			machine.kernelMachineType = machineDuo2300;
		break;
		
		// PowerBooks
		case 310:	// PowerBook 1400
		case 311:	// PowerBook Mustang??  Seen on 1400
			machine.kernelMachineType = machinePB1400;
		break;
		case 125:	// PowerBook 2300 (not released?)
			machine.kernelMachineType = machinePB2300;
		break;
		case 307:	// PowerBook 2400
			machine.kernelMachineType = machinePB2400;
		break;
		case 128:	// PowerBook 5300
		case 129:	// PowerBook 5300 (as reported by Apple System Profiler 1.3.1)
			machine.kernelMachineType = machinePB5300;
		break;
		
		// G3 Machines
		case 406:	// iMac
			machine.kernelMachineType = machineNewWorld;
		break;
		case 312:	// PowerBook G3
		case 313:
		case 314:
			machine.kernelMachineType = machinePBG3;
		break;
		case 510:	// PowerMac G3
			machine.kernelMachineType = machineDesktopG3;
		break;
		
		// Apple Workshare Servers!
		case 39:	// AWS 9150_80
		case 57:	// AWS 9150_120
			machine.kernelMachineType = machineAWS9150;
		break;
		
		default:
			machine.kernelMachineType = machineUnknown;
		break;
	}
	
	// Set up the machine name
	machine.machineName = machineName[machine.kernelMachineType];
	
	// Set up the machine class.
	switch(machine.kernelMachineType)
	{
		// PDM machines are Nubus based
		case machine8100:
		case machineAWS9150:
		case machine6100:
		case machine7100:
			machine.machineClass = classPDM;
		break;
		
		// PCI machines have a PCI motherboard layout, even if they don't directly support PCI cards.
		case machine7300:
		case machine7500:
		case machine7600:
		case machine8500:
		case machine9500:
		case machine7200:
		case machine6500:
			machine.machineClass = classPCI;
			//machine.sccAddr = (void*)0xF3013000;
		break;
		
		// Performa machines are just weird.
		case machine4400:
		case machine5200:
		case machine5400:
		case machine5500:
		case machine6200:
		case machine6400:
		case machine20thAnniv:	// Is this a performa???
			machine.machineClass = classPerforma;
		break;
		
		// Duos
		case machineDuo2300:
			machine.machineClass = classPowerBook;
		break;
		
		// PowerBooks aren't always PowerBooks - ex: 3400c is a PCI machine... oh well.
		case machinePB3400:
			machine.machineClass = classPowerBookPCI;
		break;
		case machinePB1400:
		case machinePB2300:
		case machinePB5300:
			machine.machineClass = classPowerBook;
		break;
		
		// G3 machines
		case machineDesktopG3:
		case machinePBG3:
			machine.machineClass = classG3;
		break;
		
		// New world
		case machineNewWorld:
			machine.machineClass = classNewWorld;
		break;
	}
	
	switch(machine.machineClass)
	{
		case classPDM:
		case classPerforma:
		case classPowerBook:
			machine.interruptControllerWidth = 0;
		break;
		case classPCI:
		case classPowerBookPCI:
			machine.interruptControllerWidth = 32;
		break;
		case classG3:
		case classNewWorld:
			machine.interruptControllerWidth = 64;
		break;
	}
}

void NKMachineOpenFirmwareInit()
{
	// IO addresses
	OpenFirmwareNode*	esccNode =		machine.openFirmwareTree->getNode("escc");
	OpenFirmwareNode*	viaCudaNode =		machine.openFirmwareTree->getNode("via-cuda");
	OpenFirmwareNode*	viaPMUNode =		machine.openFirmwareTree->getNode("via-pmu");
	OpenFirmwareNode*	via2Node =		machine.openFirmwareTree->getNode("via2");
	OpenFirmwareNode*	scsi53c94Node = 	machine.openFirmwareTree->getNode("53c94");
	OpenFirmwareNode*	scsi53cf94Node =	machine.openFirmwareTree->getNode("53cf94");
	OpenFirmwareNode*	maceNode =		machine.openFirmwareTree->getNode("mace");
	OpenFirmwareNode*	swim3Node =		machine.openFirmwareTree->getNode("swim3");
	if(!swim3Node)
		swim3Node = machine.openFirmwareTree->getNode("floppy");
	if(!swim3Node)
		swim3Node = machine.openFirmwareTree->getNode("fdc");
	OpenFirmwareNode*	meshNode =		machine.openFirmwareTree->getNode("mesh");
	if(!meshNode)
		meshNode = machine.openFirmwareTree->getNode("scsi");
	OpenFirmwareNode*	ide0Node =		machine.openFirmwareTree->getNodeByDevType("ata");
	if(!ide0Node)
		ide0Node = machine.openFirmwareTree->getNodeByDevType("ide");
	
	if(esccNode)
	{
		OpenFirmwareDeviceNode*		esccADeviceNode = static_cast<OpenFirmwareDeviceNode*>(machine.openFirmwareTree->getNode("ch-a"));
		OpenFirmwareDeviceNode*		esccBDeviceNode = static_cast<OpenFirmwareDeviceNode*>(machine.openFirmwareTree->getNode("ch-b"));
		
		if(esccADeviceNode)
		{
			MachineDevice<struct esccChannel>	esccAChannel = {(void*)esccADeviceNode->address(0),nil,sizeof(esccChannel),{esccADeviceNode->interrupt(0),0},channelA};
			machine.esccAChannel = esccAChannel;
		}
		
		if(esccBDeviceNode)
		{
			MachineDevice<struct esccChannel>	esccBChannel = {(void*)esccBDeviceNode->address(0),nil,sizeof(esccChannel),{esccBDeviceNode->interrupt(0),0},channelB};
			machine.esccBChannel = esccBChannel;
		}
		
		MachineDevice<struct esccDevice>	esccDevice = {(void*)esccBDeviceNode->address(0),nil,sizeof(esccDevice),{0,0}};
		machine.esccDevice = esccDevice;
	}
	if(viaCudaNode)
	{
		OpenFirmwareDeviceNode*		viaCudaDeviceNode = static_cast<OpenFirmwareDeviceNode*>(viaCudaNode);
		MachineDevice<struct VIA_Chip>	viaDevice = {(void*)viaCudaDeviceNode->address(0),nil,sizeof(VIA_Chip),{viaCudaDeviceNode->interrupt(0),viaCudaDeviceNode->interrupt(1)}};
		machine.viaDevice0 = viaDevice;
		machine.cudaDevice = viaDevice;
		machine.cudaDevice.interrupts[0] = PMAC_DEV_CUDA;
	}
	if(viaPMUNode)
	{
		OpenFirmwareDeviceNode*		viaPMUDeviceNode = static_cast<OpenFirmwareDeviceNode*>(viaPMUNode);
		MachineDevice<struct VIA_Chip>	viaDevice = {(void*)viaPMUDeviceNode->address(0),nil,sizeof(VIA_Chip),{viaPMUDeviceNode->interrupt(0),viaPMUDeviceNode->interrupt(1)}};
		machine.viaDevice0 = viaDevice;
		machine.pmuDevice = viaDevice;
		machine.pmuDevice.interrupts[0] = PMAC_DEV_PMU;
	}
	if(via2Node)
	{
		OpenFirmwareDeviceNode*		via2DeviceNode = static_cast<OpenFirmwareDeviceNode*>(via2Node);
		MachineDevice<struct VIA2_Chip>	via2Device = {(void*)via2DeviceNode->address(0),nil,sizeof(VIA2_Chip),{via2DeviceNode->interrupt(0),via2DeviceNode->interrupt(1)}};
		machine.viaDevice1 = via2Device;
	}
	if(scsi53c94Node)
	{
		OpenFirmwareDeviceNode*		scsi53c94DeviceNode = static_cast<OpenFirmwareDeviceNode*>(scsi53c94Node);
		MachineDevice<struct asc_curio_regmap>	scsi53c94Device = {(void*)scsi53c94DeviceNode->address(0),nil,sizeof(asc_curio_regmap),{scsi53c94DeviceNode->interrupt(0),scsi53c94DeviceNode->interrupt(1)},slowASC};
		machine.slowASCDevice = scsi53c94Device;
	}
	if(scsi53cf94Node)
	{
		OpenFirmwareDeviceNode*		scsi53cf94DeviceNode = static_cast<OpenFirmwareDeviceNode*>(scsi53cf94Node);
		MachineDevice<struct asc_curio_regmap>	scsi53cf94Device = {(void*)scsi53cf94DeviceNode->address(0),nil,sizeof(asc_curio_regmap),{scsi53cf94DeviceNode->interrupt(0),scsi53cf94DeviceNode->interrupt(1)},fastASC};
		machine.fastASCDevice = scsi53cf94Device;
	}
	if(swim3Node)
	{
		OpenFirmwareDeviceNode*		swim3DeviceNode = static_cast<OpenFirmwareDeviceNode*>(swim3Node);
		MachineDevice<struct SWIM3Regs> swim3Device = {(void*)swim3DeviceNode->address(0),nil,sizeof(SWIM3Regs),{swim3DeviceNode->interrupt(0),swim3DeviceNode->interrupt(1)}};
		machine.swim3 = swim3Device;
	}
	if(maceNode)
	{
		OpenFirmwareDeviceNode*				maceDeviceNode = static_cast<OpenFirmwareDeviceNode*>(maceNode);
		MachineDevice<struct mace_board>			maceDevice = {(void*)maceDeviceNode->address(0),nil,sizeof(mace_board),{maceDeviceNode->interrupt(0),maceDeviceNode->interrupt(1)}};
		MachineDevice<struct PDMMACEDMARegs>	maceDMA = {(void*)maceDeviceNode->address(1),nil,sizeof(PDMMACEDMARegs),{maceDeviceNode->interrupt(1),maceDeviceNode->interrupt(2)}};
		machine.maceDevice = maceDevice;
		machine.maceDMA = maceDMA;
	}
	if(meshNode)
	{
		OpenFirmwareDeviceNode*			meshDeviceNode = static_cast<OpenFirmwareDeviceNode*>(meshNode);
		MachineDevice<struct mesh_regmap>	meshDevice = {(void*)meshDeviceNode->address(0),nil,sizeof(mesh_regmap),{meshDeviceNode->interrupt(0),meshDeviceNode->interrupt(1)}};
		machine.meshDevice = meshDevice;
	}
	if(ide0Node)
	{
		OpenFirmwareDeviceNode*		ide0DeviceNode = static_cast<OpenFirmwareDeviceNode*>(ide0Node);
		if(strcmp(ide0Node->parentNode()->name(),"media-bay"))
		{
			MachineDevice<struct IDERegsIBM>	ide0Device = {(void*)ide0DeviceNode->address(0),nil,sizeof(IDERegsIBM),{ide0DeviceNode->interrupt(0),ide0DeviceNode->interrupt(1)}};
			machine.ide0Chip = ide0Device;
		}
	}
}