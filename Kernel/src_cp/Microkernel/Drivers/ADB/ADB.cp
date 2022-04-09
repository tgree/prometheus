/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * Copyright 1996 1995 by Apple Computer, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MKLINUX-1.0DR2
 */

/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	adb.c					MkLinux DR2.1 update 6	???				Code is adapted from here.
	cuda_power.c			MkLinux DR2.1 update 6	???				And here.
	
	Version History
	============
	Patrick Varilly		-	Thur, 22 Jan 98		-	Original creation of file
	Patrick Varilly		-	Monday, 26 Jan 98		-	Phew!!! Long file. Finished adaptation of ADB driver.
	Patrick Varilly		-	Saturday, 21 Mar 98	-	Made minor adjustments.
													Added support for non-interrupt ADB (for debugger)
	Patrick Varilly		-	Thursday, 28 May 98	-	Added Mach copyright notices (oops!)
	Patrick Varilly		-	Friday, 12 Jun 98		-	Re-wrote file to support ADBHardwareDrivers
	Terry Greeniaus	-	Sat, 13 June 98		-	Added return value for InitADB() so we know if it worked or not
	Patrick Varilly		-	Saturday, 20 June 98	-	Made ADB fit into the new driver init/start model.
													Removed return value from InitADB, since we now just
													don't add our driver to the driver list if things didn't
													work out.
	Terry Greeniaus	-	Sunday, 21 June 98		-	Added noInterruptMode = false to ADBHardware
	Terry Greeniaus	-	Sunday, 21 June 98		-	ShutDownHandler::shutDown() now takes a variable stating if it is a ShutDown()
												or Restart().
	Patrick Varilly		-	Monday, 29 March 99	-	De-commented driver->start() in ADBHardware::start()
												(see comments for why).  Also added ADBPresent for USB-only
												machines support
	Terry Greeniaus	-	Wednesday, 27 Oct 99	-	Changed ADBPresent to see if a CUDA or PMU device has a physical address (previously
												it checked if this was not a NewWorld machine, however B&W supports ADB).
*/

#include "NKVideo.h"
#include "ADB.h"
#include "ADBInternals.h"
#include "Streams.h"
#include "Shutdown.h"
#include "Time.h"
#include "CUDADriver.h"
#include "PMUDriver.h"
#include "NKThreads.h"
#include "NKMachineInit.h"

static ADBHardware		*adb = nil;

//#define CAN_TRACE_ADB	1

#ifdef CAN_TRACE_ADB
	static Boolean				tracingADB = true;

	#define START_ADB_TRACE	tracingADB = true
	#define STOP_ADB_TRACE	tracingADB = false
	#define ADB_MSG(x,y)		if( 0 ) do { cout << x; Wait_ms(200); } while(0)
	#define ADB_SPR(x,y)			if( tracingADB ) do { cout << x; Wait_ms(200); } while(0)
#else
	#define START_ADB_TRACE
	#define STOP_ADB_TRACE
							// 0 -17 work
							// 0 - 18 crash
	#define ADB_MSG(x,y)		//cout << x;
	#define ADB_SPR(x,y)	//	cout << x;
#endif

// Device flags
enum
{
	kADBDeviceFlagPresent = 0x01,
	kADBDeviceFlagUnresolved = 0x02,
	kADBDeviceFlagRegistered = 0x04
};

void InitADB( void )
{
	ADB_SPR( "Creating ADB Hardware Driver\n",1 );
	adb = new ADBHardware;
	
	switch(machine.machineClass)
	{
		case classPowerBookPCI:
			ADB_SPR("Creating PMU Hardware Driver\n",2);
			adb->driver = new PMUDriver(adb,&machine.pmuDevice);
			
			machine.driverList.enqueue(adb->driver);
		break;
		case classPCI:
		case classPDM:
		case classPerforma:
		case classG3:
		case classNewWorld:
			ADB_SPR( "Creating CUDA Hardware Driver\n",2 );
			adb->driver = new CUDADriver(adb,&machine.cudaDevice);
			
			machine.driverList.enqueue(adb->driver);
		break;
		default:
			cout << "Don't know enough about this machine's ADB chip to start up ADB!\n";
			return;
		break;
	}
	
	if( ADBPresent() )
		machine.driverList.enqueue(adb);
}

Boolean ADBPresent( void )
{
	if(machine.cudaDevice.physicalAddr != nil)
		return true;
	if(machine.pmuDevice.physicalAddr != nil)
		return true;
	return false;
	//return (machine.kernelMachineType != machineNewWorld);
}

ADBHardware::ADBHardware()
	: Driver( "ADB Driver" )
{
	noInterruptMode = false;
}

void
ADBHardware::initialize()
{
}

void
ADBHardware::start()
{
	ADBIOCommand			*command;
	
	ADB_SPR( "Initializing ADB...\n",3 );
	
	// Start the hardware driver -- Pat: You're starting the CUDADriver twice!!!
	// ADB drivers can be safely "started" more than once. This is to support stop, then start
	driver->start();
	
	// Stop auto-polling while ADB reset is complete
	ADB_SPR( "Disabling autopoll\n",4 );
	command = driver->SetAutoPollState( false );
	ADB_MSG( "Waiting\n",5 );
	CurrThread::blockForIO(command);
	delete command;
	ADB_MSG( "Done\n",6 );
	
	// Reset the bus. By the time this returns, the reset is complete, so we wait no more
	resetADBBus();
	
	// Attempt to reassign the bus
	
	// Skip 0 -- its special!
	devices[0].address = devices[0].type = devices[0].flags = 0;
	devices[0].owner = nil;
	
	for ( Int32 i = 1; i < kADBDeviceCount; i++ )
	{
		devices[i].address = i;
		devices[i].type = 0;
		devices[i].flags = 0;
		devices[i].owner = nil;
		
		if ( isDevicePresent(i) )
		{
			devices[i].type = i;
			devices[i].flags |= kADBDeviceFlagUnresolved;
		}
	}
	
	// Now attempt to reassign the addresses
	UInt8			deviceNum, freeNum;
	
	while ( findUnresolvedDevice( &deviceNum ) )
	{
		freeNum = findFreeDevice();
		
		moveDevice(deviceNum, freeNum);

		if ( !isDevicePresent(freeNum) )
		{
			// It didn't move.. damn!
			devices[deviceNum].flags &= ~kADBDeviceFlagUnresolved;
			cout << "Warning: ADB device " << deviceNum << " is having problems probing.\n";
			continue;
		}

		if ( !isDevicePresent(deviceNum) )
		{
			// no more at this address, good !
			// Move it back...

			moveDevice(freeNum, deviceNum);

			// Check the device to talk again, and check freeNum is free
			(void) isDevicePresent(deviceNum);
			if(!isDevicePresent(freeNum))
				devices[freeNum].flags &= ~kADBDeviceFlagPresent;
			devices[deviceNum].flags &= ~kADBDeviceFlagUnresolved;
		}
		else
		{
			// Found another device at the address. Leave the first device moved to one side and set up
			// the newly found device for probing
			devices[freeNum].flags &= ~kADBDeviceFlagUnresolved;
			devices[deviceNum].type = deviceNum;
			devices[deviceNum].flags = kADBDeviceFlagUnresolved;
			cout << "Found hidden ADB device at address " << deviceNum << "\n";
		}
	}
	
	STOP_ADB_TRACE;
	
	// Build a bitmap list of all devices
	UInt16		devList = 0;
	deviceCount = 0;
	
	// Skip 0
	for ( Int32 i = 1; i < kADBDeviceCount; i++ )
	{
		if ( (devices[i].flags & kADBDeviceFlagPresent) == 0 )
			continue;

		devList |= (1<<i);
		deviceCount++;
	}
	
	// Send the deviceList to the hardware
	ADB_SPR( "Setting device list\n",7 );
	command = driver->SetDeviceList( devList );
	ADB_MSG( "Waiting\n",8 );
	CurrThread::blockForIO(command);
	ADB_MSG( "Done\n",9 );
	delete command;
	
	// Set the auto poll rate
	ADB_SPR( "Setting autopoll rate\n",10 );
	command = driver->SetAutoPollRate( 11 );
	ADB_MSG( "Waiting\n",11 );
	CurrThread::blockForIO(command);
	ADB_MSG( "Done\n",12 );
	delete command;
	
	// Start auto polling
	ADB_SPR( "Enabling autopoll\n",13 );
	command = driver->SetAutoPollState( true );
	ADB_MSG( "Waiting\n",14 );
	CurrThread::blockForIO(command);
	ADB_MSG( "Done\n",15 );
	delete command;
	
	ADB_SPR( "ADB Initialized!\n",16 );
}

void
ADBHardware::stop()
{
	ADBIOCommand		*command;
	
	// Disable auto-polling so no device messages come in.
	ADB_SPR( "Disabling autopoll\n",17 );
	command = driver->SetAutoPollState( false );
	ADB_MSG( "Waiting\n",18 );
	CurrThread::blockForIO(command);
	delete command;
	ADB_MSG( "Done\n",19 );
	
	// Now stop the hardware driver
	driver->stop();
}

void
ADBHardware::resetADBBus()
{
	ADBIOCommand*		theCommand = new ADBIOCommand( kADBCmdResetBus | (16 << 4) );
	
	ADB_SPR( "Resetting bus\n",20 );
	driver->sendCommand( theCommand );
	ADB_MSG( "Waiting\n",21 );
	CurrThread::blockForIO( theCommand );
	ADB_MSG( "Done\n",22 );
	
	delete theCommand;
}

Boolean
ADBHardware::isDevicePresent( UInt8 address )
{
	ADBDeviceInfo		*device = &devices[address];
	UInt16			value;
	Int32			result;
	
	ADB_SPR( "Is Device " << address << " present? ",23 );
	result = readReg(address, 3, &value);

	if (result != kADBResultTimeout)
	{
		device->address = (value >> 8) & 0xF;
		device->flags |= kADBDeviceFlagPresent;
		ADB_SPR( "Yes\n",24 );
		return true;
	}
	
	ADB_SPR( "No\n",25 );
	return false;
}

// Find an unresolved device
Boolean
ADBHardware::findUnresolvedDevice( UInt8* deviceNum )
{
	// Skip 0!!!
	for( Int32 i=1; i < kADBDeviceCount; i++ )
	{
		if( (devices[i].flags & kADBDeviceFlagUnresolved) != 0 )
		{
			*deviceNum = i;
			return true;
		}
	}
	
	// YAY!!! No more unresolved devices!!!
	return false;
}

// Find a free device
UInt8
ADBHardware::findFreeDevice( void )
{
	// Skip 0!!!
	// Try and find a free slot in reverse order since most present devices will be in the lower slots
	for( Int32 i=kADBDeviceCount-1; i > 0; i-- )
	{
		if( (devices[i].flags & kADBDeviceFlagPresent) == 0 )
			return i;
	}
	
	// Damn!
	Panic("ADB bus has more than 16 devices???\n");
	
	// Just so that CW doesn't get upset
	return 0;
}

// Move a device from "from" to "to"
void
ADBHardware::moveDevice( UInt8 from, UInt8 to )
{
	UInt8		address;
	
	ADB_SPR( "Moving device " << from << " to " << to << "\n" ,26);
	writeReg(from, 3, ((to << 8) | 0xfe));

	address = devices[to].address;
	devices[to] = devices[from];
	devices[to].address = address;
	devices[from].flags = 0;
}

ADBResultCode
ADBHardware::readReg( UInt8 deviceNum, UInt8 regNum, UInt16* value )
{
	ADBIOCommand		*theCommand;
	ADBResultCode		result;
	
	theCommand = readRegAsync( deviceNum, regNum );
	ADB_SPR( "Waiting\n",27 );
	CurrThread::blockForIO(theCommand);
	ADB_SPR( "Done, reply is " << (UInt32)theCommand->reply << "\n",28 );
	
	if( theCommand->reply )
		*value = (theCommand->reply[0] << 8) | theCommand->reply[1];
	else
		*value = 0;
	result = theCommand->result;
	delete theCommand;
	
	return result;
}

ADBIOCommand*
ADBHardware::readRegAsync( UInt8 deviceNum, UInt8 regNum )
{
	ADBIOCommand		*theCommand = new ADBIOCommand( (kADBCmdReadADB | deviceNum << 4 | regNum) );
	ADB_SPR( "Sending read reg command\n",29 );
	driver->sendCommand(theCommand);
	return theCommand;
}

ADBResultCode
ADBHardware::writeReg( UInt8 deviceNum, UInt8 regNum, UInt16 value )
{
	ADBIOCommand		*theCommand;
	ADBResultCode		result;
	
	theCommand = writeRegAsync( deviceNum, regNum, value );
	ADB_MSG( "Waiting\n",30 );
	CurrThread::blockForIO(theCommand);
	ADB_MSG( "Done\n",31 );
	
	result = theCommand->result;
	delete theCommand;
	
	return result;
}

ADBIOCommand*
ADBHardware::writeRegAsync( UInt8 deviceNum, UInt8 regNum, UInt16 value )
{
	ADBIOCommand		*theCommand = new ADBIOCommand( (kADBCmdWriteADB | deviceNum << 4 | regNum), 2, value >> 8, value & 0xFF );
	ADB_SPR( "Sending write reg command\n",32 );
	driver->sendCommand(theCommand);
	return theCommand;
}

ADBResultCode
ADBHardware::setHandlerID( UInt8 deviceNum, UInt8 newHandlerID )
{
	ADBResultCode		result;
	UInt16			value;

	result = readReg(devices[deviceNum].address, 3, &value);
	if (result != kADBResultOK) 
		return result;

	value = (value & 0xF000) | newHandlerID;
	result = writeReg(devices[deviceNum].address, 3, value);
	if (result != kADBResultOK) 
		return result;
	
	return kADBResultOK;
}

ADBResultCode
ADBHardware::getHandlerID( UInt8 deviceNum, UInt8* handlerID )
{
	ADBResultCode		result;
	UInt16			value;

	result = readReg(devices[deviceNum].address, 3, &value);
	if (result != kADBResultOK) 
		return result;

	*handlerID = (value & 0xff);
	
	return kADBResultOK;
}

// Make a device receive all messages from devices of one type
UInt8
ADBHardware::registerOwnerByType( ADBDevice *owner, ADBDeviceType type )
{
	UInt8		deviceID = 0;
	
	for( Int32 i=0; i < kADBDeviceCount; i++ )
	{
		ADBDeviceInfo		*device = &devices[i];
		
		if( device->type != type )
			continue;
		
		if( deviceID == 0 )
			deviceID = i;
		//cout << "Registering device " << i << " to " << (UInt32)owner << "\n";
		device->owner = owner;
		device->flags |= kADBDeviceFlagRegistered;
	}
	
	return deviceID;
}

// Make owner the owner of the device at bus address addr
UInt8
ADBHardware::registerOwnerByAddr( ADBDevice *owner, UInt8 addr )
{
	if( (addr < 1) || (addr >= kADBDeviceCount) )
		Panic("Tried to register an unexistant ADB device\n");
	
	devices[addr].owner = owner;
	//cout << "Registering device " << addr << " to " << (UInt32)owner << "\n";
	devices[addr].flags |= kADBDeviceFlagRegistered;
	return addr;
}

// Called when a device sends a message to us
void
ADBHardware::DeviceMessage( UInt8 deviceNum, UInt32 msgLen, UInt8* msg )
{
	if ( (devices[deviceNum].flags & kADBDeviceFlagRegistered) != 0 )
		devices[deviceNum].owner->handleDevice( deviceNum, msg, msgLen );
	else
		nkVideo << "Device message from " << deviceNum << " ignored\n";
}

// Print the number and type of the devices attached to the ADB
void PrintADBInfo( void )
{
	adb->printBusInfo();
}

ASCII7Str			deviceNames[16] = {
	nil, "security device", "keyboard", "mouse", "tablet", "modem", nil, "application-specific",
	nil, nil, nil, nil, nil, nil, nil, nil };

void
ADBHardware::printBusInfo(void)
{
	cout << "ADB Bus Info: " << deviceCount << " devices on bus\n";
	
	for( Int32 i=1; i < kADBDeviceCount; i++ )
	{
		if( (devices[i].flags & kADBDeviceFlagPresent) != 0 )
		{
			ASCII7Str			name = deviceNames[devices[i].type];
			
			cout << "    " << i << ": ";
			if( name != nil )
				cout << name;
			else
				cout << "unknown device id=" << devices[i].type;
			
			cout << "\n";
		}
	}
}

void
ADBHardware::shutDown(void)
{
	if ( machine.kernelMachineType != machine6100 )
	{		
		ADBIOCommand		*theCommand;
		theCommand = driver->PowerDown();
		CurrThread::blockForIO(theCommand);
		
		/* If we come back, it's an error */
		Panic( "System didn't restart!!! E-mail Patrick Varilly at\n"
				"varilly@cariari.ucr.ac.cr specifying your machine type\n" );
	}
		/* If we come back, just loop around */
	cout << "\n\n\nIt is now safe to switch off your machine.\n";
	for(;;)
		;
}

void
ADBHardware::restart(void)
{
	ADBIOCommand		*theCommand;
	theCommand = driver->Restart();
	CurrThread::blockForIO(theCommand);
	
	Panic("System should have restarted by now!!!\n");
}

// Restart the system
void Restart(void)
{
	ShutDownHandler::handleShutDown(false);
	adb->restart();
}

// Shut down the system
void ShutDown(void)
{
	ShutDownHandler::handleShutDown(true);
	adb->shutDown();
}

void
ADBHardware::checkInterrupt()
{
	if( noInterruptMode )
		driver->checkInterrupt();
}

void
ADBHardware::toggleInterruptMode( Boolean noInterruptMode )
{
	nkVideo  << "ADB going to ";
	if(noInterruptMode)
		nkVideo << "no ";
	nkVideo << "interrupt mode.\n";
	this->noInterruptMode = noInterruptMode;
	driver->toggleInterruptMode( noInterruptMode );
}

Boolean
ADBHardware::getInterruptMode()
{
	return noInterruptMode;
}

/*************************************************
**************** Misc. Stuff *************************
**************************************************/
void ADBCheckInterrupt( void );
void ADBCheckInterrupt( void )
{
	adb->checkInterrupt();
}

void ADBToggleInterrupt( Boolean noInterruptMode );
void ADBToggleInterrupt( Boolean noInterruptMode )
{
	adb->toggleInterruptMode( noInterruptMode );
}

Boolean ADBGetInterruptMode( void );
Boolean ADBGetInterruptMode( void )
{
	return adb->getInterruptMode();
}

#pragma mark Ê
#pragma mark ADB Device
#pragma mark =========
/************************************************************
 **********************  ADB Device ******************************
 *************************************************************/

ADBDevice::ADBDevice()
{
	FatalAssert( adb->driver != nil );
}

ADBDevice::~ADBDevice()
	{}

UInt8 ADBDevice::registerByType( ADBDeviceType deviceType )
	{ return adb->registerOwnerByType( this, deviceType ); }

UInt8 ADBDevice::registerByNumber( UInt8 deviceNum )
	{ return adb->registerOwnerByAddr( this, deviceNum ); }

ADBResultCode ADBDevice::setHandlerID( UInt8 deviceNum, UInt8 newHandlerID )
	{ return adb->setHandlerID( deviceNum, newHandlerID ); }

ADBResultCode ADBDevice::getHandlerID( UInt8 deviceNum, UInt8* handlerID )
	{ return adb->getHandlerID( deviceNum, handlerID ); }

ADBResultCode ADBDevice::readReg( UInt8 deviceNum, UInt8 regNum, UInt16* value )
	{ return adb->readReg( deviceNum, regNum, value ); }

ADBIOCommand* ADBDevice::readRegAsync( UInt8 deviceNum, UInt8 regNum )
	{ return adb->readRegAsync( deviceNum, regNum ); }

ADBResultCode ADBDevice::writeReg( UInt8 deviceNum, UInt8 regNum, UInt16 value )
	{ return adb->writeReg( deviceNum, regNum, value ); }

ADBIOCommand* ADBDevice::writeRegAsync( UInt8 deviceNum, UInt8 regNum, UInt16 value )
	{ return adb->writeRegAsync( deviceNum, regNum, value ); }