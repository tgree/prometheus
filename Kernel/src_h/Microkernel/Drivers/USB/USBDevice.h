/*
	USBDevice.h
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
	Patrick Varilly		-	Sat, 27 Nov 99		-	Creation of file
*/

#ifndef __USB_DEVICE__
#define __USB_DEVICE__

#include "USB.h"
#include "USBDriver.h"
#include "NKThreads.h"

#define USB_DEVICE_THREAD_STACK	4096

class USBInterface;

class USBDevice :
	public Thread
{
public:
						USBDevice( USBDeviceDescription *description, ConstASCII8Str name,
							UInt32 stackSize = USB_DEVICE_THREAD_STACK );
	virtual				~USBDevice();
	
	virtual void			threadLoop();
	
	virtual void			initialize();
	virtual void			deviceLoop();
	
	inline USBBus*			bus() { return description->bus(); }
	inline USBDevID			address() { return description->devID(); }
	inline Boolean			connected() { return _connected; }
	inline Boolean			fullSpeed() { return description->fullSpeed(); }
	
	virtual void			deviceRemoved();				// Callback from the USBBus
	
protected:
	friend class USBInterface;
	
	// Device descriptor
	USBDeviceDesc*		getDeviceDesc() { return description->desc; }
	
	// Configuration
	inline UInt8			getNumConfigs() { return description->desc->bNumConfigurations; }
	USBErr				setConfig( UInt8 configNum );
	
	USBConfiguration*		getConfig( UInt8 configNum );
	inline UInt8			getConfigNum() { return _curConfig; }
	
	// Interface settings
	UInt8				getInterface( UInt8 interfaceNum );
	USBErr				setInterface( UInt8 interfaceNum, UInt8 settingNum );
	
	// Non-standard commands
	USBErr				sendClassCommand( USBControlSetupPtr cmd, void* buffer = nil );
	
	// Pipe transfers
	USBErr				controlTransfer( USBPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer = nil,
							Boolean shortTransferOK = false );
	USBErr				interruptTransfer( USBPipe *pipe, void* buffer, const UInt32 bufferSize,
							Boolean shortTransferOK = false );
	
private:
	USBErr				sendCommand( USBHCPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer = nil,
							Boolean shortTransferOK = false );
	
	USBDeviceDescription	*description;
	Boolean				_connected;
	UInt8				_curConfig;
};

// Dummy device, does nothing, but gets deleted when removed
class USBDummyDevice :
	public USBDevice
{
public:
						USBDummyDevice( USBDeviceDescription *description );
	virtual				~USBDummyDevice();
	
	virtual void			deviceRemoved();
};

#endif /* __USB_DEVICE__ */