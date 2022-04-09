/*
	USBInterface.cp
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
	Patrick Varilly		-	Mon, 6 Dec 99			-	Creation of file
*/

#include "USBInterface.h"

USBInterfacedDevice::USBInterfacedDevice( USBDeviceDescription* description ) :
	USBDevice( description, "USB Interfaced Device" ), connectSem(0), interfaceSem(0)
{
	interfaces = nil;
	numInterfaces = 0;
}

USBInterfacedDevice::~USBInterfacedDevice()
{
	if( interfaces )
		delete [] interfaces;
}

USBErr
USBInterfacedDevice::configure()
{
	// Select first configuration which doesn't give an error
	USBErr				err;
	UInt8				numConfigs = getNumConfigs();
	UInt32				i;
	for( i = 0; i < numConfigs; i++ )
	{
		err = setConfig( i );
		if( !err )
			break;
	}
	
	// If there wasn't an error, create interfaces array
	if( !err )
	{
		USBConfiguration	*config = getConfig( i );
		interfaces = new USBInterface*[config->desc->bNumInterfaces];
		if( !interfaces )
			err = kUSBErr_OutOfResources;
		for( i = 0; i < config->desc->bNumInterfaces; i++ )
			interfaces[i] = nil;
	}
	
	return err;
}

void
USBInterfacedDevice::assignInterface( UInt8 intNum, USBInterface* interface, UInt8 setting )
{
	interfaces[intNum] = interface;
	numInterfaces++;
	if( setting != getConfig( getConfigNum() )->interfaces[intNum].curSetting )
		setInterface( intNum, setting );
}

void
USBInterfacedDevice::interfaceDestroyed( UInt8 intNum )
{
	interfaces[intNum] = nil;
	interfaceSem.up();
}

void
USBInterfacedDevice::deviceLoop()
{
	// Start all interfaces
	UInt32			i;
	USBConfiguration	*config = getConfig( getConfigNum() );
	for( i = 0; i < config->desc->bNumInterfaces; i++ )
	{
		if( interfaces[i] )
			interfaces[i]->resume();
	}
	
	// Block until disconnected
	connectSem.down();
	
	// Wait for all interfaces to disappear
	for( i = 0; i < numInterfaces; i++ )
		interfaceSem.down();
	
	delete this;
}

void
USBInterfacedDevice::deviceRemoved()
{
	USBDevice::deviceRemoved();
	connectSem.up();
}

USBInterface::USBInterface( USBInterfaceDescription* interface, USBInterfacedDevice* owner, UInt8 interfaceNum,
	UInt8 setting )
	: Thread( 4096, kPriorityDriver, "USB Interface", true ),
	_interface( &interface->settings[setting] ), _owner( owner ), _interfaceNum( interfaceNum ), _setting( setting )
{
}

USBInterface::~USBInterface()
{
	_owner->interfaceDestroyed( _interfaceNum );
}

void
USBInterface::threadLoop()
{
	usbOut << redMsg << "Shouldn't be here!!!\n" << whiteMsg;
}

USBErr
USBInterface::sendClassCommand( USBControlSetupPtr cmd, void* buffer )
{
	// For now, don't check anything
	return _owner->sendClassCommand( cmd, buffer );
}

USBErr
USBInterface::controlTransfer( USBPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer, Boolean shortTransferOK )
{
	// For now, don't check anything
	return _owner->controlTransfer( pipe, setup, buffer, shortTransferOK );
}

USBErr
USBInterface::interruptTransfer( USBPipe *pipe, void* buffer, const UInt32 bufferSize, Boolean shortTransferOK )
{
	// For now, don't check anything
	return _owner->interruptTransfer( pipe, buffer, bufferSize, shortTransferOK );
}