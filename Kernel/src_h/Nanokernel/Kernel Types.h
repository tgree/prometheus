/*
	Types.h
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
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __PROMETHEUS_TYPES__
#define __PROMETHEUS_TYPES__
#define __TYPES__	// Some magic to make PEFBinaryFormat.h work

#ifndef __MACTYPES__
#ifndef nil
#define	nil	0
#endif
#endif

#define	true	1
#define	false	0

// Integer types
typedef int			Int;		// Evil, do not use!!
typedef char			Int8;
typedef signed char		SInt8;
typedef unsigned char	UInt8;
typedef signed short		Int16;
typedef unsigned short	UInt16;
typedef signed long		Int32;
typedef unsigned long	UInt32;
typedef signed long long	Int64;
typedef unsigned long long	UInt64;

// Boolean
#ifndef __MACTYPES__
typedef char*			Ptr;
typedef const char*		ConstPtr;
typedef char			Boolean;	// (0 = false, !0 = true)
#endif

// String types.  Wherever you see an ASCII7 you can use ASCII codes 0 - 127.  For ASCII8 you can use codes 0 - 255.
typedef char			ASCII7;
typedef char			ASCII8;
typedef char*			ASCII7Str;
typedef const char*		ConstASCII7Str;
typedef char*			ASCII8Str;
typedef const char*		ConstASCII8Str;
typedef UInt16			UniChar;
typedef UniChar*		UniStr;
typedef const UniChar*	ConstUniStr;

// Device register types
typedef volatile signed char	Reg8;
typedef volatile unsigned char	UReg8;
typedef volatile signed short	Reg16LE;
typedef volatile signed short	Reg16BE;
typedef volatile unsigned short	UReg16LE;
typedef volatile unsigned short	UReg16BE;
typedef volatile signed long	Reg32LE;
typedef volatile signed long	Reg32BE;
typedef volatile unsigned long	UReg32LE;
typedef volatile unsigned long	UReg32BE;

// Floating-point types
typedef float				Float32;
typedef short double			Float64;	// That's how MacTypes.h defines it, so this is to not conflict with them.

#ifndef __MACTYPES__

#define __KILL_BASIC_TYPES__
#define	char		DIE CHAR DIE
#define	short	DIE SHORT DIE
#define	int		DIE INT DIE
#define	long		DIE SHORT DIE

typedef struct Rect
{
	Int32	x1;
	Int32	y1;
	Int32	x2;
	Int32	y2;
}Rect;

typedef struct MacOSRect
{
	Int16	top;
	Int16	left;
	Int16	bottom;
	Int16	right;
}MacOSRect;

typedef struct MacOSPoint
{
	Int16	v;
	Int16	h;
}MacOSPoint;

typedef UInt32	OSType;

#endif /*__MACTYPES__*/

#endif /*__PROMETHEUS_TYPES__*/