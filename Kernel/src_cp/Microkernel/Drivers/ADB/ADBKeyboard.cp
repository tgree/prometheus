/*
	CUDADriver.cp
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
	Other sources			Project				Author				Notes
	===========			======				=====				====
	adb.c					MkLinux DR2.1 update 6	Bradley A. Grantham		originally adpated from here
	
	Version History
	============
	Patrick Varilly			Tuesday, 27 Jan 98		Original creation of file
	Patrick Varilly			Thursday, 28 May 98	Added Mach copyright notices
*/
#include "NKDebuggerNub.h"
#include "ADBInternals.h"
#include "ADBKeyboard.h"
#include "ADBKeyboardMaps.h"
#include "Kernel Console.h"
#include "NKVideo.h"
#include "NKThreads.h"
#include "Time.h"
#include "ShutDown.h"
#include "NKInterruptVectors.h"

// Only the keyboard needs to work in no interrupt mode, but this allows any ADB device to respond
// while reading from the keyboard.
void ADBCheckInterrupt( void );

// Special key values
enum
{
	kADBKeyControl					= 0x36,
	kADBKeyControlR					= 0x7D,
	kADBKeyFlower					= 0x37,		/* Command key is the "flower" */
	kADBKeyShift						= 0x38,
	kADBKeyShiftR						= 0x7B,
	kADBKeyCapsLock					= 0x39,
	kADBKeyOption						= 0x3A,
	kADBKeyOptionR					= 0x7C,
	kADBKeyNumLock					= 0x47,
	kADBKeySpace						= 0x31,
	kADBKeyPowerPB					= 0x7E,	// Power key a PowerBook
	kADBKeyPower						= 0x7F
};

#define isPressed(x)						(((x) & 0x80) == 0)
#define keyVal(x)						((x) & 0x7F)
#define isModifier(x)					((keyval(x) == kADBKeyControl) || \
										(keyval(x) == kADBKeyControlR) || \
										(keyval(x) == kADBKeyFlower) || \
										(keyval(x) == kADBKeyShift) || \
										(keyval(x) == kADBKeyShiftR) || \
										(keyval(x) == kADBKeyCapsLock) || \
										(keyval(x) == kADBKeyOption) || \
										(keyval(x) == kADBKeyOptionR) || \
										(keyval(x) == kADBKeyNumLock))

// The maximum size for the keyboard buffer. 128 chars should be plenty (
const UInt32	kMaxKeyboardBufferSize	= 128;

// The state of the LEDs on the keyboard
enum
{
	kKeyboardLEDNumLock				= 0x01,
	kKeyboardLEDCapsLock				= 0x02,
	kKeyboardLEDScrollLock				= 0x04
};

// The bits in ADB register 2 (state register)
enum
{
	kADBKeyboardStateLEDNumLock		= 0x0001,
	kADBKeyboardStateLEDCapsLock		= 0x0002,
	kADBKeyboardStateLEDScrollLock		= 0x0004,
	kADBKeyboardStateScrollLock			= 0x0040,
	kADBKeyboardStateNumLock			= 0x0080,
		/* Bits 3 to 5 are reserved */
	kADBKeyboardStateAppleCmd			= 0x0100,
	kADBKeyboardStateOption				= 0x0200,
	kADBKeyboardStateShift				= 0x0400,
	kADBKeyboardStateControl			= 0x0800,
	kADBKeyboardStateCapsLock			= 0x1000,
	kADBKeyboardStateReset				= 0x2000,
	kADBKeyboardStateDelete				= 0x4000
		/* bit 16 is reserved */
};

class ADBKeyboard : public ADBDevice,
				public IStream
{
protected:
	volatile UInt8*		buffer;	// Buffer is a *cyclic* buffer
	volatile UInt8*		bufferPos;
	volatile UInt8*		bufferEnd;
	volatile UInt8		ledState;
	UInt8			keyboardAddr;
	UInt32			currentKeyboard;
	volatile Boolean		shiftPressed, optionPressed, controlPressed, flowerPressed;
	ADBIOCommand		*setLEDCommand;
	
	void				keyUpDown( UInt8 rawData );
	UInt8			readChar(void);

public:
					ADBKeyboard();
	virtual void		handleDevice( UInt8 deviceNum, UInt8* data, UInt32 dataSize );
	virtual void		read(Int8* data,UInt32 len);
	UInt8			getLEDState(void);
	void				setLEDState(UInt8 newLEDState);
};

struct LEDThread	:	public Thread,
					public ShutDownHandler
{
	UInt8	initialLEDState;
	LEDThread();
	
	virtual	void	threadLoop(void);
	virtual	void	shutDown(Boolean isShutDown);
};

static ADBKeyboard			*keyboardDevice = nil;

IStreamWrapper	keyboard("keyboard");
LEDThread*		ledThread;

void InitKeyboard( void );

void InitLEDThread(void)
{
	ledThread = new LEDThread;
	ledThread->resume();
}

LEDThread::LEDThread():
	Thread(10240,kPriorityDriver,"LEDThread")
{
	initialLEDState = keyboardDevice->getLEDState();
}

void LEDThread::threadLoop(void)
{
	UInt8	ledState[] = {kKeyboardLEDNumLock,kKeyboardLEDCapsLock,kKeyboardLEDScrollLock,kKeyboardLEDCapsLock};
	
	for(UInt32 i=0;;i++)
	{
		keyboardDevice->setLEDState(ledState[ i % (sizeof(ledState)/sizeof(ledState[0])) ]);
		sleepMS(100);
	}
}

void LEDThread::shutDown(Boolean /*isShutDown*/)
{
	suspend();								// Turn off the LED toggler
	keyboardDevice->setLEDState(initialLEDState);	// Reset the LEDS
}

void InitKeyboard( void )
{
	keyboardDevice = new ADBKeyboard;
	keyboard.setIStream(keyboardDevice);
}

ADBKeyboard::ADBKeyboard()
	:Stream("adb keyboard"), shiftPressed(0), optionPressed(0), controlPressed(0), flowerPressed(0), currentKeyboard(0),
		setLEDCommand(nil)
{
	buffer = new UInt8[kMaxKeyboardBufferSize];
	bufferEnd = bufferPos = buffer;
	
	// Actually initialize the keyboard handler
	keyboardAddr = registerByType( kADBDeviceKeyboard );
	
	// Get the state of the keyboard LEDs
	ledState = getLEDState();
}

UInt8
ADBKeyboard::getLEDState(void)
{
	ADBResultCode			result;
	UInt16				value;
	UInt8				state = 0;
	
	result = readReg( keyboardAddr, 2, &value );
	if( result != kADBResultOK )
		return 0;
	
	value = ~value;			// ADB says that 0=set, 1=cleared. Stupid engineers!!!
	
	if (value & (kADBKeyboardStateCapsLock|kADBKeyboardStateLEDCapsLock))
		state |= kKeyboardLEDCapsLock;
	if (value & (kADBKeyboardStateScrollLock|kADBKeyboardStateLEDScrollLock))
		state |= kKeyboardLEDScrollLock;
	if (value & (kADBKeyboardStateNumLock| kADBKeyboardStateLEDNumLock))
		state |= kKeyboardLEDNumLock;
	
	return state;
}

void
ADBKeyboard::setLEDState(UInt8 newLEDState)
{
	UInt16			value = 0;
	
	if ( newLEDState & kKeyboardLEDCapsLock )
		value |= kADBKeyboardStateLEDCapsLock;
	if ( newLEDState & kADBKeyboardStateLEDScrollLock )
		value |= kKeyboardLEDScrollLock;
	if ( newLEDState & kADBKeyboardStateLEDNumLock )
		value |= kKeyboardLEDNumLock;
	
	value = ~value;			// ADB says that 0=set, 1=cleared. Stupid engineers!!!
	
	if( !setLEDCommand )
		setLEDCommand = writeRegAsync( keyboardAddr, 2, value );
	else
	{
		if( setLEDCommand->hasDoneIO )
		{
			delete setLEDCommand;
			setLEDCommand = writeRegAsync( keyboardAddr, 2, value );
		}
	}
		// (Pat): This needs to be written asynchronously because this may be called when interrupts are off.
		// If they are off, we can't receive a reply until they are turned back on again. This means that if this
		// was synchronous, it would wait forever for a reply. It's also just plain efficient.
}

void
ADBKeyboard::handleDevice( UInt8 deviceNum, UInt8* data, UInt32 dataSize )
{
	#pragma unused(deviceNum,dataSize)
	
	keyUpDown( data[0] );
	if( data[1] != 0xFF )
		keyUpDown( data[1] );
}

void
ADBKeyboard::keyUpDown( UInt8 rawData )
{
	/* Support auto-repeat here later */
	Boolean		isKeyPressed;
	UInt8		keyValue;

	isKeyPressed = isPressed(rawData);
	keyValue = keyVal(rawData);

	switch (keyValue)
	{
		case	kADBKeyFlower:
			flowerPressed = isKeyPressed;
			break;

		case	kADBKeyOption:
		case	kADBKeyOptionR:
			optionPressed = isKeyPressed;
			break;

		case	kADBKeyShift:
		case	kADBKeyShiftR:
			shiftPressed = isKeyPressed;
			break;

		case	kADBKeyControl:
		case	kADBKeyControlR:
			controlPressed = isKeyPressed;
			break;

		case	kADBKeyCapsLock:
			if (isKeyPressed)
				ledState |= kKeyboardLEDCapsLock;
			else
				ledState &= ~kKeyboardLEDCapsLock;
			setLEDState(ledState);
			break;

		case	kADBKeyNumLock:
			
			ledState ^= ~kKeyboardLEDNumLock;
			setLEDState(ledState);
			break;

		default:
			if (isKeyPressed)
			{
				switch ( keyValue )
				{
					case kADBKeyPower:
					case kADBKeyPowerPB:
						if( optionPressed )
							debuggerNub->interrupt(externalException,&savedRegs[externalException]);
						else
							goto normalCharacter;
						break;
					case kADBKeySpace:
						/* Michel Pollet: (<-- MkLinux, not Pandora)
						   Look for Cmd-Option space
						   for switching keyboard layout
						*/
						if (flowerPressed && optionPressed)
						{
							if ( (keyMapTable[++currentKeyboard].keyMap) == nil )
								currentKeyboard = 0;
							
							nkVideo << "Keyboard changed to " << (keyMapTable[currentKeyboard].mapName) << "\n";
							return;
						}
						/* Fall */
					default:
					
					normalCharacter:
						Boolean		shiftState = false;
						UInt8		toType;
						if (shiftPressed || (ledState & kKeyboardLEDCapsLock))
							shiftState = true;
						
						toType = (*keyMapTable[currentKeyboard].keyMap)[keyValue][shiftState];
						if( toType != 0 )
							*bufferEnd++ = toType;
						
						if( bufferEnd >= buffer+kMaxKeyboardBufferSize )
							bufferEnd = buffer;
						
						if( flowerPressed && optionPressed && KEYPRESS_VALUE_DISPLAY)
							nkVideo << "Key Value: " << keyValue << "\nASCII: " << toType << "\n";
						break;
				}
			}
			break;
	}
}

void
ADBKeyboard::read(Int8* data,UInt32 len)
{
	while( len-- > 0 )
		*data++ = readChar();
}

UInt8
ADBKeyboard::readChar(void)
{
	while( bufferPos == bufferEnd )		// Wait for a character
	{
		ADBCheckInterrupt();
		if(bufferPos == bufferEnd)
			CurrThread::yield();
	}
	UInt8		theChar;
	
	theChar = *bufferPos++;
	if( bufferPos >= buffer+kMaxKeyboardBufferSize )
		bufferPos = buffer;
	
	return theChar;
}