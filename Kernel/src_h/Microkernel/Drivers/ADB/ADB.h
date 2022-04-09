/*
	ADB.h
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
	Terry Greeniaus	-	Sat, 13 June 98	-	Added return value for InitADB() so we know if it worked or not
	Patrick Varilly		-	Sat, 20 June 98	-	Return value for InitADB() is gone again!
	Patrick Varilly		-	Mon, 29 March 99	-	Added ADBPresent in case ADB is not present in the machine (e.g.,
											in an iMac or a Yosemite G3)
*/
#ifndef __ADB__HARDWARE__
#define __ADB__HARDWARE__

void InitADB( void );
Boolean ADBPresent( void );
void PrintADBInfo( void );
void Restart(void);
void ShutDown(void);

#endif /* !__ADB__HARDWARE__ */