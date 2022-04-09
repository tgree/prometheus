/*
	ADBMouse.cp
	
	Code for the ADB mouse driver
	Adapted from the MkLinux Mach kernel sources (extremely little is left from the original sources)
		Corresponding copyright notices follow.
	
	Copyright © 1998 by The Pandora Team. All rights reserved worldwide.
	Permission to use and modify this file is given solely to the Pandora Team until further notice
*/

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
 * work original based on adb.c from NetBSD-1.0
 */

/*	$NetBSD: adb.c,v 1.3 1995/06/30 01:23:21 briggs Exp $	*/

/*-
 * Copyright (C) 1994	Bradley A. Grantham
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Bradley A. Grantham.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
	Version History
	============
	Patrick Varilly			Monday, 26 Jan 98		Original creation of file
	Patrick Varilly			Thursday, 28 May 98	Extensive modifications for the introduction of
												the mouse queue.
	Patrick Varilly			Friday, 29 May 98		Even more modifications for the introduction of the HAL
												(see HALMouse.cp and SOSMouse.cp)
	Patrick Varilly			Thursday, 11 Jun 98		HAL scrapped, used much more elegant method proposed
												in the list.
	Terry Greeniaus		Tuesday, 16 June 98		Added shield/unshieldMouse() stuff.
	Terry Greeniaus		Friday, 19 June 98		Added next = nil line to MouseHandler().  This turned up when I stopped
											MemSetting operator new blocks to 0.
	Terry Greeniaus		Tuesday, 2 Feb 2000		Added critical sections for each mouse so that they won't be called
											reentrantly (drawing the mouse while moving it could be bad).
*/

#include "Kernel Types.h"
#include "ADBInternals.h"
#include "ADBMouse.h"
#include "NKProcesses.h"
#include "Macros.h"

#define pos(x)			((x) & 0x3f)
#define posIsNegative(x)	((x) & 0x40)
#define button(x)		(((x) & 0x80) == 0x80)

class ADBMouse : public ADBDevice
{
	Boolean			buttonUp;

public:
					ADBMouse();
	virtual void		handleDevice( UInt8 deviceNum, UInt8* data, UInt32 dataSize );
};

static ADBMouse		*mouse = nil;
MouseHandler*			MouseHandler::head = nil;
SpinLock				MouseHandler::listLock;
SpinLock				MouseHandler::mouseLock;

void InitMouse( void )
{
	mouse = new ADBMouse;
}

ADBMouse::ADBMouse()
	: buttonUp(true)
{
	// Actually initialize the mouse handler
	UInt8		firstMouse;
	
	firstMouse = registerByType( kADBDeviceMouse );
	
	// Some 3-button mice switch to one-button emulation when the bus is reset.
	// This is solved by setting their handler ID to 4.
	setHandlerID( firstMouse, 4 );
}

void
ADBMouse::handleDevice( UInt8 deviceNum, UInt8* data, UInt32 dataSize )
{
	#pragma unused(deviceNum,dataSize)
	
	if( pos(data[0]) || pos(data[1]) )
	{
		// Mouse position has changed
		Int8	changeX, changeY;
		
		// the change values are two's complements, but this doesn't include the top two bits, which
		// indicate whether the button is pressed (top bit) and whether the value is negative (second to top bit)
		changeY = pos(data[0]);
		if( posIsNegative(data[0]) )	changeY = -((changeY ^ 0x3f) + 1);	// Trick to make changeY negative
		changeX = pos(data[1]);
		if( posIsNegative(data[1]) )	changeX = -((changeX ^ 0x3f) + 1);	// Trick to make changeX negative
		
		// Report the change to the mouse handlers
		MouseHandler::tellAllMouseMoves( changeX, changeY );
	}
	
	if( button(data[0]) != buttonUp )
	{
		// Button state changed
		// Support multiple buttons later
		// Report the change to the mouse handlers
		buttonUp = button(data[0]);
		
		MouseHandler::tellAllMouseClicked( 0, !buttonUp );
	}
}

void
MouseHandler::tellAllMouseMoves( Int8 deltaX, Int8 deltaY )
{
	CriticalSection		criticalLock(mouseLock);
	
	MouseHandler*		next = head;
	
	if(ABS(deltaX) >= DELTA_FAST)
		deltaX = deltaX*DELTA_NUM/DELTA_DENOM;
	if(ABS(deltaY) >= DELTA_FAST)
		deltaY = deltaY*DELTA_NUM/DELTA_DENOM;
	
	while( next )
	{
		ProcessWindow			window(next->process);
		next->mouseMoved( deltaX, deltaY );
		next = next->next;
	}
}

void
MouseHandler::tellAllMouseClicked( UInt8 numButton, Boolean clicked )
{
	CriticalSection		criticalLock(mouseLock);
	
	MouseHandler*		next = head;
	
	while( next )
	{
		ProcessWindow			window(next->process);
		next->mouseClicked( numButton, clicked );
		next = next->next;
	}
}

void
MouseHandler::tellAllMousePosSet( Int32 newX, Int32 newY )
{
	CriticalSection		criticalLock(mouseLock);
	
	MouseHandler*		next = head;
	
	while( next )
	{
		ProcessWindow			window(next->process);
		next->mousePosSet( newX, newY );
		next = next->next;
	}
}

void
MouseHandler::tellAllShieldMouse()
{
	CriticalSection		criticalLock(mouseLock);
	
	MouseHandler*		next = head;
	
	while( next )
	{
		ProcessWindow			window(next->process);
		next->shieldMouse();
		next = next->next;
	}
}

void
MouseHandler::tellAllUnshieldMouse()
{
	CriticalSection		criticalLock(mouseLock);
	
	MouseHandler*		next = head;
	
	while( next )
	{
		ProcessWindow			window(next->process);
		next->unshieldMouse();
		next = next->next;
	}
}

MouseHandler::MouseHandler()
{
	CriticalSection		locker(listLock);
	
	next = nil;
	if( !head )
		head = this;
	else
	{
		MouseHandler		*next = head;
		while( next->next )
			next = next->next;
		next->next = this;
	}
	
	process = CurrProcess::process();
}

MouseHandler::~MouseHandler()
{
	CriticalSection			locker(listLock);
	
	MouseHandler*	currHandler = head;
	MouseHandler*	prev = nil;
	while(currHandler)
	{
		if(currHandler == this)
		{
			if(prev)
				prev->next = next;
			else
				head = next;
			break;
		}
		prev = currHandler;
		currHandler = currHandler->next;
	}
}
