/*
	OHCIPCI.cp
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
	Other sources			Project				Author			Notes
	===========			======				=====			====
	uusbd/ohci/driver.c		Linux PowerPC			I–aky PŽrez Gonz‡lez	Quirks for Opti FireLink taken from here.  Not quite
															sure what they actually do, though...
	
	Version History
	============
	Patrick Varilly		-	Fri, 26 Nov 99		-	Original creation of file
*/

#include "PCI.h"
#include "OHCIGenericDriver.h"
#include "External Interrupt.h"
#include "NKMachineInit.h"
#include "Chip Debugger.h"
#include "Streams.h"

struct OHCIProber : public PCIProber
{
							OHCIProber();
	virtual Boolean				probe( PCIDevice* pciDevice );
};

class OHCIPCIDriver :
	public OHCIGenericDriver,
	public InterruptHandler
{
protected:
	friend class OHCIProber;
	
							OHCIPCIDriver( PCIDevice* dev );
	virtual					~OHCIPCIDriver();
	
	virtual void				handleInterrupt();
	
	virtual void				start();
	virtual void				stop();
};

static OHCIProber				ohciProber;

OHCIProber::OHCIProber()
{
}

Boolean
OHCIProber::probe( PCIDevice* pciDevice )
{
#if USB_DRIVERS == 0
	return false;
#endif
	if( (pciDevice->readClass() == 0x0C03)		// USB Controller
	&& (pciDevice->readRegLevel() == 0x10) )	// OHCI-compliant USB Controller
	{
		OHCIDriver				*driver;
		driver = new OHCIPCIDriver( pciDevice );
		machine.driverList.enqueue( driver );
		new USBBus( driver );
		return true;
	}
	return false;
}

OHCIPCIDriver::OHCIPCIDriver( PCIDevice* dev ) :
	InterruptHandler(dev->readInterruptLine())
{
	// * (OHCI 5.1.1.1) Load and locate
	regs = (OHCIRegs*)dev->mapBaseAddr(0);
	
	// Set up for Chip Debugging (WARNING:  Do *not* chip debug (a) and hot-swap or (b) if you're using USB devices to
	// interact with the Chip Debugger [doh!])
	new Chip( "OHCI USB Controller", ohciRegisterDescriptor, regs );
	
	// Ensure control is set up properly
	UInt32				command;
	command = dev->readCommand();
	command |= 0x6;		// Bus master and memory access bits must both be set
	if( (dev->readVendorID() == 0x1045) && (dev->readDeviceID() == 0xc861) )	// Opti FireLink PCI-to-USB bridge
	{
		// Enable SERR# and PERR# (this comes directly from Linux's uusbd, not exactly sure what it's for)
		command |= 0x0140;
	}
	dev->writeCommand( command );
	
	// Set latency to 24 PCI clocks, as per OHCI 1.0A specs
	dev->writeLatency( 0x16 );
}

OHCIPCIDriver::~OHCIPCIDriver()
{
}

void
OHCIPCIDriver::handleInterrupt()
{
	ohciInterrupt();
}

void
OHCIPCIDriver::start()
{
	OHCIGenericDriver::start();
	
	enable();
}

void
OHCIPCIDriver::stop()
{
	disable();
	
	OHCIGenericDriver::stop();
}