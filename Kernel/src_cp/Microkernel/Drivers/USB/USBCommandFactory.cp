/*
	USBCommandFactory.cp
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
	Patrick Varilly		-	Tue, 30 Nov 99		-	Creation of file
*/

#include "USBCommandFactory.h"
#include "Macros.h"

// *** Standard requests ***

#pragma mark Standard requests
#pragma mark ==============

void
USBCommandFactory::clearDeviceFeature( USBControlSetup& setup, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::clearInterfaceFeature( USBControlSetup& setup, UInt8 interface, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientInterface;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = LE16(interface);
	setup.wLength = 0;
}

void
USBCommandFactory::clearInEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = IN_EN_TO_INDEX(endpoint);
	setup.wLength = 0;
}

void
USBCommandFactory::clearOutEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = OUT_EN_TO_INDEX(endpoint);
	setup.wLength = 0;
}

void
USBCommandFactory::getConfiguration( USBControlSetup& setup )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBGetConfiguration;
	setup.wValue = 0;
	setup.wIndex = 0;
	setup.wLength = LE16(1);
}

void
USBCommandFactory::getDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID, UInt16 dataSize )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBGetDescriptor;
	setup.wValue = LE16((type << 8) | index);
	setup.wIndex = LE16(langID);
	setup.wLength = LE16(dataSize);
}

void
USBCommandFactory::getInterface( USBControlSetup& setup, UInt8 interface )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientInterface;
	setup.bRequest = kUSBGetInterface;
	setup.wValue = 0;
	setup.wIndex = LE16(interface);
	setup.wLength = LE16(1);
}

void
USBCommandFactory::getDeviceStatus( USBControlSetup& setup )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = 0;
	setup.wLength = LE16(2);
}

void
USBCommandFactory::getInterfaceStatus( USBControlSetup& setup, UInt8 interface )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientInterface;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = LE16(interface);
	setup.wLength = LE16(2);
}

void
USBCommandFactory::getInEndpointStatus( USBControlSetup& setup, UInt8 endpoint )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = IN_EN_TO_INDEX(endpoint);
	setup.wLength = LE16(2);
}

void
USBCommandFactory::getOutEndpointStatus( USBControlSetup& setup, UInt8 endpoint )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = OUT_EN_TO_INDEX(endpoint);
	setup.wLength = LE16(2);
}

void
USBCommandFactory::setAddress( USBControlSetup& setup, UInt8 address )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBSetAddress;
	setup.wValue = LE16(address);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::setConfiguration( USBControlSetup& setup, UInt8 config )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBSetConfiguration;
	setup.wValue = LE16(config);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::setDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID, UInt16 dataSize )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBSetDescriptor;
	setup.wValue = LE16((type << 8) | index);
	setup.wIndex = LE16(langID);
	setup.wLength = LE16(dataSize);
}

void
USBCommandFactory::setDeviceFeature( USBControlSetup& setup, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientDevice;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::setInterfaceFeature( USBControlSetup& setup, UInt8 interface, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientInterface;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = LE16(interface);
	setup.wLength = 0;
}

void
USBCommandFactory::setInEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = IN_EN_TO_INDEX(endpoint);
	setup.wLength = 0;
}

void
USBCommandFactory::setOutEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = OUT_EN_TO_INDEX(endpoint);
	setup.wLength = 0;
}

void
USBCommandFactory::setInterface( USBControlSetup& setup, UInt8 interface, UInt8 setting )
{
	setup.bmRequestType = kBmDownstream | kBmTypeStandard | kBmRecipientInterface;
	setup.bRequest = kUSBSetInterface;
	setup.wValue = LE16(setting);
	setup.wIndex = LE16(interface);
	setup.wLength = 0;
}

void
USBCommandFactory::synchFrameIn( USBControlSetup& setup, UInt8 endpoint )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBSynchFrame;
	setup.wValue = 0;
	setup.wIndex = IN_EN_TO_INDEX(endpoint);
	setup.wLength = LE16(2);
}

void
USBCommandFactory::synchFrameOut( USBControlSetup& setup, UInt8 endpoint )
{
	setup.bmRequestType = kBmUpstream | kBmTypeStandard | kBmRecipientEndpoint;
	setup.bRequest = kUSBSynchFrame;
	setup.wValue = 0;
	setup.wIndex = OUT_EN_TO_INDEX(endpoint);
	setup.wLength = LE16(2);
}

// *** General class requests ***

#pragma mark Ê
#pragma mark General class requests
#pragma mark =================

void
USBCommandFactory::classClearDeviceFeature( USBControlSetup& setup, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientDevice;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::classSetDeviceFeature( USBControlSetup& setup, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientDevice;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = 0;
	setup.wLength = 0;
}

void
USBCommandFactory::classGetDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
	UInt16 dataSize )
{
	setup.bmRequestType = kBmUpstream | kBmTypeClass | kBmRecipientDevice;
	setup.bRequest = kUSBGetDescriptor;
	setup.wValue = LE16((type << 8) | index);
	setup.wIndex = LE16(langID);
	setup.wLength = LE16(dataSize);
}

void
USBCommandFactory::classSetDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
	UInt16 dataSize )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientDevice;
	setup.bRequest = kUSBSetDescriptor;
	setup.wValue = LE16((type << 8) | index);
	setup.wIndex = LE16(langID);
	setup.wLength = LE16(dataSize);
}

// *** Hub requests ***

#pragma mark Ê
#pragma mark Hub requests
#pragma mark ==========

void
USBCommandFactory::hubClearPortFeature( USBControlSetup& setup, UInt8 port, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientOther;
	setup.bRequest = kUSBClearFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = LE16(port);
	setup.wLength = 0;
}

void
USBCommandFactory::hubGetHubStatus( USBControlSetup& setup )
{
	setup.bmRequestType = kBmUpstream | kBmTypeClass | kBmRecipientDevice;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = 0;
	setup.wLength = LE16(4);
}

void
USBCommandFactory::hubGetPortStatus( USBControlSetup& setup, UInt8 port )
{
	setup.bmRequestType = kBmUpstream | kBmTypeClass | kBmRecipientOther;
	setup.bRequest = kUSBGetStatus;
	setup.wValue = 0;
	setup.wIndex = LE16(port);
	setup.wLength = LE16(4);
}

void
USBCommandFactory::hubSetPortFeature( USBControlSetup& setup, UInt8 port, UInt16 feature )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientOther;
	setup.bRequest = kUSBSetFeature;
	setup.wValue = LE16(feature);
	setup.wIndex = LE16(port);
	setup.wLength = 0;
}

// *** HID requests ***

#pragma mark Ê
#pragma mark HID requests
#pragma mark ==========

void
USBCommandFactory::hidSetIdle( USBControlSetup& setup, UInt8 interfaceNum, UInt8 duration, UInt8 reportID )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientInterface;
	setup.bRequest = 0x0A;
	setup.wValue = LE16((duration << 8) | reportID);
	setup.wIndex = LE16(interfaceNum);
	setup.wLength = 0;
}

void
USBCommandFactory::hidSetProtocol( USBControlSetup& setup, UInt8 interfaceNum, Boolean boot )
{
	setup.bmRequestType = kBmDownstream | kBmTypeClass | kBmRecipientInterface;
	setup.bRequest = 0x0B;
	if( boot )
		setup.wValue = 0;
	else
		setup.wValue = LE16(1);
	setup.wIndex = LE16(interfaceNum);
	setup.wLength = 0;
}