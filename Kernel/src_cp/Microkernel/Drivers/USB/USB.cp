/*
	USB.cp
	Copyright © 1999 by Patrick Varilly

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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	
	Version History
	============
	Patrick Varilly		-	Monday, 29 March 99	-	Creation of file
*/

#include "USB.h"
#include "USBHCDriver.h"
#include "USBHub.h"
#include "USBConfigurator.h"
#include "ANSI.h"
#include "NKThreads.h"
#include "NKMachineInit.h"
#include "Kernel Console.h"
#include "Gonzales.h"

UInt32							USBBus::_nextBusID = 0;
OStreamWrapper					usbOut("usbOut");
ASCII8Str							gUSBErrorStrings[] = {
	"None", "Operation unsupported", "Out of resources", "In use", "Internal error", "Bad arguments", "Aborted",
	"Data overrun", "Data underrun", "Buffer overrun", "Buffer underrun", "Resource deleted",
	"CRC mismatch", "Bit-stuffing violation", "Data toggle mismatch", "Stall", "Device not responding",
	"PID check failure", "Unexpected PID", "Device removed" };

// Pat, I think this is what you really meant, right?  A "const USBDevice*" means you can't do "myDevice->myValue = x",
// but you can do "myDevice = x".  A "USBDevice * const" means you can do "myDevice->myValue = x", but you can't do
// "myDevice = x".  I think you wanted these to be constants that you use to test stuff against or something.
USBDevice * const					kDeviceBeingProbed = (USBDevice*)0xDEADBEEF;
USBDevice * const					kDeviceRemovedWhileProbing = (USBDevice*)0xFEEDDEAD;
USBDevice * const					kDeviceProblemsProbing = (USBDevice*)0xDEADBEAD;

class USBPseudoKeyboard : public IStream
{
public:
						USBPseudoKeyboard( ASCII8Str stringToFeed );
	
protected:
	virtual	void			read(Ptr data,UInt32 len);
	
	ASCII8Str				curPos;
	UInt32				lenRemain;
};

void InitUSB( void )
{
	// This is just in case no keyboard driver is installed.
	// Any keyboard drivers will replace this one, provided they are loaded later.
	USBPseudoKeyboard		*keyboard;
	keyboard = new USBPseudoKeyboard( "patrick\rbubba312\rshutdown\r" );
	cin.setIStream(keyboard);
}

void InitUSBBusses( void )
{
	// Create USB console
	Rect					consoleBounds = {4*video->width()/5,0,video->width(),4*video->height()/5};
	VideoConsoleStream		*usbConsole;
	usbConsole = new VideoConsoleStream( &consoleBounds, true, gonzales9 );
	FatalAssert( usbConsole != nil );
	usbOut.setOStream( usbConsole );
	
	usbOut << "USB Console\n===========\n\n";
	
	USBBus				*bus = machine.usbBusses;
	
	while( bus )
	{
		// Get root hub of bus
		bus->rootHub = bus->driver->getRootHub( bus );
		if( !bus->rootHub )
			Panic( "Still can't do normal Hub initialization!" );
		bus->device[1] = bus->rootHub;
		
		// Start up threads to do enumeration
		usbOut << greenMsg << "Starting Root Hub on bus " << bus->bus() << "\n" << whiteMsg;
		bus->rootHub->resume();
			
		bus = bus->next();
	}
}

void USBStat( void )
{
	// Walk through all the busses, displaying the address of every device we have
	USBBus				*bus = machine.usbBusses;
	
	while( bus )
	{
		cout << "USB Bus " << bus->bus() << ":" << newLine;
		
		{
			MutexLocker	locker(bus->topologyLock);
			UInt32		i;
			
			for( i = 1; i < 128; i++ )
				if( bus->device[i] != nil )
					cout << "\tDevice at address " << i << newLine;
		}
		
		bus = bus->next();
	}
}

USBPseudoKeyboard::USBPseudoKeyboard( ASCII8Str stringToFeed )
{
	curPos = stringToFeed;
	lenRemain = strlen( stringToFeed );
}

void
USBPseudoKeyboard::read( Ptr data, UInt32 len )
{
	if( len > lenRemain )
		CurrThread::suspend();
	
	lenRemain -= len;
	memcpy( data, curPos, len );
	curPos += len;
}

USBHCDriver::USBHCDriver( ConstASCII8Str name )
	: Driver( name )
{
}

USBHCDriver::~USBHCDriver()
{
}

USBBus::USBBus( USBHCDriver* inDriver )
{
	// Init
	driver = inDriver;
	for( UInt32 i = 0; i < 128; i++ )
		device[i] = nil;
	_next = nil;
	_busID = _nextBusID;
	_nextBusID++;
	
	// Create default pipes
	USBErr		err;
	err = driver->createControlPipe( defaultPipe, 0, 0, true, 8 );
	FatalAssert( err == kUSBErr_None );
	err = driver->createControlPipe( slowDefaultPipe, 0, 0, false, 8 );
	FatalAssert( err == kUSBErr_None );
	
	// Link up
	USBBus*		bus = machine.usbBusses;
	USBBus*		prev = nil;
	while(bus)
	{
		prev = bus;
		bus = bus->_next;
	}
	if(prev)
		prev->_next = this;
	else
		machine.usbBusses = this;
}

UInt32
USBBus::bus()
{
	return _busID;
}

USBBus*
USBBus::next()
{
	return _next;
}

USBDevice*
USBBus::getDevice( ConstUSBDevID devID )
{
	return device[devID];
}

USBDevID
USBBus::deviceAdded( Boolean fullSpeed )
{
	MutexLocker		locker(topologyLock);
	
	// Look for a device slot that's empty
	USBDevID				devID;
	for( devID = 1; devID < 128; devID++ )
	{
		if( device[devID] == nil )
			break;
	}
	
	// If none is found, return 0 (hub driver will complain)
	if( devID == 128 )
		return 0;
	
	// Change address
	USBControlSetup		setup;
	USBTransaction			*trans;
	USBErr				err;
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBSetAddress;
	setup.wValue = LE16(devID);
	setup.wIndex = 0;
	setup.wLength = 0;
	if( fullSpeed )
		err = driver->controlTransfer( trans, defaultPipe, &setup );
	else
		err = driver->controlTransfer( trans, slowDefaultPipe, &setup );
	if( err )
		return 0;
	CurrThread::blockForIO( trans );
	if( trans->ioError() )
		return 0;
	CurrThread::sleepMS( 2 );		// 2ms SetAddress recovery delay
	
	// Probe device now
	device[devID] = kDeviceBeingProbed;
	(new USBConfigurator( this, devID, fullSpeed ))->resume();
	
	return devID;
}

void
USBBus::deviceRemoved( ConstUSBDevID devID )
{
	USBDevice			*dev;
	
	{
		MutexLocker	locker(topologyLock);
		dev = device[devID];
		if( dev == kDeviceProblemsProbing )
		{
			device[devID] = nil;
			return;
		}
		else if( dev != kDeviceBeingProbed )
			device[devID] = nil;
		else
		{
			device[devID] = kDeviceRemovedWhileProbing;
			return;
		}
	}
	
	if( dev != nil )
		dev->deviceRemoved();
}

void
USBBus::deviceProbed( ConstUSBDevID devID, USBDevice* dev )
{
	MutexLocker		locker(topologyLock);
	if( device[devID] == kDeviceRemovedWhileProbing )
	{
		if( dev )
			dev->deviceRemoved();
		device[devID] = nil;
	}
	else if( device[devID] == kDeviceBeingProbed )
	{
		if( dev )
			device[devID] = dev;
		else
			device[devID] = kDeviceProblemsProbing;
	}
	else
		usbOut << "Spurious deviceProbed() call\n";
}

USBTransaction::USBTransaction()
{
}

USBTransaction::~USBTransaction()
{
}