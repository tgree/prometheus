/*
	ADBInternals.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	???
	
	Version History
	============
	Patrick Varilly		-	Thur, 22 Jan 98	-	Original creation of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Patrick Varilly		-	Fri, 12 June 98		-	Updated for new ADB classes
	Terry Greeniaus	-	Sat, 13 June 98	-	Added return value for InitADB() so we know if it worked or not
	Patrick Varilly		-	Sat, 20 June 98	-	Removed return value for InitADB() and updated for new Driver model
*/
#ifndef __ADB__INTERNALS__
#define __ADB__INTERNALS__

#include "ADBHardwareDriver.h"

class ADBDevice;

// Device types
enum ADBDeviceType
{
	kADBDeviceProtect = 1,
	kADBDeviceKeyboard,
	kADBDeviceMouse,
	kADBDeviceTablet,
	kADBDeviceModem,		// ADB Modems???? What else will they come up with?!?!?
	kADBDeviceReserved,
	kADBDeviceAppl		// Application-specific device
	
	// Note: There can be other types of ADB devices, but these are not automatically recognized by Pandora
};

typedef enum ADBDeviceType ADBDeviceType;

typedef enum ADBResultCode ADBResultCode;

class ADBDevice
{
protected:
					ADBDevice();
	virtual			~ADBDevice();
	
	UInt8			registerByType( ADBDeviceType deviceType );
	UInt8			registerByNumber( UInt8 deviceNumber );
	ADBResultCode		setHandlerID( UInt8 deviceNum, UInt8 newHandlerID );
	ADBResultCode		getHandlerID( UInt8 deviceNum, UInt8 *handlerID );
	ADBResultCode		readReg( UInt8 deviceNum, UInt8 regNum, UInt16* value );
	ADBIOCommand*	readRegAsync( UInt8 deviceNum, UInt8 regNum );
	ADBResultCode		writeReg( UInt8 deviceNum, UInt8 regNum, UInt16 value );
	ADBIOCommand*	writeRegAsync( UInt8 deviceNum, UInt8 regNum, UInt16 value );

public:
	virtual void		handleDevice( UInt8 deviceNum, UInt8* data, UInt32 dataSize ) = 0;
};

typedef struct
{
	UInt8			address, type, flags;
	ADBDevice*		owner;
} ADBDeviceInfo;

const Int8						kADBDeviceCount = 16;

class ADBHardware	:	public Driver
{
protected:
	friend void				InitADB( void );
	friend class				ADBDevice;
	
	ADBHardwareDriver			*driver;
	ADBDeviceInfo				devices[kADBDeviceCount];
	UInt8					deviceCount, noInterruptMode;
	
	virtual void				initialize();
	virtual void				start();
	virtual void				stop();
	
	void						resetADBBus();
	Boolean					isDevicePresent( UInt8 address );
	Boolean					findUnresolvedDevice( UInt8* deviceNum );
	UInt8					findFreeDevice( void );
	void						moveDevice( UInt8 from, UInt8 to );
	
public:
							ADBHardware();
							
	ADBResultCode				readReg( UInt8 deviceNum, UInt8 regNum, UInt16* value );
	ADBIOCommand*			readRegAsync( UInt8 deviceNum, UInt8 regNum );
	ADBResultCode				writeReg( UInt8 deviceNum, UInt8 regNum, UInt16 value );
	ADBIOCommand*			writeRegAsync( UInt8 deviceNum, UInt8 regNum, UInt16 value );
	ADBResultCode				setHandlerID( UInt8 deviceNum, UInt8 newHandlerID );
	ADBResultCode				getHandlerID( UInt8 deviceNum, UInt8* handlerID );
	UInt8					registerOwnerByType( ADBDevice *owner, ADBDeviceType type );
	UInt8					registerOwnerByAddr( ADBDevice *owner, UInt8 address );
	void						restart(void);
	void						shutDown(void);
	void						printBusInfo(void);
	
	void						checkInterrupt();
	void						toggleInterruptMode( Boolean noInterruptMode );
	Boolean					getInterruptMode();
							
	void						DeviceMessage( UInt8 deviceNum, UInt32 msgLen, UInt8* msg );
};

#endif /* !__ADB__INTERNALS__ */