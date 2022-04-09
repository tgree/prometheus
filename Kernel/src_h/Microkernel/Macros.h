/*
	Macros.h
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
#ifndef __MACROS__
#define __MACROS__

// Rounding functions
#define ROUND_UP(k,n)			( (n) + ( ( (k) - ( (n) % (k) ) ) % (k) ) )	// Rounds n up to the nearest multiple of k.  If n is already a multiple of k, it is unchanged
#define ROUND_DOWN(k,n)		( (n) - ( (n) % (k) ) )					// Rounds n down to a multiple of k.  If n is already a multiple of k, it is unchanged
#define ROUND_UP_POW_2(k,n)	( (n) + ( ( (1 << (k) ) - ( (n) & ( (1 <<  (k) ) - 1) ) ) & ( (1 << (k) ) - 1) ) )	// Rounds n up to the nearest power of 2.
#define ROUND_DOWN_POW_2(k,n)	( (n) & ~( (1 << (k) ) - 1) )			// Rounds n down to the nearest power of 2.
#define ROUND(k,n)				( (((n) + (k)/2)/(k))*(k) )				// Rounds n to the nearest multiple of k

// Max/min
#define MAX(a,b)				( ((a) > (b)) ? (a) : (b) )
#define MIN(a,b)				( ((a) < (b)) ? (a) : (b) )

// Absolute value
#define ABS(a)					( ((a) < 0) ? -(a) : (a) )

// Get the address of a function from a pointer-to-function (TVector)
#define FUNC_ADDR(func)			(((UInt32*)func)[0])
#define FUNC_RTOC(func)			(((UInt32*)func)[1])

// Copies a TVector from 8 bytes of memory to another 8 bytes of memory
#define COPY_TVECTOR(src,dst)	do{ ((UInt32*)&(dst))[0] = ((UInt32*)(src))[0]; ((UInt32*)&(dst))[1] = ((UInt32*)(src))[1]; } while(0)

// In-place byte-reversing
#define BYTE_REVERSE16(x)		((((x) >> 8) & 0xFF) | (((x) & 0xFF) << 8))
#define BYTE_REVERSE32(x)		((((x) >> 24) & 0xFF) | (((x) & 0xFF) << 24) | (((x) & 0xFF00) << 8) | (((x) & 0xFF0000) >> 8))

#if BIG_ENDIAN
#define BE16(x)				(x)
#define LE16(x)				BYTE_REVERSE16(x)
#define BE32(x)				(x)
#define LE32(x)				BYTE_REVERSE32(x)
#else
#define BE16(x)				BYTE_REVERSE16(x)
#define LE16(x)				(x)
#define BE32(x)				BYTE_REVERSE32(x)
#define LE32(x)				(x)
#endif

#endif
