/*
	USBDriver.cp
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

#include "USBDriver.h"
#include "USBHCDriver.h"

enum
{
	// Class types
	classProductAndVersion = 0,
	classProduct,
	classVendorAndProtocol,
	classVendor,
	classClassAndProtocol,
	classClass,
	classMax,
	
	// Masks for each field
	maskVendor = 0x1,
	maskProduct = 0x2,
	maskDevice = 0x4,
	maskClass = 0x8,
	maskSubClass = 0x10,
	maskProtocol = 0x20
};

const UInt8						masks[classMax] =
	{ maskVendor | maskProduct | maskDevice,
	maskVendor | maskProduct,
	maskVendor | maskSubClass | maskProtocol,
	maskVendor | maskSubClass,
	maskClass | maskSubClass | maskProtocol,
	maskClass | maskSubClass };

typedef struct USBDeviceDriverDescription
{
	UInt16						idVendor, idProduct;
	UInt16						bcdDevice;
	UInt8						bDeviceClass, bDeviceSubClass, bDeviceProtocol;
	USBDeviceDriver				*driver;
	USBDeviceDriverDescription		*next;
} USBDeviceDriverDescription;

typedef struct USBInterfaceDriverDescription
{
	UInt16						idVendor, idProduct;
	UInt16						bcdDevice;
	UInt8						bConfigurationValue, bInterfaceNumber;
	UInt8						bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol;
	USBInterfaceDriver				*driver;
	USBInterfaceDriverDescription		*next;
} USBInterfaceDriverDescription;

static USBDeviceDriverDescription*		deviceDrivers[classMax] = { nil, nil, nil, nil, nil, nil };
static USBInterfaceDriverDescription*	interfaceDrivers[classMax] = { nil, nil, nil, nil, nil, nil };
static SpinLock						deviceDriverLock;
static SpinLock						interfaceDriverLock;

USBDeviceDriver::USBDeviceDriver()
{
}

USBDeviceDriver::~USBDeviceDriver()
{
	CriticalSection				critical(deviceDriverLock);
	UInt8					curClass;
	for( curClass = 0; curClass < classMax; curClass++ )
	{
		// Remove from all driver lists
		USBDeviceDriverDescription	*prev = nil, *cur = deviceDrivers[curClass], *next;
		while( cur )
		{
			next = cur->next;
			if( cur->driver == this )
			{
				// Remove me
				if( prev == nil )
					deviceDrivers[curClass] = next;
				else
					prev->next = next;
				delete cur;
			}
			else
				prev = cur;
			cur = next;
		}
	}
}

USBErr
USBDeviceDriver::registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt16 bcdDevice )
{
	return registerDriver( this, classProductAndVersion, idVendor, idProduct, bcdDevice, 0, 0, 0 );
}

USBErr
USBDeviceDriver::registerProductDriver( UInt16 idVendor, UInt16 idProduct )
{
	return registerDriver( this, classProduct, idVendor, idProduct, 0, 0, 0, 0 );
}

USBErr
USBDeviceDriver::registerVendorDriver( UInt16 idVendor, UInt8 bDeviceSubClass, UInt8 bDeviceProtocol )
{
	return registerDriver( this, classVendorAndProtocol, idVendor, 0, 0, 0, bDeviceSubClass, bDeviceProtocol );
}

USBErr
USBDeviceDriver::registerVendorDriver( UInt16 idVendor, UInt8 bDeviceSubClass )
{
	return registerDriver( this, classVendor, idVendor, 0, 0, 0, bDeviceSubClass, 0 );
}

USBErr
USBDeviceDriver::registerClassDriver( UInt8 bDeviceClass, UInt8 bDeviceSubClass, UInt8 bDeviceProtocol )
{
	return registerDriver( this, classClassAndProtocol, 0, 0, 0, bDeviceClass, bDeviceSubClass, bDeviceProtocol );
}

USBErr
USBDeviceDriver::registerClassDriver( UInt8 bDeviceClass, UInt8 bDeviceSubClass )
{
	return registerDriver( this, classClass, 0, 0, 0, bDeviceClass, bDeviceSubClass, 0 );
}

USBErr
USBDeviceDriver::registerDriver( USBDeviceDriver* driver, UInt8 driverClass, UInt16 idVendor, UInt16 idProduct,
	UInt16 bcdDevice, UInt8 bDeviceClass, UInt8 bDeviceSubClass, UInt8 bDeviceProtocol )
{
	// Make new USBDeviceDriverDescription
	USBDeviceDriverDescription		*desc;
	desc = new USBDeviceDriverDescription;
	if( desc == nil )
		return kUSBErr_OutOfResources;
	
	// Fill in
	desc->idVendor = idVendor;
	desc->idProduct = idProduct;
	desc->bcdDevice = bcdDevice;
	desc->bDeviceClass = bDeviceClass;
	desc->bDeviceSubClass = bDeviceSubClass;
	desc->bDeviceProtocol = bDeviceProtocol;
	desc->driver = driver;
	
	// Prepend it into proper list
	CriticalSection				critical(deviceDriverLock);
	desc->next = deviceDrivers[driverClass];
	deviceDrivers[driverClass] = desc;
	
	return kUSBErr_None;
}

USBDevice*
USBDeviceDriver::tryToBuildDevice( USBDeviceDescription *description, Boolean onlyProduct )
{
	CriticalSection				critical(deviceDriverLock);
	
	// Walk through each driver list seeing it there's a match
	UInt8					curClass, maxClass;
	if( onlyProduct )
		maxClass = classVendorAndProtocol;
	else
		maxClass = classMax;
	for( curClass = 0; curClass < classMax; curClass++ )
	{
		if( ((curClass == classVendorAndProtocol) || (curClass == classVendor)) &&
			(description->desc->bDeviceClass != 0xFF) )
			continue;
		if( ((curClass == classClassAndProtocol) || (curClass == classClass)) &&
			(description->desc->bDeviceClass == 0xFF) )
			continue;
		
		USBDeviceDriverDescription	*cur = deviceDrivers[curClass];
		while( cur )
		{
			Boolean			match = true;
			
			if( (masks[curClass] & maskVendor) && (description->desc->idVendor != cur->idVendor) )
				match = false;
			else if( (masks[curClass] & maskProduct) && (description->desc->idProduct != cur->idProduct) )
				match = false;
			else if( (masks[curClass] & maskDevice) && (description->desc->bcdDevice != cur->bcdDevice) )
				match = false;
			else if( (masks[curClass] & maskClass) && (description->desc->bDeviceClass != cur->bDeviceClass) )
				match = false;
			else if( (masks[curClass] & maskSubClass) && (description->desc->bDeviceSubClass != cur->bDeviceSubClass) )
				match = false;
			else if( (masks[curClass] & maskProtocol) && (description->desc->bDeviceProtocol != cur->bDeviceProtocol) )
				match = false;
			
			if( match )
			{
				// Attempt to build device
				USBDevice	*dev;
				dev = cur->driver->buildDevice( description );
				if( dev )
					return dev;
			}
			
			cur = cur->next;
		}
	}
	
	//usbOut << "No matching drivers found\n";
	return nil;
}

USBInterfaceDriver::USBInterfaceDriver()
{
}

USBInterfaceDriver::~USBInterfaceDriver()
{
	CriticalSection				critical(interfaceDriverLock);
	UInt8					curClass;
	for( curClass = 0; curClass < classMax; curClass++ )
	{
		// Remove from all driver lists
		USBInterfaceDriverDescription	*prev = nil, *cur = interfaceDrivers[curClass], *next;
		while( cur )
		{
			next = cur->next;
			if( cur->driver == this )
			{
				// Remove me
				if( prev == nil )
					interfaceDrivers[curClass] = next;
				else
					prev->next = next;
				delete cur;
			}
			else
				prev = cur;
			cur = next;
		}
	}
}

USBErr
USBInterfaceDriver::registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt16 bcdDevice,
	UInt8 bConfigurationValue, UInt8 bInterfaceNumber )
{
	return registerDriver( this, classProductAndVersion, idVendor, idProduct, bcdDevice, bConfigurationValue,
		bInterfaceNumber, 0, 0, 0 );
}

USBErr
USBInterfaceDriver::registerProductDriver( UInt16 idVendor, UInt16 idProduct, UInt8 bConfigurationValue,
	UInt8 bInterfaceNumber )
{
	return registerDriver( this, classProduct, idVendor, idProduct, 0, bConfigurationValue, bInterfaceNumber, 0, 0, 0 );
}

USBErr
USBInterfaceDriver::registerVendorDriver( UInt16 idVendor, UInt8 bInterfaceSubClass, UInt8 bInterfaceProtocol )
{
	return registerDriver( this, classVendorAndProtocol, idVendor, 0, 0, 0, 0, 0, bInterfaceSubClass, bInterfaceProtocol );
}

USBErr
USBInterfaceDriver::registerVendorDriver( UInt16 idVendor, UInt8 bInterfaceSubClass )
{
	return registerDriver( this, classVendor, idVendor, 0, 0, 0, 0, 0, bInterfaceSubClass, 0 );
}

USBErr
USBInterfaceDriver::registerClassDriver( UInt8 bInterfaceClass, UInt8 bInterfaceSubClass, UInt8 bInterfaceProtocol )
{
	return registerDriver( this, classClassAndProtocol, 0, 0, 0, 0, 0, bInterfaceClass, bInterfaceSubClass,
		bInterfaceProtocol );
}

USBErr
USBInterfaceDriver::registerClassDriver( UInt8 bInterfaceClass, UInt8 bInterfaceSubClass )
{
	return registerDriver( this, classClass, 0, 0, 0, 0, 0, bInterfaceClass, bInterfaceSubClass, 0 );
}

USBErr
USBInterfaceDriver::registerDriver( USBInterfaceDriver* driver, UInt8 driverClass, UInt16 idVendor, UInt16 idProduct,
	UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber, UInt8 bInterfaceClass, UInt8 bInterfaceSubClass,
	UInt8 bInterfaceProtocol )
{
	// Make new USBInterfaceDriverDescription
	USBInterfaceDriverDescription		*desc;
	desc = new USBInterfaceDriverDescription;
	if( desc == nil )
		return kUSBErr_OutOfResources;
	
	// Fill in
	desc->idVendor = idVendor;
	desc->idProduct = idProduct;
	desc->bcdDevice = bcdDevice;
	desc->bConfigurationValue = bConfigurationValue;
	desc->bInterfaceNumber = bInterfaceNumber;
	desc->bInterfaceClass = bInterfaceClass;
	desc->bInterfaceSubClass = bInterfaceSubClass;
	desc->bInterfaceProtocol = bInterfaceProtocol;
	desc->driver = driver;
	
	// Prepend it into proper list
	CriticalSection				critical(interfaceDriverLock);
	desc->next = interfaceDrivers[driverClass];
	interfaceDrivers[driverClass] = desc;
	
	return kUSBErr_None;
}

USBInterface*
USBInterfaceDriver::tryToBuildInterface( USBInterfaceDescription *description, USBInterfacedDevice* device,
	UInt16 idVendor, UInt16 idProduct, UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber,
	UInt8& outSetting )
{
	CriticalSection				critical(interfaceDriverLock);
	
	// Walk through each driver list seeing it there's a match
	UInt8					curClass, i;
	for( curClass = 0; curClass < classMax; curClass++ )
	{
		// Breadth-first search accross all settings
		for( i = 0; i < description->numSettings; i++ )
		{
			USBInterfaceSetting	*setting = &description->settings[i];
			if( ((curClass == classVendorAndProtocol) || (curClass == classVendor)) &&
				(setting->desc->bInterfaceClass != 0xFF) )
				continue;
			if( ((curClass == classClassAndProtocol) || (curClass == classClass)) &&
				(setting->desc->bInterfaceClass == 0xFF) )
				continue;
			
			USBInterfaceDriverDescription	*cur = interfaceDrivers[curClass];
			while( cur )
			{
				Boolean			match = true;
				
				if( (masks[curClass] & maskVendor) && (idVendor != cur->idVendor) )
					match = false;
				else if( (masks[curClass] & maskProduct) &&
					((idProduct != cur->idProduct) || (bConfigurationValue != cur->bConfigurationValue)
						|| (bInterfaceNumber != cur->bInterfaceNumber)) )
					match = false;
				else if( (masks[curClass] & maskDevice) && (bcdDevice != cur->bcdDevice) )
					match = false;
				else if( (masks[curClass] & maskClass)
					&& (setting->desc->bInterfaceClass != cur->bInterfaceClass) )
					match = false;
				else if( (masks[curClass] & maskSubClass)
					&& (setting->desc->bInterfaceSubClass != cur->bInterfaceSubClass) )
					match = false;
				else if( (masks[curClass] & maskProtocol)
					&& (setting->desc->bInterfaceProtocol != cur->bInterfaceProtocol) )
					match = false;
				
				if( match )
				{
					// Attempt to build interface
					USBInterface	*interface;
					interface = cur->driver->buildInterface( description, device, idVendor, idProduct, bcdDevice,
						bConfigurationValue, bInterfaceNumber, i );
					if( interface )
					{
						outSetting = i;
						return interface;
					}
				}
				
				cur = cur->next;
			}
		}
	}
	
	//usbOut << "No matching drivers found\n";
	return nil;
}

USBDeviceDescription::USBDeviceDescription( USBBus *bus, ConstUSBDevID devID, Boolean fullSpeed,
	USBHCPipe *defaultPipe ) :
	_bus( bus ), _devID( devID ), _fullSpeed( fullSpeed ), _defaultPipe( defaultPipe ), desc( nil ), configs( nil )
{
}

USBDeviceDescription::~USBDeviceDescription()
{
	// This is a bit involved, because we must delete everything below us (imagine a tree with all the descriptors...)
	// I should really make everything a class and give each its proper destructor (who the heck uses four loop counters
	// at once, anyway?!?!?!?!?)
	UInt32					i,j,k,l;
	
	if( desc )
	{
		UInt8				numConfigs;
		numConfigs = desc->bNumConfigurations;
		delete desc;
		
		if( configs )
		{
			for( i = 0; i < numConfigs; i++ )
			{
				if( configs[i].desc )
				{
					UInt8		numInterfaces = configs[i].desc->bNumInterfaces;
					if( configs[i].interfaces )
					{
						for( j = 0; j < numInterfaces; j++ )
						{
							USBInterfaceDescription	*interface = &configs[i].interfaces[j];
							if( interface->settings )
							{
								for( k = 0; k < interface->numSettings; k++ )
								{
									USBInterfaceSetting	*setting = &interface->settings[k];
									if( setting->desc && setting->pipes )
									{
										for( l = 0; l < setting->desc->bNumEndpoints; l++ )
										{
											if( setting->pipes[l].pipe )
												_bus->driver->deletePipe( setting->pipes[l].pipe );
										}
										delete [] setting->pipes;
									}
								}
								delete interface->settings;
							}
						}
						delete [] configs[i].interfaces;
					}
					delete configs[i].desc;
				}
			}
			delete [] configs;
		}
	}
}