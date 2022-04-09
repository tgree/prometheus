/*
	USBHub.h
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

#ifndef __USB_HUB__
#define __USB_HUB__

#include "USBDevice.h"
#include "NKThreads.h"

struct USBHubDescriptor;
typedef struct USBHubDescriptor USBHubDescriptor;

// Hub characteristics
enum
{
	kHubPowerSwitchingMask			= 0x0003,
	kHubPowerSwitchingGanged		= 0,
	kHubPowerSwitchingPerPort		= 1,
	kHubPowerSwitchingNone			= 2,
	kHubPowerSwitchingNeither		= 3,
	
	kHubCompoundDevice				= 0x0004
};

typedef enum HubFeature
{
	// Hub features
	kFeature_CHubLocalPower			= 0,
	kFeature_CHubOvercurrent		= 1,
	
	// Port features
	kFeature_PortConnection			= 0,
	kFeature_PortEnable				= 1,
	kFeature_PortSuspend			= 2,
	kFeature_PortOverCurrent		= 3,
	kFeature_PortReset				= 4,
	kFeature_PortPower				= 8,
	kFeature_PortLowSpeed			= 9,
	kFeature_CPortConnection			= 16,
	kFeature_CPortEnable			= 17,
	kFeature_CPortSuspend			= 18,
	kFeature_CPortOverCurrent		= 19,
	kFeature_CPortReset			= 20
} HubFeature;

// Hub Status.  The bits correspond to the feature selector numbers
enum
{
	kHubStatus_LocalPower			= (1<<kFeature_CHubLocalPower),
	kHubStatus_OverCurrent			= (1<<kFeature_CHubOvercurrent)
};
typedef struct HubStatus
{
	UInt16						wHubStatus;
	UInt16						wHubChange;
} HubStatus;

// Port Status
enum
{
	kPortStatus_Connection			= (1<<kFeature_PortConnection),
	kPortStatus_Enable				= (1<<kFeature_PortEnable),
	kPortStatus_Suspend			= (1<<kFeature_PortSuspend),
	kPortStatus_OverCurrent			= (1<<kFeature_PortOverCurrent),
	kPortStatus_Reset				= (1<<kFeature_PortReset),
	kPortStatus_Power				= (1<<kFeature_PortPower),
	kPortStatus_LowSpeed			= (1<<kFeature_PortLowSpeed)
};
typedef struct PortStatus
{
	UInt16						wPortStatus;
	UInt16						wPortChange;
} PortStatus;

class USBHub :
	public USBDevice
{
public:
								USBHub( USBDeviceDescription *description );
	virtual						~USBHub();
	
	virtual void					initialize();
	virtual void					deviceLoop();
	
protected:
	// Configuration descriptor
	virtual UInt8					getMaxPower();
	virtual Boolean					isSelfPowered();
	
	// Hub descriptor
	virtual UInt8					getNumPorts();
	virtual UInt16					getHubCharacteristics();
	virtual USBErr					setHubCharacteristics( UInt16 newCharacteristics );
	virtual UInt8					getPowerOnToPowerGoodTime();
	virtual UInt8					getHubControllerCurrent();
	virtual Boolean					isDeviceRemovable( UInt8 port );
	
	// Hub-specific control messages
	virtual USBErr					clearHubFeature( HubFeature feature );
	virtual USBErr					clearPortFeature( UInt8 port, HubFeature feature );
	virtual USBErr					getHubStatus( HubStatus& hubStatus );
	virtual USBErr					getPortStatus( UInt8 port, PortStatus& portStatus );
	virtual USBErr					setHubFeature( HubFeature feature );
	virtual USBErr					setPortFeature( UInt8 port, HubFeature feature );
	
	// Status Change processing
	virtual USBErr					waitForStatusChange();
	virtual Boolean					hasPortStatusChanged( UInt8 port );
	
	// Devices connected
	USBDevID						*devices;
	
	// Have we been configured?
	Boolean						configured;
	
private:
	USBPipe						*statusChangePipe;
	USBHubDescriptor				*hubDesc;
	UInt8						portStatChangeBits[32];
	Boolean						firstTime;
};

#endif /* __USB_HUB__ */