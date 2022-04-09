/*
	Config.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __CONFIG__
#define __CONFIG__

#define	COMPILING_PROMETHEUS		// Only define this when compiling the kernel

#define	nanokernelVers		"0.10a5"
#define	microKernelVers	"0.10a5"

// Use multiple-word instructions
#define	USE_MW_INST		1
#if !USE_MW_INST
#define	lmw		#error
#define	stmw	#error
#endif

// Time values
#define	ONE_SEC_NANOSECONDS	(1*1000*1000*1000)
#define	ONE_MS_NANOSECONDS	(ONE_SEC_NANOSECONDS/1000)
#define	ONE_US_NANOSECONDS	(ONE_MS_NANOSECONDS/1000)

// Display IO memory mappings when made
#define	DISP_IO_MAP	0

// Mis-align new() memory blocks for stress testing (misaligns by NEW_MISALIGNMENT bytes)
#define	NEW_MISALIGNMENT	0

// Zap new() memory blocks on allocation (for stress testing)
#define	ZAP_NEW	1
#if ZAP_NEW
#define	ZAP_NEW_VALUE	0xBB
#endif

// Display keypresses in trace console when flower-option is down
#define	KEYPRESS_VALUE_DISPLAY	1

// Use '\n' instead of '\r' for "Return" and "Enter" keys on keyboard
#define	KEYBOARD_0x0A_RETURN	0	// Set this to 1 for '\n' returns and 0 for '\r' returns
#if KEYBOARD_0x0A_RETURN
#define	KEYBOARD_RETURN			0x0A	// '\n'
#else
#define	KEYBOARD_RETURN			0x0D	// '\r'
#endif

// Keeps track of bad allocations/releases
#define	SAFE_NEW_DELETE			1

// Size of the preempter stack.
#define	PREEMPTER_STACK_SIZE	2048

// Whether or not to have USB drivers turned on
#define	USB_DRIVERS				1

// Mouse speed stuff - if(delta > DELTA_FAST) delta = delta*DELTA_NUM/DELTA_DENOM;
// This is done in MouseHandler.
#define	DELTA_FAST	6	// After DELTA_FAST change, we start multiplying
#define	DELTA_NUM	5
#define	DELTA_DENOM	2

// USB mouse speed stuff - USB values are larger than ADB values, so we try to normalize them a bit
// here.  Similar equations as the one above, except that this one is applied in USBBootMouse and it is
// applied to every delta.
#define	USB_NORMALIZE_NUM	1
#define	USB_NORMALIZE_DENOM	1

// Pat: What names?
// Maximum length of names kept by the kernel (can't use string pointers since they could be in other address spaces)
#define	MAX_KSTR_LEN			32

#endif /* __CONFIG__ */
