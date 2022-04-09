/*
	USBDriver.h
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
	Patrick Varilly		-	Sat, 4 Dec 99		-	Creation of file
*/

#ifndef __USB_DRIVER__
#define __USB_DRIVER__

#include "USB.h"

class USBInterfacedDevice;

class USBPipe
{
public:
	USBEndpointDesc			*desc;
	
protected:
	friend class USBConfigurator;
	friend class USBDevice;
	friend class USBDeviceDescription;
	USBHCPipe				*pipe;
};

typedef struct USBInterfaceSetting
{
	USBInterfaceDesc			*desc;
	USBPipe					*pipes;
} USBInterfaceSetting;

typedef struct USBInterfaceDescription
{
	UInt8					numSettings, curSetting;
	USBInterfaceSetting			*settings;
} USBInterfaceDescription;

typedef struct USBConfiguration
{
	USBConfigDesc				*desc;
	USBInterfaceDescription		*interfaces;
} USBConfiguration;

class USBDeviceDescription
{
public:
							USBDeviceDescription( USBBus *bus, ConstUSBDevID devID, Boolean fullSpeed,
								USBHCPipe *defaultPipe );
	virtual					~USBDeviceDescription();
	
	USBDeviceDesc				*desc;
	USBConfiguration			*configs;
	
protected:
	friend class USBConfigurator;
	friend class USBDevice;
	
	inline USBBus*				bus() { return _bus; }
	inline USBDevID				devID() { return _devID; }
	inline Boolean				fullSpeed() { return _fullSpeed; }
	inline USBHCPipe*			defaultPipe() { return _defaultPipe; }
	
private:
	USBBus					*_bus;
	USBDevID					_devID;
	Boolean					_fullSpeed;
	USBHCPipe				*_defaultPipe;
};

class USBDeviceDriver
{
public:
							USBDeviceDriver();
	virtual					~USBDeviceDriver();
	
	static USBDevice*			tryToBuildDevice( USBDeviceDescription *description, Boolean onlyProduct );
	
protected:
	USBErr					registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt16 bcdDevice );
	USBErr					registerProductDriver( UInt16 idVendor, UInt16 idProduct );
	USBErr					registerVendorDriver( UInt16 idVendor, UInt8 bDeviceSubClass, UInt8 bDeviceProtocol );
	USBErr					registerVendorDriver( UInt16 idVendor, UInt8 bDeviceSubClass );
	USBErr					registerClassDriver( UInt8 bDeviceClass, UInt8 bDeviceSubClass, UInt8 bDeviceProtocol );
	USBErr					registerClassDriver( UInt8 bDeviceClass, UInt8 bDeviceSubClass );
	
	virtual USBDevice*			buildDevice( USBDeviceDescription *description ) = 0;
	
private:
	static USBErr				registerDriver( USBDeviceDriver* driver, UInt8 driverClass, UInt16 idVendor,
								UInt16 idProduct, UInt16 bcdDevice, UInt8 bDeviceClass, UInt8 bDeviceSubClass,
								UInt8 bDeviceProtocol );
};

class USBInterfaceDriver
{
public:
							USBInterfaceDriver();
	virtual					~USBInterfaceDriver();
	
	static USBInterface*			tryToBuildInterface( USBInterfaceDescription *description,
								USBInterfacedDevice* device, UInt16 idVendor, UInt16 idProduct,
								UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber,
								UInt8& outSetting );
	
protected:
	USBErr					registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt16 bcdDevice,
								UInt8 bConfigurationValue, UInt8 bInterfaceNumber );
	USBErr					registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt8 bConfigurationValue,
								UInt8 bInterfaceNumber );
	USBErr					registerVendorDriver( UInt16 idVendor, UInt8 bInterfaceSubClass,
								UInt8 bInterfaceProtocol );
	USBErr					registerVendorDriver( UInt16 idVendor, UInt8 bInterfaceSubClass );
	USBErr					registerClassDriver( UInt8 bInterfaceClass, UInt8 bInterfaceSubClass,
								UInt8 bInterfaceProtocol );
	USBErr					registerClassDriver( UInt8 bInterfaceClass, UInt8 bInterfaceSubClass );
	
	virtual USBInterface*		buildInterface( USBInterfaceDescription *description,
								USBInterfacedDevice* device, UInt16 idVendor, UInt16 idProduct,
								UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber,
								UInt8 setting ) = 0;
	
private:
	static USBErr				registerDriver( USBInterfaceDriver* driver, UInt8 driverClass, UInt16 idVendor,
								UInt16 idProduct, UInt16 bcdDevice, UInt8 bConfigurationValue,
								UInt8 bInterfaceNumber, UInt8 bInterfaceClass, UInt8 bInterfaceSubClass,
								UInt8 bInterfaceProtocol );
};

#endif /* __USB_DRIVER__ */