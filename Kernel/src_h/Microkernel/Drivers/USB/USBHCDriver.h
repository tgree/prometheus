/*
	USBHCDriver.h
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

#ifndef __USB_HC_DRIVER__
#define __USB_HC_DRIVER__

#include "USB.h"

class USBHub;
class USBHCDriver : public Driver
{
public:
						USBHCDriver( ConstASCII8Str driverName );
	virtual				~USBHCDriver();
	
	// Root hub (into slot 1, if it's integrated into the driver.  Otherwise, it's probed by the USB driver on top)
	virtual USBHub*		getRootHub( USBBus* bus ) = 0;
	
	// General USB protocol handling 
	virtual UInt64			getFrameNumber() = 0;		// Will overflow after about 584.5 million years... dang!
	
	// Pipe management
	virtual USBErr			createControlPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
							ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize ) = 0;
	//virtual USBErr		createBulkPipe( USBHCPipe*& outPipe, ConstUSBDevID address, ConstUSBEndpointAddress endpoint,
	//						const Boolean fullSpeed, const UInt16 maxPacketSize ) = 0;
	virtual USBErr			createInterruptPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
							ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize,
							const UInt32 interval ) = 0;
							// interval is number of ms between polls.  Actual rate guaranteed to be less or equal.
	//virtual USBErr		createIsochronousPipe( USBHCPipe*& outPipe, ConstUSBDevID address,
	//						ConstUSBEndpointAddress endpoint, const Boolean fullSpeed, const UInt16 maxPacketSize ) = 0;
	virtual USBErr			abortPipe( USBHCPipe* pipe ) = 0;
	virtual USBErr			resetPipe( USBHCPipe* pipe ) = 0;
	virtual USBErr			pausePipe( USBHCPipe* pipe ) = 0;
	virtual USBErr			resumePipe( USBHCPipe* pipe ) = 0;
	virtual USBErr			deletePipe( USBHCPipe* pipe ) = 0;
	
	// Transactions
	virtual USBErr			controlTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe,
							ConstUSBControlSetupPtr setup, ConstPtr buffer = nil, Boolean shortResponseOK = false ) = 0;
	virtual USBErr			interruptTransfer( USBTransaction*& outTransaction, USBHCPipe* pipe,
							ConstPtr buffer, const UInt32 bufferSize, Boolean shortResponseOK = false ) = 0;
	
	// *** Bulk and Isochronous interfaces pending
};

#endif /* __USB_HC_DRIVER__ */