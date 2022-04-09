/*
	USBConfigurator.cp
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
	Patrick Varilly		-	Fri, 3 Dec 99		-	Creation of file
*/

#include "USBConfigurator.h"
#include "USBCommandFactory.h"
#include "USBHCDriver.h"
#include "USBDevice.h"
#include "USBInterface.h"
#include "NKDebuggerNub.h"
#include "Macros.h"
#include "Time.h"

//#define PRINT_CONFIGS			// Uncomment to get config/interface/endpoint descriptors for all devices (Warning: this
								// is truly a lot of info, and can clog up your screen bad!)

USBConfigurator::USBConfigurator( USBBus* bus, ConstUSBDevID devID, Boolean fullSpeed ) :
	bus( bus ), defaultPipe( nil ), devID( devID ), fullSpeed( fullSpeed ),
	Thread( 4096, kPriorityDriver, "USB Device Configurator", true )
{
}

USBConfigurator::~USBConfigurator()
{
}

// This gathers all info available on a device (quite a long process) and then tries to couple a device
// with a driver (using static functions of USBDriver).
void
USBConfigurator::threadLoop()
{
	USBDevice				*device = nil;
	USBDeviceDescription		*dev = nil;
	USBErr					err;
	UInt32					i, j, k, l;
	
	// Set up dummy default pipe
	err = bus->driver->createControlPipe( defaultPipe, devID, 0, fullSpeed, 8 );
	if( err )
		goto error;
	
	// Get MPS of ep0
	USBDeviceDesc				dummyDesc;
	USBCommandFactory::getDescriptor( setup, kDescType_Device, 0, 0, 8 );
	err = sendCommand( &dummyDesc );
	if( err )
		goto error;
	
	// Create real default pipe
	err = bus->driver->deletePipe( defaultPipe );
	if( err )
		goto error;
	err = bus->driver->createControlPipe( defaultPipe, devID, 0, fullSpeed, dummyDesc.bMaxPacketSize0 );
	if( err )
		goto error;
	
	// Set up mem for new device description
	dev = new USBDeviceDescription( bus, devID, fullSpeed, defaultPipe );
	if( dev == nil )
		goto error;
	
	// Get device descriptor
	dev->desc = new USBDeviceDesc;
	if( dev->desc == nil )
		goto error;
	USBCommandFactory::getDescriptor( setup, kDescType_Device, 0, 0, sizeof( USBDeviceDesc ) );
	err = sendCommand( dev->desc );
	if( err )
		goto error;
	
	// Byte-reverse where necessary
	dev->desc->bcdUSB = LE16(dev->desc->bcdUSB);
	dev->desc->idVendor = LE16(dev->desc->idVendor);
	dev->desc->idProduct = LE16(dev->desc->idProduct);
	dev->desc->bcdDevice = LE16(dev->desc->bcdDevice);
	
	// Create configuration space in mem
	UInt8				numConfigs = dev->desc->bNumConfigurations;
	dev->configs = new USBConfiguration[numConfigs];
	if( dev->configs == nil )
		goto error;
	
	// Clear all configurations (for later partial deletion if out of mem)
	for( i = 0; i < numConfigs; i++ )
	{
		dev->configs[i].desc = nil;
		dev->configs[i].interfaces = nil;
	}
	
	// Now iterate getting all configurations and setting up all interfaces and their pipes
	for( i = 0; i < numConfigs; i++ )
	{
		USBConfiguration		*config = &dev->configs[i];
		
		// First get 18 bytes to get total length
		USBConfigDesc			partialDesc;
		USBCommandFactory::getDescriptor( setup, kDescType_Configuration, i, 0, sizeof( USBConfigDesc ) );
		err = sendCommand( &partialDesc );
		if( err )
			goto error;
		
		// Now get full descriptor
		config->desc = (USBConfigDesc*)new Int8[LE16(partialDesc.wTotalLength)];
		USBCommandFactory::getDescriptor( setup, kDescType_Configuration, i, 0, LE16(partialDesc.wTotalLength) );
		err = sendCommand( config->desc );
		if( err )
			goto error;
		
		// Byte-reverse where necessary
		config->desc->wTotalLength = LE16(config->desc->wTotalLength);
		
		// Allocate space for interfaces and clear them
		UInt8				numInterfaces = config->desc->bNumInterfaces;
		config->interfaces = new USBInterfaceDescription[numInterfaces];
		if( config->interfaces == nil )
			goto error;
		for( j = 0; j < numInterfaces; j++ )
		{
			config->interfaces[j].numSettings = 0;
			config->interfaces[j].curSetting = 0;
			config->interfaces[j].settings = nil;
		}
		
		// Now cycle through all the descriptors, counting the number of alternate settings for each interface
		Ptr					curPtr = (Ptr)config->desc;
		Ptr					endPtr = curPtr + config->desc->wTotalLength;
		while( curPtr < endPtr )
		{
			USBDescHeader		*header = (USBDescHeader*)curPtr;
			if( header->bDescriptorType == kDescType_Interface )
			{
				USBInterfaceDesc	*intDesc = (USBInterfaceDesc*)header;
				if( intDesc->bInterfaceNumber >= numInterfaces )
				{
					err = kUSBErr_Internal;
					goto error;
				}
				config->interfaces[intDesc->bInterfaceNumber].numSettings++;
			}
			
			curPtr += header->bLength;
		}
		
		// Allocate memory for settings
		for( j = 0; j < numInterfaces; j++ )
		{
			USBInterfaceDescription		*interface = &config->interfaces[j];
			interface->settings = new USBInterfaceSetting[interface->numSettings];
			if( interface->settings == nil )
				goto error;
			
			for( k = 0; k < interface->numSettings; k++ )
			{
				interface->settings[k].desc = nil;
				interface->settings[k].pipes = nil;
			}
		}
		
		// Couple each interface setting with its descriptor
		curPtr = (Ptr)config->desc;
		endPtr = curPtr + config->desc->wTotalLength;
		while( curPtr < endPtr )
		{
			USBDescHeader		*header = (USBDescHeader*)curPtr;
			if( header->bDescriptorType == kDescType_Interface )
			{
				USBInterfaceDesc	*intDesc = (USBInterfaceDesc*)header;
				USBInterfaceDescription	*interface = &config->interfaces[intDesc->bInterfaceNumber];
				if( interface->curSetting >= interface->numSettings )
				{
					err = kUSBErr_Internal;
					goto error;
				}
				interface->settings[interface->curSetting++].desc = intDesc;
			}
			
			curPtr += header->bLength;
		}
		
		// Create pipes for each setting (and set the current alternate setting to the first one)
		for( j = 0; j < numInterfaces; j++ )
		{
			USBInterfaceDescription		*interface = &config->interfaces[j];
			interface->curSetting = 0;
			
			for( k = 0; k < interface->numSettings; k++ )
			{
				USBInterfaceSetting	*setting = &interface->settings[k];
				UInt8			numPipes = setting->desc->bNumEndpoints;
				setting->pipes = new USBPipe[numPipes];
				if( setting->pipes == nil )
					goto error;
				
				// Clear pipes array
				for( l = 0; l < numPipes; l++ )
				{
					setting->pipes[l].desc = nil;
					setting->pipes[l].pipe = nil;
				}
				
				// Now couple each pipe with its descriptor and create the pipe on the HCD
				curPtr = (Ptr)setting->desc;
				for( l = 0; l < numPipes; l++ )
				{
					USBPipe				*pipe = &setting->pipes[l];
					
					// Seek for next endpoint descriptor
					USBDescHeader			*header = (USBDescHeader*)curPtr;
					while( header->bDescriptorType != kDescType_Endpoint )
					{
						curPtr += header->bLength;
						header = (USBDescHeader*)curPtr;
					}
					curPtr += header->bLength;
					
					// Match it up and byte-reverse where necessary
					pipe->desc = (USBEndpointDesc*)header;
					pipe->desc->wMaxPacketSize = LE16(pipe->desc->wMaxPacketSize);
					
					// Now create pipe in HCD
					err = kUSBErr_None;
					switch( pipe->desc->bmAttributes & kBmEPTypeMask )
					{
						case kBmEPControl:
							err = bus->driver->createControlPipe( pipe->pipe, devID, pipe->desc->bEndpointAddress,
								fullSpeed, pipe->desc->wMaxPacketSize );
							break;
						case kBmEPIsochronous:
							//err = bus->driver->createIsochronousPipe( pipe->pipe, devID, pipe->desc->bEndpointAddress,
							//	fullSpeed, pipe->desc->wMaxPacketSize );
							break;
						case kBmEPBulk:
							//err = bus->driver->createBulkPipe( pipe->pipe, devID, pipe->desc->bEndpointAddress,
							//	fullSpeed, pipe->desc->wMaxPacketSize );
							break;
						case kBmEPInterrupt:
							err = bus->driver->createInterruptPipe( pipe->pipe, devID, pipe->desc->bEndpointAddress,
								fullSpeed, pipe->desc->wMaxPacketSize, pipe->desc->bInterval );
							break;
					}
					if( err )
						goto error;
				}
			}
		}
	}
	
	// We should now have a complete device description
	
#ifdef PRINT_CONFIGS
	// Let's report it
	{
		NKCriticalSection		critical;		// Make sure no "interleaved" console emerges.
		usbOut << "\nDevice probed at address " << devID << "\n";
		usbOut << "USB version: " << (UInt8)(dev->desc->bcdUSB >> 8) << "." << (UInt8)dev->desc->bcdUSB << ", ";
		usbOut << "C/S/P: " << dev->desc->bDeviceClass << "/" << dev->desc->bDeviceSubClass
			<< "/" << dev->desc->bDeviceProtocol << ", ";
		usbOut << "EP0 MPS: " << dev->desc->bMaxPacketSize0 << "\n";
		usbOut << "Vendor: " << dev->desc->idVendor << ", Product: " << dev->desc->idProduct << ", Version: "
			<<  (UInt8)(dev->desc->bcdDevice >> 8) << "." << (UInt8)dev->desc->bcdDevice << "\n";
		
		for( i = 0; i < dev->desc->bNumConfigurations; i++ )
		{
			USBConfiguration	*config = &dev->configs[i];
			usbOut << "Configuration " << i << ":\n";
			usbOut << "\t#Ifs: " << config->desc->bNumInterfaces << ", Value: " << config->desc->bConfigurationValue << ", ";
			usbOut << "Attr: " << hexMsg << config->desc->bmAttributes << decMsg << ", ";
			usbOut << "Pwr: " << config->desc->MaxPower*2 << "mA\n";
			
			for( j = 0; j < config->desc->bNumInterfaces; j++ )
			{
				USBInterface	*interface = &config->interfaces[j];
				usbOut << "\tInterface " << j << ":\n";
				for( k = 0; k < interface->numSettings; k++ )
				{
					USBInterfaceSetting	*setting = &interface->settings[k];
					usbOut << "\t- Alt: " << setting->desc->bAlternateSetting << ", ";
					usbOut << "C/S/P: " << setting->desc->bInterfaceClass << "/" << setting->desc->bInterfaceSubClass
						<< "/" << setting->desc->bInterfaceProtocol << "\n";
					
					for( l = 0; l < setting->desc->bNumEndpoints; l++ )
					{
						USBPipe		*pipe = &setting->pipes[l];
						usbOut << "\t-- ";
						switch( pipe->desc->bmAttributes & kBmEPTypeMask )
						{
							case kBmEPControl:
								usbOut << "Control";
								break;
							case kBmEPIsochronous:
								usbOut << "Isochronous";
								break;
							case kBmEPBulk:
								usbOut << "Bulk";
								break;
							case kBmEPInterrupt:
								usbOut << "Interrupt";
								break;
						}
						usbOut << " Pipe at " << hexMsg << pipe->desc->bEndpointAddress << decMsg << ", ";
						usbOut << "MPS: " << pipe->desc->wMaxPacketSize << ", ";
						usbOut << "Int: " << pipe->desc->bInterval << "\n";
					}
				}
			}
		}
	}
#endif
	
	Boolean						interfaced;
	interfaced = (dev->desc->bDeviceClass == 0);
	
	// Match up to driver
	device = USBDeviceDriver::tryToBuildDevice( dev, interfaced );	// If interfaced, only allow product drivers to work
	
	// If we couldn't find a device-specific driver, then try to get to all its interfaces
	// First, create a common device driver, and get a valid configuration
	if( !device && interfaced )
	{
		USBInterfacedDevice			*intDevice;
		intDevice = new USBInterfacedDevice( dev );
		device = intDevice;
		if( intDevice )
		{
			// Get a proper configuration
			err = intDevice->configure();
			if( err )
			{
				delete intDevice;
				goto error;
			}
			
			// Walk through all the interfaces of the devices, trying to find a driver for each one
			USBConfiguration		*config;
			config = intDevice->getConfig( intDevice->getConfigNum() );
			FatalAssert( config != nil );
			
			for( i = 0; i < config->desc->bNumInterfaces; i++ )
			{
				USBInterfaceDescription	*description = &config->interfaces[i];
				USBInterface			*interface;
				UInt8				theSetting;
				interface = USBInterfaceDriver::tryToBuildInterface( description, intDevice, dev->desc->idVendor,
					dev->desc->idProduct, dev->desc->bcdDevice, config->desc->bConfigurationValue,
					i, theSetting );
				if( interface )
					intDevice->assignInterface( i, interface, theSetting );
			}
		}
	}
	
	// If still nothing, well, we did our best
	if( !device )
		delete dev;
	
	// Report to bus
	bus->deviceProbed( devID, device );
	if( device )
		device->resume();
	return;
	
error:
	if( device )
		delete device;			// This will take down device description with it
	else if( dev )
		delete dev;			// This will clean-up all of the allocations here;
	if( !err )
		err = kUSBErr_OutOfResources;
	usbOut << "Device " << devID << " of bus " << bus->bus() << " had problems probing: " << gUSBErrorStrings[err] << "\n";
	bus->deviceProbed( devID, nil );
}

USBErr
USBConfigurator::sendCommand( void* buffer )
{
	USBErr					err;
	USBTransaction				*trans;
	
	err = bus->driver->controlTransfer( trans, defaultPipe, &setup, (Ptr)buffer );
	if( err )
		return err;
	blockForIO( trans );
	err = (USBErr)trans->ioError();
	delete trans;
	
	return err;
}