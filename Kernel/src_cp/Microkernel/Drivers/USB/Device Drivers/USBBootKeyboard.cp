/*
	USBBootKeyboard.cp
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
	Patrick Varilly		-	Mon, 6 Dec 99			-	Creation of file
*/

#include "USB.h"
#include "USBDriver.h"
#include "USBInterface.h"
#include "USBCommandFactory.h"
#include "NKDebuggerNub.h"
#include "Streams.h"
#include "Macros.h"

#include "ADBKeyboard.h"		// Kludge for now.  Must separate keyboard and ADB driver for it

// The keyboard map
ASCII8						usbKeyMap[165][2] = {
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 'a', 'A' }, { 'b', 'B' }, { 'c', 'C' }, { 'd', 'D' },
	{ 'e', 'E' }, { 'f', 'F' }, { 'g', 'G' }, { 'h', 'H' },
	{ 'i', 'I' }, { 'j', 'J' }, { 'k', 'K' }, { 'l', 'L' },
	{ 'm', 'M' }, { 'n', 'N' }, { 'o', 'O' }, { 'p', 'P' },
	{ 'q', 'Q' }, { 'r', 'R' }, { 's', 'S' }, { 't', 'T' },
	{ 'u', 'U' }, { 'v', 'V' }, { 'w', 'W' }, { 'x', 'X' },
	{ 'y', 'Y' }, { 'z', 'Z' }, { '1', '!' }, { '2', '@' },
	{ '3', '#' }, { '4', '$' }, { '5', '%' }, { '6', '^' },
	{ '7', '&' }, { '8', '*' }, { '9', '(' }, { '0', ')' },
	{ KEYBOARD_RETURN, KEYBOARD_RETURN }, { 0x1B, 0x1B }, { 0x08, 0x08 }, { 0x09, 0x09 },
	{ ' ', ' ' }, { '-', '_' }, { '=', '+' }, { '[', '{' },
	{ ']', '}' }, { '\\', '|' }, { '`', '~' }, { ';', ':' },
	{ '\'', '"' }, { 0, 0 }, { ',', '<' }, { '.', '>' },
	{ '/', '?' }, { 0, 0 }, { /*F1*/ 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { /*F12*/0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0x7F, 0x7F }, { 0, 0 }, { 0, 0 }, { 0x1D, 0x1D },
	{ 0x1C, 0x1C }, { 0x1F, 0x1F }, { 0x1E, 0x1E }, { 0, 0 },
	{ '/', '/' }, { '*', '*' }, { '-', '-' }, { '+', '+' },
	{ KEYBOARD_RETURN, KEYBOARD_RETURN }, { '1', '1' },
	{ '2', '2' }, { '3', '3' }, { '4', '4' }, { '5', '5' },
	{ '6', '6' }, { '7', '7' }, { '8', '8' }, { '9', '9' },
	{ '0', '0' }, { '.', '.' }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { '=', '=' }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { ',', ',' },
	{ '=', '=' }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ KEYBOARD_RETURN, KEYBOARD_RETURN }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 } };

// The maximum size for the keyboard buffer. 128 chars should be plenty
const UInt32	kMaxKeyboardBufferSize	= 128;

class USBBootKeyboardInterface : public USBInterface, public IStream
{
public:
							USBBootKeyboardInterface( USBInterfaceDescription* interface,
								USBInterfacedDevice* owner, UInt8 interfaceNum, UInt8 setting );
	virtual					~USBBootKeyboardInterface();
	
	virtual void				threadLoop();
	
	virtual void				read(Int8* data,UInt32 len);
	
private:
	ASCII8					readChar();
	
	UInt8					modifiers;
	UInt8					keys[6];
	ASCII8					buffer[kMaxKeyboardBufferSize];
	volatile ASCII8				*bufferPos, *bufferEnd;
};

class USBBootKeyboardDriver : public USBInterfaceDriver
{
public:
							USBBootKeyboardDriver();
	virtual					~USBBootKeyboardDriver();
	
	virtual USBInterface*		buildInterface( USBInterfaceDescription *description,
								USBInterfacedDevice* device, UInt16 idVendor, UInt16 idProduct,
								UInt16 bcdDevice, UInt8 bConfigurationValue, UInt8 bInterfaceNumber,
								UInt8 setting );
};

static USBBootKeyboardDriver		kbdDriver;

USBBootKeyboardDriver::USBBootKeyboardDriver()
{
	registerClassDriver( kClassHID, kSubClassHIDBoot, kProtocolHIDKeyboard );
}

USBBootKeyboardDriver::~USBBootKeyboardDriver()
{
}

USBInterface*
USBBootKeyboardDriver::buildInterface( USBInterfaceDescription *description, USBInterfacedDevice* device,
	UInt16 /*idVendor*/, UInt16 /*idProduct*/, UInt16 /*bcdDevice*/, UInt8 /*bConfigurationValue*/, UInt8 bInterfaceNumber,	// I commented these to get rid of "unused argument" warnings
	UInt8 setting )
{
	return new USBBootKeyboardInterface( description, device, bInterfaceNumber, setting );
}

// *** Boot keyboard driver start here ***

USBBootKeyboardInterface::USBBootKeyboardInterface( USBInterfaceDescription* interface, USBInterfacedDevice* owner,
	UInt8 interfaceNum, UInt8 setting ) :
	USBInterface( interface, owner, interfaceNum, setting ), modifiers( 0 )
{
	UInt32					i;
	for( i = 0; i < 6; i++ )
		keys[i] = 0;
	bufferPos = bufferEnd = buffer;
	
	keyboard.setIStream( this );
}

USBBootKeyboardInterface::~USBBootKeyboardInterface()
{
	keyboard.setIStream( nil );
}

void
USBBootKeyboardInterface::threadLoop()
{
	// Try to set to boot protocol
	USBErr					err = kUSBErr_None;
	USBControlSetup			cmd;
	USBCommandFactory::hidSetProtocol( cmd, getInterfaceNum(), true );
	err = sendClassCommand( &cmd );
	if( !err )
	{
		// Try to shut up, if possible
		USBCommandFactory::hidSetIdle( cmd, getInterfaceNum(), 0, 0 );
		sendClassCommand( &cmd );
		
		// Get interrupt pipe
		USBPipe				*intPipe;
		intPipe = &getInterface()->pipes[0];		// Should be 0
		
		// Loop until disconnected
		while( getOwner()->connected() )
		{
			UInt8			rawBuffer[8];
			interruptTransfer( intPipe, rawBuffer, 8 );
			
			// Ignore errors
			if( rawBuffer[2] == 0x01 )
				continue;
			
			// Look for new keys
			UInt32			i, j;
			Boolean			foundOrig[6], foundNew[6];
			for( i = 0; i < 6; i++ )
				foundOrig[i] = foundNew[i] = false;
			
			for( i = 0; i < 6; i++ )
			{
				if( keys[i] == 0x00 )
					break;
				
				for( j = 0; j < 6; j++ )
				{
					if( keys[i] == rawBuffer[j+2] )
						foundOrig[i] = foundNew[j] = true;
				}
			}
			
			// Any valid keys in keys[] not found have been released
			
			// Any valid keys in rawBuffer[] not found have been pressed
			Boolean			shiftPressed = ((rawBuffer[0] & 0x02) | (rawBuffer[0] & 0x20));
			if( shiftPressed )
				shiftPressed = true;		// Normalize to 0 or 1
			for( i = 0; i < 6; i++ )
			{
				if( !foundNew[i] )
				{
					ASCII8	theChar;
					theChar = usbKeyMap[rawBuffer[i+2]][shiftPressed];
					if( theChar )
					{
						*bufferEnd++ = theChar;
						if( bufferEnd >= buffer+kMaxKeyboardBufferSize )
							bufferEnd = buffer;
					}
				}
			}
			
			// Copy rawBuffer to keys
			for( i = 0; i < 6; i++ )
				keys[i] = rawBuffer[i+2];
		}
	}
	else
	{
		usbOut << "Boot protocol enabling failed! Error: " << gUSBErrorStrings[err] << "\n";
	}
}

void
USBBootKeyboardInterface::read(Int8* data,UInt32 len)
{
	while( len-- > 0 )
		*data++ = readChar();
}

ASCII8
USBBootKeyboardInterface::readChar()
{
	while( bufferPos == bufferEnd )		// Wait for a character
		CurrThread::yield();
	
	ASCII8		theChar;
	theChar = *bufferPos++;
	if( bufferPos >= buffer+kMaxKeyboardBufferSize )
		bufferPos = buffer;
	
	return theChar;
}