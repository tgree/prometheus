/*
	MicroKernelStart.cp
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
*/
#include "MicroKernelStart.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKInterruptVectors.h"
#include "NKThreads.h"
#include "NKVideo.h"
#include "NKProcessors.h"
#include "External Interrupt.h"
#include "Kernel Console.h"
#include "Video Driver.h"
#include "ESCC.h"
#include "Time.h"
#include "Compiler.h"
#include "Assembly.h"
#include "Config.h"
#include "ADB.h"
#include "ADBMouse.h"
#include "ADBKeyboard.h"
#include "Gonzales.h"
#include "SCSI Device.h"
#include "Mace.h"
#include "Code Fragments.h"
#include "File Systems.h"
#include "OSs.h"
#include "IDE Device.h"
#include "OpenFirmware.h"
#include "Login.h"
#include "USB.h"
#include "Chip Debugger.h"
#include "PCI.h"
#include "SWIM3.h"

OStreamWrapper	cout("cout");
IStreamWrapper	cin("cin");

typedef void (*DriverInitProc)(void);
typedef struct DriverInitInfo
{
	DriverInitProc		proc;
	ConstASCII8Str	procName;
}DriverInitInfo;

DriverInitInfo	driverProc[]	=	{	{InitPCI,"PCI"},
								{InitADB,"ADB"},
								{InitSCSI,"SCSI"},
								{InitMace,"Mace"},
								{InitIDE,"IDE"},
								//{InitUSB,"USB"},
								{InitESCC,"ESCC"},
								{InitSWIM3,"SWIM3"}
							};

static void DumpOpenFirmwareTree();

void MicroKernelStart(void)
{
	// We are coming in from the Nanokernel.  Paged virtual memory works, as does the nanokernel video driver.  If you need to print
	// messages for debugging purposes before the video console is inited, uses the nanokernel console, nkVideo, just like you would use
	// any other stream.  In fact, you can use nkVideo before you even call InitCPPGlobals().
	
	// Make OpenFirmware work
	nkVideo << "Initializing OpenFirmware\n";
	machine.openFirmwareTree = new OpenFirmwareTree(machine.openFirmwareFlattenedTree);
	NKMachineOpenFirmwareInit();
	
	// Construct global C++ objects.
	nkVideo << "Initializing C++ Globals\n";
	InitCPPGlobals();
	
	// Initialize video.  This creates the Video Driver "video" C++ object and maps video ram.  This must be done before you call InitvideoConsole()
	// InitMemoryManager() must be called before InitVideo().
	nkVideo << "Initializing Kernel Video\n";
	InitVideo();
	
	// Initialize the kernel console.
	Rect	consoleBounds = {0,0,4*video->width()/5,4*video->height()/5};
	InitVideoConsole(&consoleBounds,true,gonzales9);
	
	cout.setOStream(videoConsole);
	cout << "Prometheus " << microKernelVers << "\n___________________\n";
	cout << "\nWelcome to Prometheus, bringer of fire!\n";
	cout << "\nBoot processor is a PowerPC " << NKGetProcessorInfo(0)->processorName << " processor\n";
	for(UInt32 i=1;i<NKGetNumProcessors();i++)
		cout << "Processor " << i << " is a PowerPC " << NKGetProcessorInfo(i)->processorName << " processor\n";
	cout << "This is a";
	ASCII8				c = machine.machineName[0];
	if( (c == 'A') || (c == 'a') || (c == 'E') || (c == 'e') || (c == 'I') || (c == 'i') || (c == 'O') || (c == 'o') || (c == 'U') || (c == 'u') )
		cout << "n";
	cout << " " << machine.machineName << "\n";
	
	// Initialize code fragments and also PEF info for the kernel
	cout << "\nInitializing code fragments.\n";
	InitCodeFragments();
	
	// Dump OpenFirmware tree
	//nkVideo << "\nDumping OpenFirmware Tree\n";
	//DumpOpenFirmwareTree();
	
	// Initialize time - don't use any time routines until you call this!!!!!!
	InitTime();
	
	// ***************** First part of device initialization **************************
	// Here we allocate all device structures and tell them to disable their interrupts from being generated.
	// *****************************************************************
	
	// Here we call the init routines for all the drivers.  These routines enqueue Driver*'s onto the
	// machine.driverList queue.  We later use that queue to initialize() and start() the drivers.
	for(UInt32 i=0;i<sizeof(driverProc)/sizeof(driverProc[0]);i++)
	{
		//cout << "\nInitializing " << driverProc[i].procName << ".\n";
		(*driverProc[i].proc)();
	}
	cout << "\n";
	
	// Reset all drivers.  This causes them to stop generating interrupts, so we can then proceed to turn on external interrupts and use
	// threads.
	for(UInt32 i=0;;i++)
	{
		Driver* theDev = machine.driverList[i];
		if(!theDev)
			break;
		cout << "Initializing driver: \"" << theDev->driverName() << "\"\n";
		theDev->initialize();
	}
	
/*#define FOUR_NL		"\n\n\n\n"
#define EIGHT_NL		FOUR_NL FOUR_NL
#define SIXTEEN_NL		EIGHT_NL EIGHT_NL
#define THIRTYTWO_NL	SIXTEEN_NL SIXTEEN_NL
#define SIXTYFOUR_NL	THIRTYTWO_NL THIRTYTWO_NL
#define ONETWOEIGHT_NL	SIXTYFOUR_NL SIXTYFOUR_NL
	
	nkVideo << THIRTYTWO_NL;
	nkVideo << "Gauging newline speed\n";
	Float64			start,end;
	start = GetTime_ns();
	nkVideo << ONETWOEIGHT_NL;
	end = GetTime_ns();
	cout << "Newline speed: " << (UInt32)((end-start)/128)/1000 << " us\n";
	Wait_s(10);*/
	
	// Initialize external and decrementor interrupts.
	cout << "\nInitializing external interrupts.\n";
	InitExternalInterrupt();
	
	// Turn on threads now that the decrementor interrupts have been enabled.  This is a call back into the nanokernel.
	cout << "Initializing preemptive threads.\n";
	NKInitThreads();
	
	// FIXME MP: We should notify other processor to get their decrementors up and running, and to fire ASAP, to
	// get into the scheduler.  Once the scheduler takes over, MP is completely transparent!!!
	if( NKGetNumProcessors() > 1 )
		Panic("More changes to kernel: Need to tell other processors to enable decrementor exceptions!!!");
	
	//NKGaugeThreads();
	
	// ***************** Second part of device initialization *************************
	// Here we enable all devices to start generating interrupts.  This will all be threaded later.
	// *****************************************************************
	cout << "\n";
	for(UInt32 i=0;;i++)
	{
		Driver* theDev = machine.driverList[i];
		if(!theDev)
			break;
		cout << "Starting driver: \"" << theDev->driverName() << "\"\n";
		theDev->start();
	}
	
	// Initialize USB busses
	InitUSBBusses();
	
	// Initialize ADB Devices
	if( ADBPresent() )
	{
		cout << "\nInitializing ADB Devices\n";
		PrintADBInfo();
		
		// Initialize the keyboard - must be done after initializing ADB
		InitKeyboard();
		
		// Initialize mouse tracking - must be done after initializing ADB
		InitMouse();
	}
	
	// Initialize HFS volumes
	InitSWIM3Volumes();
	InitSCSIVolumes();
	InitIDEVolumes();
	
	// Go directly to the Chip Debugger
	//DebugChips();
	
	// Map the keyboard to cin
	cout << whiteMsg << "\n\nMapping keyboard to cin\n";
	
	cin.setIStream(&keyboard);
	
	cout << greenMsg << "\nMicrokernel finished loading\n" << whiteMsg;
	
	//ShutDown();
	
	Login();
	
	cout << greenMsg << "\n\nForever loop\n";
	for(;;)
		;
}

static void DumpNode(OpenFirmwareNode* node,UInt32 level);

void DumpOpenFirmwareTree()
{
	DumpNode(machine.openFirmwareTree->getNode(0),0);
}

void DumpNode(OpenFirmwareNode* node,UInt32 level)
{
	// Dump this node's name
	for(Int32 i=0;i<level;i++)
		cout << "  ";
	cout << node->name() << " (" << node->devType() << ")\n";
	
	// Dump this node's children
	OpenFirmwareNode*	child = node->firstChild();
	while(child)
	{
		DumpNode(child,level+1);
		child = child->nextSibling();
	}
}
