/*
	USBDevice.cp
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

#include "USBDevice.h"
#include "USBHCDriver.h"
#include "USBCommandFactory.h"
#include "NKDebuggerNub.h"
#include "Macros.h"
#include "NKThreads.h"

USBDevice::USBDevice( USBDeviceDescription *description, ConstASCII8Str name, UInt32 stackSize )
	: Thread( stackSize, kPriorityDriver, name, true ), description( description ),
	_connected( true ), _curConfig( 0 )
{
}

USBDevice::~USBDevice()
{
	delete description;			// This will delete all the pipes and descriptors
}

void
USBDevice::threadLoop()
{
	initialize();
	
	deviceLoop();
	
	while( connected() )
		suspend();				// So drivers which don't override deviceLoop() don't immediately get cut off
}

void
USBDevice::initialize()
{
}

void
USBDevice::deviceLoop()
{
}

void
USBDevice::deviceRemoved()
{
	_connected = false;
}

USBErr
USBDevice::setConfig( UInt8 configNum )
{
	if( configNum >= description->desc->bNumConfigurations )
		return kUSBErr_BadArgs;
	
	// Send command
	USBControlSetup		cmd;
	USBErr				err;
	FatalAssert( description->configs[configNum].desc != nil );
	USBCommandFactory::setConfiguration( cmd, description->configs[configNum].desc->bConfigurationValue );
	err = sendCommand( description->defaultPipe(), &cmd );
	if( !err )
		_curConfig = configNum;
	return err;
}

USBConfiguration*
USBDevice::getConfig( UInt8 configNum )
{
	if( configNum >= description->desc->bNumConfigurations )
		return nil;
	
	return &description->configs[configNum];
}

UInt8
USBDevice::getInterface( UInt8 interfaceNum )
{
	if( interfaceNum >= description->configs[_curConfig].desc->bNumInterfaces )
		return 0;
	return description->configs[_curConfig].interfaces[interfaceNum].curSetting;
}

USBErr
USBDevice::setInterface( UInt8 interfaceNum, UInt8 settingNum )
{
	USBErr				err;
	
//	usbOut << greenMsg << "Checking args...";
	if( interfaceNum >= description->configs[_curConfig].desc->bNumInterfaces )
		return kUSBErr_BadArgs;
	if( settingNum >= description->configs[_curConfig].interfaces[interfaceNum].numSettings )
		return kUSBErr_BadArgs;
	
	USBControlSetup		cmd;
//	usbOut << "Creating command...";
	USBCommandFactory::setInterface( cmd,
		description->configs[_curConfig].interfaces[interfaceNum].settings[settingNum].desc->bInterfaceNumber,
		description->configs[_curConfig].interfaces[interfaceNum].settings[settingNum].desc->bAlternateSetting );
//	usbOut << "Sending...";
	err = sendCommand( description->defaultPipe(), &cmd );
//	usbOut << "Sent!\n" << whiteMsg;
	if( !err )
		description->configs[_curConfig].interfaces[interfaceNum].curSetting = settingNum;
	
	return err;
}

USBErr
USBDevice::sendClassCommand( USBControlSetupPtr cmd, void* buffer )
{
	if( cmd == nil )
		return kUSBErr_BadArgs;
	
	// Make sure it's a class command
	cmd->bmRequestType = (cmd->bmRequestType & ~kBmTypeMask) | kBmTypeClass;
	return sendCommand( description->defaultPipe(), cmd, buffer );
}

USBErr
USBDevice::controlTransfer( USBPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer, Boolean shortTransferOK )
{
	if( pipe == nil )
		return kUSBErr_BadArgs;
	if( pipe->pipe == nil )
		return kUSBErr_Internal;
	
	return sendCommand( pipe->pipe, setup, buffer, shortTransferOK );
}

USBErr
USBDevice::interruptTransfer( USBPipe *pipe, void* buffer, const UInt32 bufferSize, Boolean shortTransferOK )
{
	if( pipe == nil )
		return kUSBErr_BadArgs;
	if( pipe->pipe == nil )
		return kUSBErr_Internal;
	
	USBErr				err;
	USBTransaction			*trans;
	err = description->bus()->driver->interruptTransfer( trans, pipe->pipe, (Ptr)buffer, bufferSize, shortTransferOK );
	if( err )
		return err;
	CurrThread::blockForIO( trans );
	if( !trans->hasDoneIO )
	{
		nkVideo << "blockForIO didn't block?  MSR=" << GetMSR() << "\n";
		Panic( "Thread::blockForIO didn't block!!!" );
	}
	err = (USBErr)trans->ioError();
	delete trans;
	
	return err;
}

USBErr
USBDevice::sendCommand( USBHCPipe *pipe, ConstUSBControlSetupPtr setup, void* buffer, Boolean shortTransferOK )
{
	USBTransaction			*trans;
	USBErr				err;
	err = description->bus()->driver->controlTransfer( trans, pipe, setup, (Ptr)buffer, shortTransferOK );
	if( err )
		return err;
	CurrThread::blockForIO( trans );
	err = (USBErr)trans->ioError();
	delete trans;
	
	return err;
}

USBDummyDevice::USBDummyDevice( USBDeviceDescription *description )
	: USBDevice( description, "USB Dummy Device" )
{
}

USBDummyDevice::~USBDummyDevice()
{
}

void
USBDummyDevice::deviceRemoved()
{
	USBDevice::deviceRemoved();
	delete this;
}