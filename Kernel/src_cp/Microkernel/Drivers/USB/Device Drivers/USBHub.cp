/*
	USBHub.cp
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

#include "USBHub.h"
#include "USBDriver.h"
#include "USBCommandFactory.h"
#include "NKDebuggerNub.h"
#include "Streams.h"
#include "Macros.h"

class USBHubDriver : public USBDeviceDriver
{
public:
							USBHubDriver();
	virtual					~USBHubDriver();
	
	virtual USBDevice*			buildDevice( USBDeviceDescription *description );
};

static USBHubDriver				hubDriver;

USBHubDriver::USBHubDriver()
{
	registerClassDriver( kClassHub, kSubClassNone );
}

USBHubDriver::~USBHubDriver()
{
}

USBDevice*
USBHubDriver::buildDevice( USBDeviceDescription *description )
{
	return new USBHub( description );
}

// *** Hub driver start here ***

enum
{
	kDescType_Hub = 0x29
};

#pragma options align=mac68k
struct USBHubDescriptor
{
	USBDescHeader				header;
	UInt8					bNbrPorts;
	UInt16					wHubCharacteristics;
	UInt8					bPwrOn2PwrGood;
	UInt8					bHubContrCurrent;
	UInt8					masks[0];
};
#pragma options align=reset

USBHub::USBHub( USBDeviceDescription* description ) :
	USBDevice( description, "USB Hub" )
	// Thread deletes itself (taking down USBDevice with it) when disconnection occurs
{
//	firstTime = true;
	hubDesc = nil;
}

USBHub::~USBHub()
{
	if( hubDesc )
		delete hubDesc;
}

void
USBHub::initialize()
{
	configured = false;
	
	// Select first configuration which doesn't give an error
	USBErr				err;
	UInt8				numConfigs = getNumConfigs();
	UInt32				i;
	for( i = 0; i < numConfigs; i++ )
	{
		err = setConfig( i );
		if( !err )
			break;
	}
	if( err )
	{
		usbOut << "Hub (" << address() << "," << bus()->bus() << ") had no valid configurations\n";
		return;
	}
	
	// We're configured!  Get hub descriptor
	USBControlSetup	cmd;
	USBHubDescriptor	desc;
	USBCommandFactory::classGetDescriptor( cmd, kDescType_Hub, 0, 0, sizeof(USBHubDescriptor) );
	err = sendClassCommand( &cmd, &desc );
	if( err )
	{
		usbOut << "Hub driver couldn't get partial hub descriptor, err \"" << gUSBErrorStrings[err] << "\"\n";
		return;
	}
	
	// Get full hub descriptor
	hubDesc = (USBHubDescriptor*)new Int8[desc.header.bLength];
	if( hubDesc == nil )
	{
		usbOut << "Hub driver couldn't allocate mem for hub descriptor, err \"" << gUSBErrorStrings[err] << "\"\n";
		return;
	}
	USBCommandFactory::classGetDescriptor( cmd, kDescType_Hub, 0, 0, desc.header.bLength );
	err = sendClassCommand( &cmd, hubDesc );
	if( err )
	{
		usbOut << "Hub driver couldn't get full hub descriptor, err \"" << gUSBErrorStrings[err] << "\"\n";
		return;
	}
	
	// Byte-reverse where necessary
	hubDesc->wHubCharacteristics = LE16(hubDesc->wHubCharacteristics);
	
	// Find status change pipe
	statusChangePipe = &getConfig(i)->interfaces[0].settings[0].pipes[0];
	if( statusChangePipe == nil )
	{
		usbOut << "Hub driver: No status change pipe!!!";
		return;
	}
	
	// Done!
	configured = true;
}

UInt8
USBHub::getMaxPower()
{
	return getConfig( getConfigNum() )->desc->MaxPower;
}

Boolean
USBHub::isSelfPowered()
{
	return getConfig( getConfigNum() )->desc->bmAttributes & kConfigSelfPowered;
}

UInt8
USBHub::getNumPorts()
{
	return hubDesc->bNbrPorts;
}

UInt16
USBHub::getHubCharacteristics()
{
	return hubDesc->wHubCharacteristics;
}

USBErr
USBHub::setHubCharacteristics( UInt16 newCharacteristics )
{
#pragma unused(newCharacteristics)
	return kUSBErr_RequestError;			// At least for now
}

UInt8
USBHub::getPowerOnToPowerGoodTime()
{
	return hubDesc->bPwrOn2PwrGood;
}

UInt8
USBHub::getHubControllerCurrent()
{
	return hubDesc->bHubContrCurrent;
}

Boolean
USBHub::isDeviceRemovable( UInt8 port )
{
	UInt8				byte, bit;
	byte = port/8;
	bit = port - byte*8;
	return hubDesc->masks[byte] & (0x80 >> bit);
}

USBErr
USBHub::clearHubFeature( HubFeature feature )
{
	USBControlSetup		cmd;
	USBCommandFactory::classClearDeviceFeature( cmd, feature );
	return sendClassCommand( &cmd );
}

USBErr
USBHub::clearPortFeature( UInt8 port, HubFeature feature )
{
	USBControlSetup		cmd;
	USBCommandFactory::hubClearPortFeature( cmd, port, feature );
	return sendClassCommand( &cmd );
}

USBErr
USBHub::getHubStatus( HubStatus& hubStatus )
{
	USBControlSetup		cmd;
	USBErr				err;
	UInt32				stat;
	
	USBCommandFactory::hubGetHubStatus( cmd );
	err = sendClassCommand( &cmd, &stat );
	if( err )
		return err;
	hubStatus.wHubStatus = LE16(stat>>16);
	hubStatus.wHubChange = LE16(stat & 0xFFFF);
	return err;
}

USBErr
USBHub::getPortStatus( UInt8 port, PortStatus& portStatus )
{
	USBControlSetup		cmd;
	USBErr				err;
	UInt32				stat;
	
	USBCommandFactory::hubGetPortStatus( cmd, port );
	err = sendClassCommand( &cmd, &stat );
	if( err )
		return err;
	portStatus.wPortStatus = LE16(stat>>16);
	portStatus.wPortChange = LE16(stat & 0xFFFF);
	return err;
}

USBErr
USBHub::setHubFeature( HubFeature feature )
{
	USBControlSetup		cmd;
	USBCommandFactory::classSetDeviceFeature( cmd, feature );
	return sendClassCommand( &cmd );
}

USBErr
USBHub::setPortFeature( UInt8 port, HubFeature feature )
{
	USBControlSetup		cmd;
	USBCommandFactory::hubSetPortFeature( cmd, port, feature );
	return sendClassCommand( &cmd );
}

USBErr
USBHub::waitForStatusChange()
{
	/*if( firstTime )
	{
		// Simply flag all of portStatChangeBits
		UInt8			byte;
		for( byte = 0; byte < 32; byte++ )
			portStatChangeBits[byte] = 0xFF;
		firstTime = false;
		return kUSBErr_None;
	}
	else
	{*/
		// Figure out how long the transmission should be
		UInt8			size;
		size = (hubDesc->bNbrPorts+1+7)/8;		// Round up and include hub itself
		return interruptTransfer( statusChangePipe, portStatChangeBits, size );
	//}
}

Boolean
USBHub::hasPortStatusChanged( UInt8 port )
{
	UInt8				byte, bit;
	Boolean				change;
	byte = (hubDesc->bNbrPorts+1)/8 - port/8;
	bit = port % 8;
	change = portStatChangeBits[byte] & (1 << bit);
	portStatChangeBits[byte] &= 0xFF & ~(1 << bit);
	return change;
}

void
USBHub::deviceLoop()
{
	UInt32				i;
	
	// Exit if configuration failed
	if( !configured )
		return;
	
	// Get info
	UInt32				numPorts;
	UInt16				characteristics;
	UInt32				potpgt;
	numPorts = getNumPorts();
	characteristics = getHubCharacteristics();
	potpgt = getPowerOnToPowerGoodTime();
	
	// Create children array
	devices = new USBDevID[numPorts];
	FatalAssert( devices != nil );
	for( i = 0; i < numPorts; i++ )
		devices[i] = 0;
	
	// Switch all ports on now (ignore errors for now)
	for( i = 1; i <= numPorts; i++ )
		setPortFeature( i, kFeature_PortPower );
	
	// Wait until the power is good
	sleepMS( potpgt*2 );
	
	// Init probing variables
	Boolean				haveProbeLock = false;
	Boolean				*portNeedsProbing;
	UInt8				portBeingProbed = 0;
	portNeedsProbing = new Boolean[numPorts];
	FatalAssert( portNeedsProbing != nil );
	for( i = 0; i < numPorts; i++ )
		portNeedsProbing[i] = false;
	
	// Loop until we're removed (N.B. This shouldn't happen yet)
	PortStatus			*portStatChanges;
	portStatChanges = new PortStatus[numPorts];
	FatalAssert( portStatChanges != nil );
	HubStatus				hubStatChanges;
	
	while( connected() )
	{
		// Clear status changes
		for( i = 0; i < numPorts; i++ )
			portStatChanges[i].wPortChange = 0;
		hubStatChanges.wHubChange = 0;
		
		// Wait for a status change
		waitForStatusChange();
		
		// Gather all port changes
		Boolean			stop;
		PortStatus		portStat;
		HubStatus			hubStat;
		do
		{
			stop = true;
			
			// Check hub
			if( hasPortStatusChanged( 0 ) )
			{
				stop = false;
				getHubStatus( hubStat );
				hubStatChanges.wHubChange |= hubStat.wHubChange;
				hubStatChanges.wHubStatus = hubStat.wHubStatus;
				
				// Clear what's necessary
				if( hubStat.wHubChange & kHubStatus_LocalPower )
					clearHubFeature( kFeature_CHubLocalPower );
				if( hubStat.wHubChange & kHubStatus_OverCurrent )
					clearHubFeature( kFeature_CHubOvercurrent );
			}
			
			// Check each port
			for( i = 0; i < numPorts; i++ )
			{
				if( hasPortStatusChanged( i+1 ) )
				{
					stop = false;
					getPortStatus( i+1, portStat );
					portStatChanges[i].wPortChange |= portStat.wPortChange;
					portStatChanges[i].wPortStatus = portStat.wPortStatus;
					
					// Clear what's necessary
					if( portStat.wPortChange & kPortStatus_Connection )
						clearPortFeature( i+1, kFeature_CPortConnection );
					if( portStat.wPortChange & kPortStatus_Enable )
						clearPortFeature( i+1, kFeature_CPortEnable );
					if( portStat.wPortChange & kPortStatus_Suspend )
						clearPortFeature( i+1, kFeature_CPortSuspend );
					if( portStat.wPortChange & kPortStatus_OverCurrent )
						clearPortFeature( i+1, kFeature_CPortOverCurrent );
					if( portStat.wPortChange & kPortStatus_Reset )
						clearPortFeature( i+1, kFeature_CPortReset );
				}
			}
		} while( !stop );
		
		// React to port changes
		// *** Ignore hub for now
		
		// Rect to each ports status changes
		Boolean						attachDetect = false;
		for( i = 0; i < numPorts; i++ )
		{
			// Check connection
			if( portStatChanges[i].wPortChange & kPortStatus_Connection )
			{
				if( portStatChanges[i].wPortStatus & kPortStatus_Connection )
				{
					// Signal need for probing and resetting
					usbOut << "Device connected to port " << i+1 << " of hub (" << address() << "," << bus()->bus() << ")\n";
					portNeedsProbing[i] = true;
					attachDetect = true;
				}
				else
				{
					// Remove need for probing
					usbOut << "Device disconnected from port " << i+1 << " of hub (" << address() << "," << bus()->bus() << ")\n";
					bus()->deviceRemoved( devices[i] );		// device will delete itself when ready
					devices[i] = nil;
					portNeedsProbing[i] = false;
					
					if( i == (portBeingProbed-1) )
						portBeingProbed = 0;
				}
			}
			
			// Check for reset completion
			if( portStatChanges[i].wPortChange & kPortStatus_Reset )
			{
				if( i != (portBeingProbed-1) )
					usbOut << "USB Hub: Reset completed on a port which didn't receive a reset!" << newLine;
				else
				{
					// We have to wait up to 10ms for "reset recovery"... Don't the delays and waits ever end???
					sleepMS( 10 );
					
					// The device should be ready and respond to default address.  Let USBBus take over now
					USBDevID				addr;
					addr = bus()->deviceAdded( !(portStatChanges[i].wPortStatus & kPortStatus_LowSpeed) );
					portNeedsProbing[i] = false;
					portBeingProbed = 0;
					devices[i] = addr;
					
					if( addr == 0 )
						usbOut << "Port " << i+1 << " had problems probing\n";
				}
			}
		}
		
		// If a device has just been attached, we need to wait 100ms for the power supply to stabilize (see USB 1.1, 7.1.7.1)
		if( attachDetect )
			sleepMS( 100 );
		
		// Check if we need to acquire or release probe lock
		Boolean				needProbeLock;
		UInt32				portToReset = 0;
		for( i = 0; i < numPorts; i++ )
		{
			if( portNeedsProbing[i] )
			{
				// Check to see if we can probe it now
				if( portBeingProbed == 0 )
					portToReset = portBeingProbed = i+1;
				break;
			}
		}
		
		if( i == numPorts )
		{
			portBeingProbed = 0;
			needProbeLock = false;
		}
		else
			needProbeLock = true;
		if( needProbeLock && !haveProbeLock )
			bus()->probeLock.lock();
		else if( haveProbeLock && !needProbeLock )
			bus()->probeLock.unlock();
		haveProbeLock = needProbeLock;
		
		// Do reset if necessary
		if( portToReset )
			setPortFeature( portToReset, kFeature_PortReset );
	}
	
	if( haveProbeLock )
		bus()->probeLock.unlock();
	
	// Report all children as disconnected
	for( i = 0; i < numPorts; i++ )
	{
		if( devices[i] )
			bus()->deviceRemoved( devices[i] );
	}
}