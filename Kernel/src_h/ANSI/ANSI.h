/*
	ANSI.h
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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Thursday, 18 June 98	-	Added srand() and rand() functions
	Patrick Varilly		-	Saturday, 20 June 98	-	Added strcpy() and strncpy() functions
	Terry Greeniaus	-	Wednesday, 22 July 98	-	Added memcpy() function
*/
#ifndef __ANSI_LIBRARY__
#define __ANSI_LIBRARY__

#include "Kernel Types.h"
#include "Macros.h"

// Limits.h
#define	CHAR_BIT		8
#define	MB_LEN_MAX	1
#define	CHAR_MIN	-128
#define	CHAR_MAX	127
#define	SCHAR_MIN	-128
#define	SCHAR_MAX	127
#define	UCHAR_MAX	255
#define	SHRT_MIN		-32768
#define	SHRT_MAX	32767
#define	USHRT_MAX	65535
#define	LONG_MIN		-2147483648
#define	LONG_MAX		2147483647
#define	ULONG_MAX	4294967295
#define	INT_MIN		LONG_MIN
#define	INT_MAX		LONG_MAX
#define	UINT_MAX	ULONG_MAX

// ctype.h
UInt32	isalpha(ASCII8 c);
UInt32	isupper(ASCII8 c);
UInt32	islower(ASCII8 c);
UInt32	isdigit(ASCII8 c);
UInt32	isxdigit(ASCII8 c);
UInt32	isalnum(ASCII8 c);
UInt32	isspace(ASCII8 c);
UInt32	ispunct(ASCII8 c);
UInt32	isprint(ASCII8 c);
UInt32	isgraph(ASCII8 c);
UInt32	iscntrl(ASCII8 c);
UInt32	isascci(UInt32 c);

// stdarg.h
typedef Ptr	va_list;
#define	va_start(ap, parmN)	(ap) = (Ptr)&(parmN)
#define	va_arg(ap, type)	*(type*)((ap) = (ap) + (ROUND_UP(4,sizeof(type))) )
#define	va_end(ap)

// string.h
UInt32		strlen(ConstASCII8Str str);
Int32		strcmp(ConstASCII8Str s1,ConstASCII8Str s2,Boolean caseSensitive = 1);
Int32		strncmp(ConstASCII8Str s1,ConstASCII8Str s2,UInt32 n);
ASCII8Str		strcat(ASCII8Str s1, ConstASCII8Str s2);
ASCII8Str		strcpy(ASCII8Str dest, ConstASCII8Str src);
ASCII8Str		strncpy(ASCII8Str dest, ConstASCII8Str src, UInt32 n);
void*		memcpy(void* s1,const void* s2,UInt32 n);
void*		memset(void* s,UInt8 c,UInt32 n);

// stdlib.h
void*		malloc(UInt32 size);
void			free(void* ptr);
void*		realloc(void* ptr,UInt32 size);
void*		calloc(UInt32 nmemb,UInt32 membsize);
void			srand(UInt32 seed);
UInt32		rand();

// non-standard stuff
ASCII8Str		num2str(Int32 n,ASCII8Str str);		// str must have 12 bytes of space
ASCII8Str		unum2str(UInt32 n,ASCII8Str str);		// str must have 11 bytes of space
ASCII8Str		num2str64(Int64 n,ASCII8Str str);		// str must have 21 bytes of space
ASCII8Str		unum2str64(UInt64 n,ASCII8Str str);	// str must have 20 bytes of space
ASCII8Str		num2hex(UInt32 n,ASCII8Str str);		// str must have 11 bytes of space
ASCII8Str		num2hex64(UInt64 n,ASCII8Str str);	// str must have 19 bytes of space

#endif /* __ANSI_LIBRARY__ */