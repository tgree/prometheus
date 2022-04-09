/*
	USBBootMouse.cp
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
	Other sources			Project
	Author		Notes
	===========			======
	=====		====

	Version History
	============
	Patrick Varilly		-	Wed, 8 Dec 99			-
	Creation of file
*/

#include "USB.h"
#include "USBDriver.h"
#include "USBInterface.h"
#include "USBCommandFactory.h"
#include "NKDebuggerNub.h"
#include "Streams.h"
#include "Macros.h"

#include "ADBMouse.h"		// Kludge for now.  Must separate mouse and ADB driver for it

class USBBootMouseInterface	:	public USBInterface
{
public:

	USBBootMouseInterface( USBInterfaceDescription* interface,

	USBInterfacedDevice* owner, UInt8 interfaceNum, UInt8 setting );
	virtual					~USBBootMouseInterface();

	virtual void				threadLoop();

private:
	Boolean					buttons[3];
};

class USBBootMouseDriver	:	public USBInterfaceDriver
{
public:
							USBBootMouseDriver();
	virtual					~USBBootMouseDriver();

	virtual USBInterface*		buildInterface(USBInterfaceDescription *description,
								USBInterfacedDevice* device, UInt16 idVendor, UInt16 idProduct,
								UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber,
								UInt8 setting );
};

static USBBootMouseDriver			mouseDriver;

USBBootMouseDriver::USBBootMouseDriver()
{
	registerClassDriver( kClassHID, kSubClassHIDBoot, kProtocolHIDMouse );
}

USBBootMouseDriver::~USBBootMouseDriver()
{
}

USBInterface* USBBootMouseDriver::buildInterface( USBInterfaceDescription *description,USBInterfacedDevice* device,
	UInt16 /*idVendor*/, UInt16 /*idProduct*/, UInt16 /*bcdDevice*/, UInt8 /*bConfigurationValue*/, UInt8 bInterfaceNumber,
	UInt8 setting )
{
	return new USBBootMouseInterface( description, device, bInterfaceNumber, setting );
}

// *** Boot mouse driver start here ***

USBBootMouseInterface::USBBootMouseInterface( USBInterfaceDescription* interface, USBInterfacedDevice* owner,
	UInt8 interfaceNum, UInt8 setting ) :
	USBInterface( interface, owner, interfaceNum, setting )
{
	UInt32					i;
	for( i = 0; i < 3; i++ )
		buttons[i] = false;
}

USBBootMouseInterface::~USBBootMouseInterface()
{
}

void USBBootMouseInterface::threadLoop()
{
	// Try to set to boot protocol
	USBErr					err = kUSBErr_None;
	USBControlSetup			cmd;
	USBCommandFactory::hidSetProtocol( cmd, getInterfaceNum(), true );
	err = sendClassCommand( &cmd );
	if( !err )
	{
		// Try to shut up, if possible
		USBCommandFactory::hidSetIdle( cmd, getInterfaceNum(), 0, 0 );
		sendClassCommand( &cmd );

		// Get interrupt pipe
		USBPipe				*intPipe;
		intPipe = &getInterface()->pipes[0];		// Should be 0

		// Loop until disconnected
		while( getOwner()->connected() )
		{
			// The report could be longer, we don't care
			UInt8			rawBuffer[3];
			interruptTransfer( intPipe, rawBuffer, 3 );

			// Report changes in mouse position
			SInt8			changeX, changeY;
			changeX = *((SInt8*)&rawBuffer[1]);		// To not loose sign
			changeY = *((SInt8*)&rawBuffer[2]);		// To not loose sign
			if( changeX || changeY )
				MouseHandler::tellAllMouseMoves( changeX*USB_NORMALIZE_NUM/USB_NORMALIZE_DENOM, changeY*USB_NORMALIZE_NUM/USB_NORMALIZE_DENOM );

			// Report changes in button state
			UInt8			button;
			for( button = 0; button < 3; button++ )
			{
				if( ((rawBuffer[0] & (1<<button)) != 0) != buttons[button] )
				{
					MouseHandler::tellAllMouseClicked(button, ((rawBuffer[0] & (1<<button)) != 0) );
					buttons[button] = ((rawBuffer[0] & (1<<button)) != 0);
				}
			}
		}
	}
	else
	{
		usbOut << "Boot protocol enabling failed! Error: " << gUSBErrorStrings[err] << "\n";
	}
}
