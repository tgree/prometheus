/*
	NKMachineInit.h
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
	Terry Greeniaus	-	Friday, 26 June 98	-	Added interrupts[2] field to MachineDevice
*/
#ifndef __NK_MACHINEINIT__
#define __NK_MACHINEINIT__

#include "NKAtomicOperations.h"	// For NKInterruptSafeList<>
#include "Video Driver.h"	// For struct VideoParams

template<class T>
struct MachineDevice
{
	void*		physicalAddr;		// Physical address of the device - never actually access through this, though - need certain memory settings only available through address translation.
	T*			logicalAddr;		// A pointer to the actual device - with address translation on.
	UInt32		len;				// The length of the device's registers
	Int32		interrupts[3];		// OpenFirmware interrupt numbers
	UInt32		otherInfo;			// Dependant on each device
};

typedef struct KernelDirectoryInfo
{
	UInt32	dirID;		// HFS dirID of the kernel
	UInt32	checkSum;	// Checksum for the MDB (sector 2) of the kernel booter's device
}KernelDirectoryInfo;

struct ClockToNSRatio
{
	Float64	clocksToNS;	// Multiply clocks by this to get ns
	Float64	nsToClocks;	// Multiply ns by this to get clocks
};

typedef struct MachineInfo
{
	// These are commented out - they belong in the ProcessorInfo struct
	//ConstASCII8Str				processorName;			// The name of the type of PPC processor
	//UInt32					pvr;						// The contents of the processor version register (note: it's usually faster to directly read the PVR then look it up in this structure)
	UInt32					gestaltMachineType;			// The machine type as returned by Gestalt()
	UInt32					kernelMachineType;			// The translated machine type
	ConstASCII8Str				machineName;				// The name of this machine
	UInt32					machineClass;				// PDM or PCI
	UInt32					busClock;					// The bus clock speed as returned by Gestalt(), later calculated by InitTime()
	UInt32					memSize;					// The physical memory size as returned by Gestalt()
	void*					kernelStart;				// The start of the kernel image in memory
	void*					kernelPreStack;			// The end of all kernel stuff, before the stack
	void*					kernelEnd;					// The first free address in physical memory after the kernel image/stack
	void*					freeMemStart;				// The address of the start of contiguous free memory (above the page table and all kernel stuff)
	void*					stackBase;				// The bottom of the stack
	void*					codeStart;				// The start of kernel code in memory
	void*					dataStart;					// The start of kernel data/stack in memory
	struct PEFLoaderInfoHeader*	pefLoaderInfo;				// The PEF loader info header for the kernel PEF image
	
	// This belongs in ProcessorInfo too, but I haven't gotten it there yet.
	ClockToNSRatio				clockRatio;				// The ratio that you need to GetTime() multiply clocks by to get nanoseconds
	
	VideoParams				videoParams;				// The current video settings
	struct CodeFragment*		pefFragment;				// The PEF fragment for the kernel
	KernelDirectoryInfo			dirInfo;					// The location of the kernel executable currently booting
	UInt32					openFirmwareFlattenedLen;	// The length of the flattened OpenFirmware tree
	Int8*					openFirmwareFlattenedTree;	// The location of the flattened OpenFirmware tree
	class OpenFirmwareTree*		openFirmwareTree;			// The actual OpenFirmwareTree object
	UInt32					numProcessors;			// The number of processors
	void*					sccAddr;					// Address of the scc to use for debugging purposes (set up in NKMachineInit)
	Boolean					sccInUseForDebugger;		// True if scc is in use for the debugger (i.e. don't reinit in microkernel)
	
	// ******************* File Systems ********************
	class FileSystem*			fileSystems;		// A list of all the file systems on the machine
	
	// *************** External Interrupt Controller **************
	UInt32					interruptControllerWidth;	// 0 on PDM, 32 on PCI, 64 on G3 onwards
	
	// ***************** Serial Driver Stuff *****************
	// The ESCC (serial) device
	MachineDevice<struct esccDevice>		esccDevice;
	MachineDevice<struct esccChannel>		esccAChannel;
	MachineDevice<struct esccChannel>		esccBChannel;
	
	// ***************** VIA/PMU/CUDA Stuff ***************
	// The Cuda chip (same as VIA, actually)
	MachineDevice<struct VIA_Chip>		cudaDevice;
	
	// The PMU chip (same as VIA, actually - I think)
	MachineDevice<struct VIA_Chip>		pmuDevice;
	
	// The VIA chip(s)
	UInt32							numViaChips;
	MachineDevice<struct VIA_Chip>		viaDevice0;
	MachineDevice<struct VIA2_Chip>		viaDevice1;
	
	// ****************** SCSI Stuff *********************
	// The Mesh chip
	MachineDevice<struct mesh_regmap>	meshDevice;		// Present only on PCI machines with a fast internal Mesh bus
	
	// The ASC chips
	MachineDevice<struct asc_curio_regmap>	fastASCDevice;		// Present only on PDM machines with a fast internal bus
	MachineDevice<struct asc_curio_regmap>	slowASCDevice;	// Present on all machines.
	
	// Internal bus SCSI devices
	class SCSIBus*						scsiBusses;
	
	// ********************** IDE **********************
	// The IDE chips
	MachineDevice<struct IDERegsIBM>		ide0Chip;		// IDE chip 0
	
	// The IDE devices
	class IDEBus*						ideBusses;
	
	// ********************** SWIM3 ********************
	MachineDevice<struct SWIM3Regs>		swim3;
	
	// ****************** Ethernet Stuff *******************
	MachineDevice<struct mace_board>			maceDevice;		// Mace ethernet controller
	MachineDevice<struct PDMMACEDMARegs>	maceDMA;		// Mace PDM DMA Controller
	
	// ********************** USB **********************
	// The USB busses (all supported chips are probed via PCI)
	class USBBus*						usbBusses;
	
	// ********************** PCI **********************
	class PCIDevice*					pciDeviceList;		// A list of all PCI devices on all the buses.  These are abstract enough that you can
													// use them directly, but you'll probably want to subclass them with real drivers...
	
	// ****************** Actual Drivers ******************
	NKInterruptSafeList<Driver>			driverList;		// All drivers get tacked on here.  Don't use this field until C++ globals are inited!
}MachineInfo;

enum
{
	classPDM			=	1,
	classPCI			=	2,
	classPerforma		=	3,
	classPowerBook	=	4,
	classPowerBookPCI	=	5,
	classG3			=	6,	// Non-New World G3's (beige, PB)
	classNewWorld		=	7
};

enum
{
	cpu601	=	1,
	cpu603	=	3,
	cpu604	=	4,
	cpu603e	=	6,
	cpu603ev	=	7,
	cpu750	=	8,
	cpu604e	=	9,
	cpu7400	=	12
};

enum
{
	machineUnknown	=	0,
	
	// PDM Machines
	machine6100,
	machine7100,
	machine8100,
	
	// PCI Machines
	machine7200,
	machine7300,
	machine7500,
	machine7600,
	machine8500,
	machine9500,
	machinePB3400,
	machineNewWorld,
	machineDesktopG3,
	machinePBG3,
	
	// Performa Machines
	machine4400,
	machine5200,
	machine5400,
	machine5500,
	machine6200,
	machine6400,
	machine6500,
	machine20thAnniv,
	
	// Duos
	machineDuo2300,
	
	// Powerbooks
	machinePB5300,
	
	// Others
	machineAWS9150,
	
	// Unknown
	machinePB1400,
	machinePB2300,
	machinePB2400
};

extern MachineInfo	machine;

void NKMachineInit(void);
void NKMachineOpenFirmwareInit(void);	// Call this after OpenFirmware has been set up to get all the addresses and interrupts stuffed in the machine struct above

#endif /* __NK_MACHINEINIT__ */