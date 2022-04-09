/*
	USB.h
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
	Patrick Varilly		-	Monday, 29 March 99	-	Creation of file
	Patrick Varilly		-	Friday, 26 Nov 99		-	Real start of USB driver, before it was just unsuccessful stabs at it
*/

#ifndef __USB__
#define __USB__

#include "Driver.h"
#include "Streams.h"
#include "NKThreads.h"

// Various standard types
typedef Int8 USBDevID;
typedef const USBDevID ConstUSBDevID;

#define kEndpointOut			0x00			// USB 1.1 uses endpoint addresses which have the number as the
#define kEndpointIn			0x80			// 4 least significant bits and the direction in the msb (all in one byte)
#define kEndpointMask		0x0F
typedef UInt8 USBEndpointAddress;
typedef const USBEndpointAddress ConstUSBEndpointAddress;

// Direction of transactions
typedef enum USBDirection
{
	kUSBDirection_In,
	kUSBDirection_Out
} USBDirection;

// USB Errors
typedef enum USBErr
{
	kUSBErr_None = 0,
	
	// Driver errors
	kUSBErr_Unsupported,
	kUSBErr_OutOfResources,
	kUSBErr_InUse,
	kUSBErr_Internal,
	kUSBErr_BadArgs,
	kUSBErr_Aborted,
	kUSBErr_DataOverrun,
	kUSBErr_DataUnderrun,
	kUSBErr_BufferOverrun,
	kUSBErr_BufferUnderrun,
	kUSBErr_Deleted,
	
	// USB errors
	kUSBErr_CRC,
	kUSBErr_BitStuffing,
	kUSBErr_DataToggleMismatch,
	kUSBErr_Stall,
	kUSBErr_RequestError = kUSBErr_Stall,
	kUSBErr_DeviceNotResponding,
	kUSBErr_PIDCheckFailure,
	kUSBErr_UnexpectedPID,
	kUSBErr_DeviceRemoved
} USBErr;
extern ASCII8Str gUSBErrorStrings[];

// *** USB Protocol Structures *** (Note: you *do not* need to reverse the byte order on these)
// Endpoints are specified in wIndex, with bit 7 (counting from lsb to msb, starting from 0) set to one
// if it's an in endpoint, or zero if it's an out endpoint.  Control endpoints are specified with a zero (out),
// though technically the device should ignore the direction bit when a control endpoint number is specified.
#define IN_EN_TO_INDEX(x)	LE16(((x)&0x0F) | kEndpointOut)
#define OUT_EN_TO_INDEX(x)	LE16((x)&0x0F)

// USB Control Pipe Request
typedef struct USBControlSetup
{
	UInt8				bmRequestType;
	UInt8				bRequest;
	UInt16				wValue;
	UInt16				wIndex;
	UInt16				wLength;
} USBControlSetup, *USBControlSetupPtr;
typedef const USBControlSetupPtr ConstUSBControlSetupPtr;

// Bits in bmRequestType
//	7	6	5	4	3	2	1	0
//	Dir	|    Type	|	   Recipient	  |
#define kBmDownstream		0x00
#define kBmUpstream		0x80
#define kBmDirMask			0x80
#define kBmTypeStandard		0x00
#define kBmTypeClass		0x20
#define kBmTypeVendor		0x40
#define kBmTypeReserved	0x60
#define kBmTypeMask		0x60
#define kBmRecipientDevice	0x00
#define kBmRecipientInterface	0x01
#define kBmRecipientEndpoint	0x02
#define kBmRecipientOther	0x03
#define kBmRecipientMask	0x18

// Standard request selectors
enum
{
	kUSBGetStatus = 0,
	kUSBClearFeature = 1,
	kUSBSetFeature = 3,
	kUSBSetAddress = 5,
	kUSBGetDescriptor = 6,
	kUSBSetDescriptor = 7,
	kUSBGetConfiguration = 8,
	kUSBSetConfiguration = 9,
	kUSBGetInterface = 10,
	kUSBSetInterface = 11,
	kUSBSynchFrame = 12
};

// Standard descriptor types
enum
{
	kDescType_Device = 1,
	kDescType_Configuration = 2,
	kDescType_String = 3,
	kDescType_Interface = 4,
	kDescType_Endpoint = 5
};

// Standard feature selectors
enum
{
	// Device selectors
	kDeviceRemoteWakeup = 1,
	
	// Endpoint selectors
	kEndpointHalt = 0
};

// Standard descriptor header
typedef struct USBDescHeader
{
	UInt8				bLength;
	UInt8				bDescriptorType;
} USBDescHeader;

// Device descriptor structure
typedef struct USBDeviceDesc
{
	USBDescHeader			header;
	UInt16				bcdUSB;
	UInt8				bDeviceClass;
	UInt8				bDeviceSubClass;
	UInt8				bDeviceProtocol;
	UInt8				bMaxPacketSize0;
	UInt16				idVendor;
	UInt16				idProduct;
	UInt16				bcdDevice;
	UInt8				iManufacturer;
	UInt8				iProduct;
	UInt8				iSerialNumber;
	UInt8				bNumConfigurations;
} USBDeviceDesc;

// Configuration descriptor structure
typedef struct USBConfigDesc
{
	USBDescHeader			header;
	UInt16				wTotalLength;
	UInt8				bNumInterfaces;
	UInt8				bConfigurationValue;
	UInt8				iConfiguration;
	UInt8				bmAttributes;
	UInt8				MaxPower;
} USBConfigDesc;

// Interface descriptor structure
typedef struct USBInterfaceDesc
{
	USBDescHeader			header;
	UInt8				bInterfaceNumber;
	UInt8				bAlternateSetting;
	UInt8				bNumEndpoints;
	UInt8				bInterfaceClass;
	UInt8				bInterfaceSubClass;
	UInt8				bInterfaceProtocol;
	UInt8				iInterface;
} USBInterfaceDesc;

// Endpoint descriptor structure
typedef struct USBEndpointDesc
{
	USBDescHeader			header;
	UInt8				bEndpointAddress;
	UInt8				bmAttributes;
	UInt16				wMaxPacketSize;
	UInt8				bInterval;
} USBEndpointDesc;

// Fields in bmAttributes
#define kBmEPTypeMask		0x03
#define kBmEPControl		0x00
#define kBmEPIsochronous	0x01
#define kBmEPBulk			0x02
#define kBmEPInterrupt		0x03

// Device class constants
enum
{
	kClassNone = 0,
	kSubClassNone = 0,
	kProtocolNone = 0,
	
	kClassHID = 3,
	kSubClassHIDBoot = 1,
	kProtocolHIDKeyboard = 1,
	kProtocolHIDMouse = 2,
	
	kClassHub = 9
};

// Configuration attributes
enum
{
	kConfigLegacy = 0x80,
	kConfigSelfPowered = 0x40,
	kConfigRemoteWakeup = 0x20
};

class USBDevice;
class USBInterface;
class USBHCDriver;

class USBHCPipe
{
};

class USBTransaction : public IOCommand
{
public:
						USBTransaction();
	virtual				~USBTransaction();
	
	virtual USBErr			abort() = 0;
		// asks for abort, keep blocking for I/O
};

/*class USBDeviceDriver
{
public:
						USBDriver();
	virtual				~USBDriver();
	
	virtual USBDevice*		probeDevice();
	
protected:
	friend class USBConfigurator;
	UInt16				idVendor, idProduct, bcdDevice;
	UInt8				bDeviceClass, bDeviceSubClass, bDeviceProtocol;
	
private:
	static USBDriver		*driverQueue;
	USBDriver			*nextDriver, *prevDriver;
};*/

class USBBus
{
public:
						USBBus( USBHCDriver* inDriver );
	
	UInt32				bus();
	USBBus*				next();
	USBDevice*			getDevice( ConstUSBDevID devID );
	
protected:
	friend class USBHub;
	friend class USBDevice;
	friend class USBConfigurator;
	friend class USBDeviceDescription;
	friend void InitUSBBusses( void );
	friend void USBStat( void );
	
	USBDevID				deviceAdded( Boolean fullSpeed );
	void					deviceRemoved( ConstUSBDevID devID );
	void					deviceProbed( ConstUSBDevID devID, USBDevice* dev );
		
	static UInt32			_nextBusID;
	UInt32				_busID;
	USBBus				*_next;
	USBHCDriver			*driver;
	USBHub				*rootHub;
	USBDevice			*device[128];	// 0 is always nil, for easy mapping
	MutexLock			topologyLock, probeLock;
	USBHCPipe			*defaultPipe, *slowDefaultPipe;	// Control pipe at address 0, endpoint 0, MPS of 8 bytes
};

extern OStreamWrapper usbOut;

void InitUSB( void );
void InitUSBBusses( void );
void USBStat( void );

#endif /* __USB__ */