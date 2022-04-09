/*
	USBCommandFactory.h
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

#ifndef __USB_COMMAND_FACTORY__
#define __USB_COMMAND_FACTORY__

#include "USB.h"

class USBCommandFactory
{
public:
	// Standard requests
	static void				clearDeviceFeature( USBControlSetup& setup, UInt16 feature );
	static void				clearInterfaceFeature( USBControlSetup& setup, UInt8 interface, UInt16 feature );
	static void				clearInEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature );
	static void				clearOutEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature );
	
	static void				setDeviceFeature( USBControlSetup& setup, UInt16 feature );
	static void				setInterfaceFeature( USBControlSetup& setup, UInt8 interface, UInt16 feature );
	static void				setInEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature );
	static void				setOutEndpointFeature( USBControlSetup& setup, UInt8 endpoint, UInt16 feature );
	
	static void				getConfiguration( USBControlSetup& setup );
	static void				setConfiguration( USBControlSetup& setup, UInt8 config );
	
	static void				getDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
								UInt16 dataSize );
	static void				setDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
								UInt16 dataSize );
	
	static void				getInterface( USBControlSetup& setup, UInt8 interface );
	static void				setInterface( USBControlSetup& setup, UInt8 interface, UInt8 setting );
	
	static void				getDeviceStatus( USBControlSetup& setup );
	static void				getInterfaceStatus( USBControlSetup& setup, UInt8 interface );
	static void				getInEndpointStatus( USBControlSetup& setup, UInt8 endpoint );
	static void				getOutEndpointStatus( USBControlSetup& setup, UInt8 endpoint );
	
	static void				setAddress( USBControlSetup& setup, UInt8 address );
	
	static void				synchFrameIn( USBControlSetup& setup, UInt8 endpoint );
	static void				synchFrameOut( USBControlSetup& setup, UInt8 endpoint );
	
	// General class requests
	static void				classClearDeviceFeature( USBControlSetup& setup, UInt16 feature );
	static void				classSetDeviceFeature( USBControlSetup& setup, UInt16 feature );
	
	static void				classGetDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
								UInt16 dataSize );
	static void				classSetDescriptor( USBControlSetup& setup, UInt8 type, UInt8 index, UInt16 langID,
								UInt16 dataSize );
	
	// Hub requests
	static void				hubClearPortFeature( USBControlSetup& setup, UInt8 port, UInt16 feature );
	static void				hubSetPortFeature( USBControlSetup& setup, UInt8 port, UInt16 feature );
	
	static void				hubGetHubStatus( USBControlSetup& setup );
	static void				hubGetPortStatus( USBControlSetup& setup, UInt8 port );
	
		// *** GetState purposely missed out;  it's optional and of little utility
	
	// HID requests
	static void				hidSetIdle( USBControlSetup& setup, UInt8 interfaceNum, UInt8 duration = 0,
								UInt8 reportID = 0 );
	static void				hidSetProtocol( USBControlSetup& setup, UInt8 interfaceNum, Boolean boot );
};

#endif /* __USB_COMMAND_FACTORY__ */