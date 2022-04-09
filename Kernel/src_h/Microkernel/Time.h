/*
	Time.h
	Copyright © 1998 by Terry Greeniaus

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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Patrick Varilly		-	Sunday, 16 Jan 2000	-	Revised to use Float64s instead of UInt64s (64-bit division is *horribly* expensive, about 10
												times more so than converting a UInt64 to a Float64, doing fp division and converting back)
*/
#ifndef __TIMER__
#define __TIMER__

#include "NKMachineInit.h"

void InitTime(void);

void Wait_ns(Float64 nsec);	// A nanosecond is a billionth of a second
void Wait_us(Float64 usec);	// A microsecond is a millionth of a second
void Wait_ms(Float64 msec);	// A millisecond is a thousandth of a second
void Wait_s(Float64 sec);		// And I think you know what a second is
Float64 GetTime_ns(void);		// Returns the number of nanoseconds since kernel startup
Float64 GetTime_us(void);
Float64 GetTime_ms(void);
Float64 GetTime_s(void);

UInt64 GetClock(void);
inline UInt64 nsecToClock(Float64 nsec) {return (UInt64)(nsec*machine.clockRatio.nsToClocks);}
inline Float64 clockToNsec(UInt64 clock) {return ((Float64)clock)*machine.clockRatio.clocksToNS;}
inline Float64 GetTime_ns(void) { return clockToNsec(GetClock()); }
inline Float64 GetTime_us(void) { return GetTime_ns()/1000.; }
inline Float64 GetTime_ms(void) { return GetTime_ns()/1000000.; }
inline Float64 GetTime_s(void) { return GetTime_ns()/1000000000.; }

#endif /*__TIMER__*/
