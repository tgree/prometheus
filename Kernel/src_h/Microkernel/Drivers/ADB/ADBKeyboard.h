/*
	ADBKeyboard.h
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
*/
#ifndef __ADB__KEYBOARD__
#define __ADB__KEYBOARD__

#include "Streams.h"

extern IStreamWrapper	keyboard;

void InitKeyboard( void );
void InitLEDThread(void);

#endif /* !__ADB__KEYBOARD__ */