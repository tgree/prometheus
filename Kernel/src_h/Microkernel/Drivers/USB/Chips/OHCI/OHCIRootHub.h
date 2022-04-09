/*
	OHCIRootHub.h
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

#ifndef __OHCI_ROOT_HUB__
#define __OHCI_ROOT_HUB__

#include "USBHub.h"
#include "OHCIDriver.h"

// Bits, masks and shifts in Root Hub Partition of Registers
enum
{
	// RhDescriptorA
	kNumDownstreamPortsMask = 0x000000FF,
	
	kHubCharacteristicsShift = 8,
	kHubCharacteristicsMask = 0x1F,
	kNoPowerSwitching = 0x00000100,
	kPortsPowerSwitchedIndividually = 0x00000200,
	kDeviceType = 0x00000400,
	kOverCurrentPerPort = 0x00000800,
	kNoOverCurrentProtection = 0x00001000,
	
	kPowerOnToPowerGoodTimeShift = 24,
	
	// RhDescriptorB
	kDeviceRemovableMask = 0x0000FFFF,
	//kPowerPortControlMaskShift = 16,		// ignored in USB 1.1
	
	// RhStatus (bits that are OHCI specific)
	kStatWrClearGlobalPower = 0x00000001,
	kStatRdDeviceRemoteWakeupEnable = 0x00008000,
	kStatWrSetRemoteWakeupEnable = 0x00008000,
	kStatWrSetGlobalPower = 0x00010000,
	kStatWrClearOverCurrentIndicatorChange = 0x00020000,
	kStatWrClearRemoteWakeupEnable = 0x80000000,
	
	// RhPortStatus (bits that are OHCI specific)
	kPStatWrClearPortEnable = 0x00000001,
	kPStatWrSetPortEnable = 0x00000002,
	kPStatWrSetPortSuspend = 0x00000004,
	kPStatWrClearSuspendStatus = 0x00000008,
	kPStatWrSetPortReset = 0x00000010,
	kPStatWrSetPortPower = 0x00000100,
	kPStatWrClearPortPower = 0x00000200,
	kPStatWrClearConnectStatusChange = 0x00010000,
	kPStatWrClearPortEnableStatusChange = 0x00020000,
	kPStatWrClearPortSuspendStatusChange = 0x00040000,
	kPStatWrClearOverCurrentIndicatorChange = 0x00080000,
	kPStatWrClearResetStatusChange = 0x00100000
};

class OHCIRootHub :
	public USBHub
{
public:
	static OHCIRootHub*				createRootHub( USBBus* bus, OHCIDriver* driver );
	
								OHCIRootHub( USBDeviceDescription *descrition, OHCIDriver* driver );
	virtual						~OHCIRootHub();
	
	virtual void					initialize();
	
protected:
	virtual USBErr					createDefaultPipe();
	
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
	
private:
	UInt32						descriptorA, descriptorB;
	OHCIDriver					*ohci;
	Boolean						lastOverCurrentIndicator;
};

#endif /* __OHCI_ROOT_HUB__ */