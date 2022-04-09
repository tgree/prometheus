/*
	ADBMouse.h
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
	none
	
	Version History
	============
	Patrick Varilly		-	Thur, 22 Jan 98	-	Original creation of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Patrick Varilly		-	Friday, 12 June 98	-	Updated for new ADB class support
	Terry Greeniaus	-	Tuesday, 16 June 98	-	Added shield/unshieldMouse() stuff.
*/
#ifndef __ADB__MOUSE__
#define __ADB__MOUSE__

#include "NKAtomicOperations.h"
#include "NKMemoryManager.h"

void InitMouse( void );

class MouseHandler	:	public KernelObject
{
protected:
	friend class	ADBMouse;
	friend class	USBBootMouseInterface;
	
	MouseHandler	*next;
	Process		*process;
	
	static MouseHandler		*head;
	static SpinLock			listLock;
	static SpinLock			mouseLock;
	static void	tellAllMouseMoves( Int8 deltaX, Int8 deltaY );
	static void	tellAllMouseClicked( UInt8 numButton, Boolean clicked );
	static void	tellAllMousePosSet( Int32 newX, Int32 newY );
	static void	tellAllShieldMouse();
	static void	tellAllUnshieldMouse();
	
	virtual ~MouseHandler();
public:
				MouseHandler();
				
	virtual void	mouseMoved( Int8 deltaX, Int8 deltaY ) = 0;
	virtual void	mouseClicked( UInt8 numButton, Boolean clicked ) = 0;
	virtual void	mousePosSet( Int32 newX, Int32 newY ) = 0;
	virtual void	shieldMouse() = 0;
	virtual void	unshieldMouse() = 0;
	
	friend class MouseShield;
};

struct MouseShield
{
	inline MouseShield()		{MouseHandler::tellAllShieldMouse();}
	inline ~MouseShield()	{MouseHandler::tellAllUnshieldMouse();}
};

#endif /* !__ADB__HARDWARE__ */