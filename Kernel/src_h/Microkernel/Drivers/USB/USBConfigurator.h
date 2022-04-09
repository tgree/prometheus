/*
	USBConfigurator.h
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

#ifndef __USB_CONFIGURATOR__
#define __USB_CONFIGURATOR__

#include "USB.h"
#include "NKThreads.h"
#include "USBDriver.h"

class USBConfigurator :
	public Thread
{
public:
							USBConfigurator( USBBus* bus, ConstUSBDevID devID, Boolean fullSpeed );
	virtual					~USBConfigurator();
	
	virtual void				threadLoop();
	
private:
	USBErr					sendCommand( void* buffer = nil );
	
	USBControlSetup			setup;
	USBBus					*bus;
	USBHCPipe				*defaultPipe;
	ConstUSBDevID				devID;
	Boolean					fullSpeed;
};

#endif /* __USB_CONFIGURATOR__ */