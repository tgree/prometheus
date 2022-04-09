/*
	USBInterface.h
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

#ifndef __USB_INTERFACE__
#define __USB_INTERFACE__

#include "USB.h"
#include "USBDevice.h"
#include "NKThreads.h"

class USBInterface;

class USBInterfacedDevice :
	public USBDevice
{
public:
						USBInterfacedDevice( USBDeviceDescription* description );
	virtual				~USBInterfacedDevice();
	
	USBErr				configure();
	
	virtual void			deviceLoop();
	virtual void			deviceRemoved();				// Callback from the USBBus
	
	friend class USBInterface;
	friend class USBConfigurator;
	
private:
	USBInterface*			*interfaces;
	Semaphore			connectSem, interfaceSem;
	UInt32				numInterfaces;
	
	void					assignInterface( UInt8 intNum, USBInterface* interface, UInt8 setting );
	void					interfaceDestroyed( UInt8 intNum );
};

class USBInterface :
	public Thread
{
public:
						USBInterface( USBInterfaceDescription* interface, USBInterfacedDevice* owner,
							UInt8 interfaceNum, UInt8 setting );
	virtual				~USBInterface();
	
	virtual void			threadLoop();
	
protected:
	inline USBInterfacedDevice* getOwner() { return _owner; }
	inline USBInterfaceSetting* getInterface() { return _interface; }
	inline UInt8			getInterfaceNum() { return _interfaceNum; }
	
	// Non-standard commands
	USBErr				sendClassCommand( USBControlSetupPtr cmd, void* buffer = nil );
	
	// Pipe transfers
	USBErr				controlTransfer( USBPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer = nil,
							Boolean shortTransferOK = false );
	USBErr				interruptTransfer( USBPipe *pipe, void* buffer, const UInt32 bufferSize,
							Boolean shortTransferOK = false );
	
private:
	USBInterfaceSetting		*_interface;
	USBInterfacedDevice		*_owner;
	UInt8				_interfaceNum, _setting;
};

#endif /* __USB_INTERFACE__ */