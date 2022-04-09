/*
	Block Device.cp
	Copyright © 1998 by Patrick Varilly

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
	none
	
	Version History
	============
	Terry Greeniaus	-	Friday, 19 June 98	-	Added GNU licence to file
	Terry Greeniaus	-	Friday, 19 June 98	-	Added driver1 = driver2 = 0 line to ADBIOCommand() since
											new doesn't do this for us anymore.
*/
#include "Kernel Types.h"
#include "ADBHardwareDriver.h"
#include "Memory Utils.h"

ADBIOCommand::ADBIOCommand()
{
	command = kADBNoCmd;
	isADBCommand = false;
	type = kArgCommand;
	data.info.numArgs = 0;
	result = kADBResultNotDone;
	reply = nil;
	replyLen = 0;
	driver1 = driver2 = 0;
}

ADBIOCommand::ADBIOCommand( UInt16 command, UInt8 numArgs, UInt8 arg1, UInt8 arg2 )
{
	this->command = command;
	isADBCommand = true;
	type = kArgCommand;
	data.info.numArgs = numArgs;
	data.info.arg[0] = arg1;
	data.info.arg[1] = arg2;
	result = kADBResultNotDone;
	reply = nil;
	replyLen = 0;
}

ADBIOCommand::ADBIOCommand( UInt16 command, UInt32 len, UInt8* buffer )
{
	this->command = command;
	isADBCommand = true;
	type = kBufferCommand;
	data.buffer.len = len;
	data.buffer.buffer = new(kernelProcess) UInt8[len];
	MemCopy( buffer, data.buffer.buffer, len );
	result = kADBResultNotDone;
	reply = nil;
	replyLen = 0;
}

ADBIOCommand::~ADBIOCommand()
{
	if( type == kBufferCommand )
		delete [] data.buffer.buffer;
	delete [] reply;
}

Boolean
ADBIOCommand::GetResult( UInt8& result )
{
	result = this->result;
	return (result != kADBResultNotDone);
}

Boolean
ADBIOCommand::GetReply( UInt8* reply, UInt32& replyLen )
{
	reply = this->reply;
	replyLen = this->replyLen;
	return (reply != nil);
}

UInt32
ADBIOCommand::ioError()
{
	return result;
}

void
ADBIOCommand::MakeStartStopAutoPoll( Boolean startAutoPoll )
{
	// All other fields set by parameter-less constructor
	command = kADBPseudoCmdStartStopAutoPoll;
	data.info.numArgs = 1;
	data.info.arg[0] = startAutoPoll;
}

void
ADBIOCommand::MakeSetDeviceList( UInt16 deviceList )
{
	// All other fields set by parameter-less constructor
	command = kADBPseudoCmdSetDeviceList;
	data.info.numArgs = 2;
	data.info.arg[0] = (deviceList >> 8) & 0xFF;
	data.info.arg[1] = deviceList & 0xFF;
}

void
ADBIOCommand::MakeSetAutoPollRate( UInt8 rate )
{
	// All other fields set by parameter-less constructor
	command = kADBPseudoCmdSetAutoPollRate;
	data.info.numArgs = 1;
	data.info.arg[0] = rate;
}

void
ADBIOCommand::MakePowerDown()
{
	// All other fields set by parameter-less constructor
	command = kADBPseudoCmdPowerDown;
}

void
ADBIOCommand::MakeRestart()
{
	// All other fields set by parameter-less constructor
	command = kADBPseudoCmdRestart;
}

ADBHardwareDriver::ADBHardwareDriver( ConstASCII8Str driverName, ADBHardware *adb )
	: IOCommandDriver( driverName )
{
	this->adb = adb;
}

ADBIOCommand*
ADBHardwareDriver::SetAutoPollState( Boolean on )
{
	ADBIOCommand*	command = new ADBIOCommand;
	command->MakeStartStopAutoPoll( on );
	if( sendCommand( command ) )
		return command;
	else
	{
		delete command;
		return nil;
	}
}

ADBIOCommand*
ADBHardwareDriver::SetAutoPollRate( UInt8 rate )
{
	ADBIOCommand*	command = new ADBIOCommand;
	command->MakeSetAutoPollRate( rate );
	if( sendCommand( command ) )
		return command;
	else
	{
		delete command;
		return nil;
	}
}

ADBIOCommand*
ADBHardwareDriver::SetDeviceList( UInt16 deviceList )
{
	ADBIOCommand*	command = new ADBIOCommand;
	command->MakeSetDeviceList( deviceList );
	if( sendCommand( command ) )
		return command;
	else
	{
		delete command;
		return nil;
	}
}

ADBIOCommand*
ADBHardwareDriver::PowerDown()
{
	ADBIOCommand*	command = new ADBIOCommand;
	command->MakePowerDown();
	if( sendCommand( command ) )
		return command;
	else
	{
		delete command;
		return nil;
	}
}

ADBIOCommand*
ADBHardwareDriver::Restart()
{
	ADBIOCommand*	command = new ADBIOCommand;
	command->MakeRestart();
	if( sendCommand( command ) )
		return command;
	else
	{
		delete command;
		return nil;
	}
}