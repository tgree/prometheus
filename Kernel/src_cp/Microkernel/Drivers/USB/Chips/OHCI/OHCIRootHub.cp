/*
	OHCIRootHub.cp
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

#include "OHCIRootHub.h"
#include "NKDebuggerNub.h"

OHCIRootHub*
OHCIRootHub::createRootHub( USBBus* bus, OHCIDriver* driver )
{
	// Create a suitable device description
	USBDeviceDescription		*description;
	description = new USBDeviceDescription( bus, 1, true, nil );
	if( !description )
		return nil;
	
	// Create the hub
	OHCIRootHub				*rootHub;
	rootHub = new OHCIRootHub( description, driver );
	if( !rootHub )
		delete description;
	
	return rootHub;
}

OHCIRootHub::OHCIRootHub( USBDeviceDescription* description, OHCIDriver* driver )
	: USBHub( description ), ohci( driver )
{
}

OHCIRootHub::~OHCIRootHub()
{
	Panic( "OHCIRootHub's should *NEVER* be deleted!!!" );
}

void
OHCIRootHub::initialize()
{
	// Save descriptors A and B
	descriptorA = ohci->getRhDescriptorA();
	descriptorB = ohci->getRhDescriptorB();
	configured = true;
}

USBErr
OHCIRootHub::createDefaultPipe()
{
	// FIXME: defaultPipe = nil;
	return kUSBErr_None;
}

UInt8
OHCIRootHub::getMaxPower()
{
	return 0;
}

Boolean
OHCIRootHub::isSelfPowered()
{
	return true;
}

UInt8
OHCIRootHub::getNumPorts()
{
	return descriptorA & kNumDownstreamPortsMask;
}

UInt16
OHCIRootHub::getHubCharacteristics()
{
	return (descriptorA >> kHubCharacteristicsShift) & kHubCharacteristicsMask;
}

USBErr
OHCIRootHub::setHubCharacteristics( UInt16 newCharacteristics )
{
	descriptorA = (descriptorA & ~(kHubCharacteristicsMask << kHubCharacteristicsShift))
			| ((newCharacteristics << kHubCharacteristicsShift) & kHubCharacteristicsMask);
	ohci->setRhDescriptorA( descriptorA );
	return kUSBErr_None;
}

UInt8
OHCIRootHub::getPowerOnToPowerGoodTime()
{
	return (descriptorA >> kPowerOnToPowerGoodTimeShift);
}

UInt8
OHCIRootHub::getHubControllerCurrent()
{
	return 0;
}

Boolean
OHCIRootHub::isDeviceRemovable( UInt8 port )
{
	if( (port >= (descriptorA & kNumDownstreamPortsMask)) || (port == 0) )
		return true;
	return descriptorB & (1<<port);
}

USBErr
OHCIRootHub::clearHubFeature( HubFeature feature )
{
	USBErr				err = kUSBErr_None;
	UInt32				hubStatus;
	switch( feature )
	{
		case kFeature_CHubOvercurrent:
			hubStatus = kStatWrClearOverCurrentIndicatorChange;
			break;
		case kFeature_CHubLocalPower:
			break;
		default:
			err = kUSBErr_RequestError;
			break;
	}
	ohci->setRhStatus( hubStatus );
	
	return err;
}

USBErr
OHCIRootHub::clearPortFeature( UInt8 port, HubFeature feature )
{
	if( (port > (descriptorA & kNumDownstreamPortsMask)) || (port == 0) )
		return kUSBErr_RequestError;
	
	USBErr				err = kUSBErr_None;
	UInt32				portStatus = 0;
	switch( feature )
	{
		case kFeature_PortEnable:
			portStatus = kPStatWrClearPortEnable;
			break;
		case kFeature_PortSuspend:
			portStatus = kPStatWrClearSuspendStatus;
			break;
		case kFeature_PortPower:
			if( !(descriptorA & kNoPowerSwitching) && !(descriptorA & kPortsPowerSwitchedIndividually) )
				ohci->setRhStatus( kStatWrClearGlobalPower );
			else
				portStatus = kPStatWrClearPortPower;
			break;
		case kFeature_CPortConnection:
			portStatus = kPStatWrClearConnectStatusChange;
			break;
		case kFeature_CPortEnable:
			portStatus = kPStatWrClearPortEnableStatusChange;
			break;
		case kFeature_CPortSuspend:
			portStatus = kPStatWrClearPortSuspendStatusChange;
			break;
		case kFeature_CPortOverCurrent:
			portStatus = kPStatWrClearOverCurrentIndicatorChange;
			break;
		case kFeature_CPortReset:
			portStatus = kPStatWrClearResetStatusChange;
			break;
		default:
			err = kUSBErr_RequestError;
			break;
	}
	if( portStatus )
		ohci->setRhPortStatus( port-1, portStatus );
	
	return err;
}

USBErr
OHCIRootHub::getHubStatus( HubStatus& hubStatus )
{
	UInt32						rhStatus = ohci->getRhStatus();
	hubStatus.wHubStatus = rhStatus & 0x3;
	hubStatus.wHubChange = (rhStatus>>16) & 0x3;
	return kUSBErr_None;
}

USBErr
OHCIRootHub::getPortStatus( UInt8 port, PortStatus& portStatus )
{
	if( (port > (descriptorA & kNumDownstreamPortsMask)) || (port == 0) )
		return kUSBErr_RequestError;
	
	UInt32						rhPortStatus = ohci->getRhPortStatus(port-1);
	portStatus.wPortStatus = rhPortStatus & 0xFFFF;
	portStatus.wPortChange = rhPortStatus >> 16;
	return kUSBErr_None;
}

USBErr
OHCIRootHub::setHubFeature( HubFeature feature )
{
#pragma unused( feature )
	return kUSBErr_RequestError;
}

USBErr
OHCIRootHub::setPortFeature( UInt8 port, HubFeature feature )
{
	if( (port > (descriptorA & kNumDownstreamPortsMask)) || (port == 0) )
		return kUSBErr_RequestError;
	
	USBErr				err = kUSBErr_None;
	UInt32				portStatus = 0;
	switch( feature )
	{
		case kFeature_PortEnable:
			portStatus = kPStatWrSetPortEnable;
			break;
		case kFeature_PortSuspend:
			portStatus = kPStatWrSetPortSuspend;
			break;
		case kFeature_PortReset:
			portStatus = kPStatWrSetPortReset;
			break;
		case kFeature_PortPower:
			if( !(descriptorA & kNoPowerSwitching) && !(descriptorA & kPortsPowerSwitchedIndividually) )
				ohci->setRhStatus( kStatWrSetGlobalPower );
			else
				portStatus = kPStatWrSetPortPower;
			break;
		default:
			err = kUSBErr_RequestError;
			break;
	}
	if( portStatus )
		ohci->setRhPortStatus( port-1, portStatus );
	
	return err;
}

USBErr
OHCIRootHub::waitForStatusChange()
{
	// We assume this is only called within our own thread
	ohci->waitForRhStatusChange();
	return kUSBErr_None;
}

Boolean
OHCIRootHub::hasPortStatusChanged( UInt8 port )
{
	if( port > (descriptorA & kNumDownstreamPortsMask) )
		return false;
	
	if( port == 0 )
	{
		UInt32				hubStatus = ohci->getRhStatus();
		if( hubStatus & 0xFFFF0000 )
			return true;
		else
			return false;
	}
	else
	{
		UInt32				portStatus = ohci->getRhPortStatus(port-1);
		if( (portStatus >> 16) & 0x1F )
			return true;
		else
			return false;
	}
}