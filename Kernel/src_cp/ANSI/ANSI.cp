/*
	ANSI.cp
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
	Terry Greeniaus	-	Thursday, 18 June 98	-	Added srand() and rand() functions
	Terry Greeniaus	-	Friday, 19 June 98		-	Changed srand() and rand() functions to those used in MSL, which
												are apparently those defined in the ANSI standard.
	Patrick Varilly		-	Saturday, 20 June 98	-	Added strcpy() and strncpy() functions
*/
#include "Kernel Types.h"
#include "ANSI.h"
#include "NKDebuggerNub.h"
#include "NKProcesses.h"
#include "NKVirtualMemory.h"
#include "Memory Utils.h"

// ************** ctype.cp ***********************
UInt32 isalpha(ASCII8 c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

UInt32 isupper(ASCII8 c)
{
	return (c >= 'A' && c <= 'Z');
}

UInt32 islower(ASCII8 c)
{
	return (c >= 'a' && c <= 'z');
}

UInt32 isdigit(ASCII8 c)
{
	return (c >= '0' && c <= '9');
}

UInt32 isxdigit(ASCII8 c)
{
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

UInt32 isalnum(ASCII8 c)
{
	return isalpha(c) || isdigit(c);
}

UInt32 isspace(ASCII8 c)
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 11 || c == 12);
}

UInt32 iscntrl(ASCII8 c)
{
	return (c == 127 || c < 32);
}

UInt32 ispunct(ASCII8 c)
{
	return (!isalnum(c) && !isspace(c) && !iscntrl(c));
}

UInt32 isprint(ASCII8 c)
{
	return (c >= 32 && c <= 126);
}

UInt32 isgraph(ASCII8 c)
{
	return (isalnum(c) || ispunct(c));
}

UInt32 isascci(UInt32 c)
{
	return (c < 128);
}

// ************** string.cp ***********************
UInt32 strlen(ConstASCII8Str str)
{
	UInt32 count = 0;
	
	while(*str++)
		count++;
	
	return count;
}

Int32 strcmp(ConstASCII8Str s1,ConstASCII8Str s2,Boolean caseSensitive)
{
	register Int32	diff = 0;
	
	while(*s1 && *s2)
	{
		ASCII8	charS1 = *s1++;
		ASCII8	charS2 = *s2++;
		
		if(!caseSensitive)
		{
			if(charS1 >= 'A' && charS1 <= 'Z')
				charS1 += 32;
			if(charS2 >= 'A' && charS2 <= 'Z')
				charS2 += 32;
		}
		
		if((diff = charS1 - charS2) != 0)
			return diff;
	}
	
	if(*s1)
		return 1;
	
	if(*s2)
		return -1;
	
	return 0;
}

Int32 strncmp(ConstASCII8Str s1,ConstASCII8Str s2,UInt32 n)
{
	register Int32	diff = 0;
	
	while(*s1 && *s2 && n)
	{
		n--;
		if((diff = *s1++ - *s2++) != 0)
			return diff;
	}
	
	if(n == 0 || (!*s1 && !*s2))
		return 0;
	
	if(*s1)
		return 1;
	
	if(*s2)
		return 2;
	
	return 0;
}

ASCII8Str strcat(ASCII8Str dest,ConstASCII8Str src)
{
	ASCII8Str p = dest + strlen(dest);
	do
	{
		*p++ = *src;
	}while(*src++);
	
	return dest;
}

ASCII8Str strcpy(ASCII8Str dest, ConstASCII8Str src)
{
	ASCII8Str	p = dest;
	do
	{
		*p++ = *src;
	} while( *src++ );
	
	return dest;
}

ASCII8Str strncpy(ASCII8Str dest, ConstASCII8Str src, UInt32 n)
{
	ASCII8Str	p = dest;
	
	do
	{
		*p++ = *src;
	} while( (*src++) && (n-- > 0) );
	*--p = 0;
	
	return dest;
}

void* memcpy(void* s1,const void* s2,UInt32 n)
{
	MemCopy( s2, s1, n );
	return s1;
}

void* memset(void* s,UInt8 c,UInt32 n)
{
	MemSet( s, n, c );
	return s;
}

// ****************** stdlib.cp ******************************
static Int32 seed = 1;

void* malloc(UInt32 size)
{
	FatalAssert(size != 0);
	return CurrProcess::process()->toc.allocateBlock(size,PP_READ_WRITE);
}

void free(void* ptr)
{
	FatalAssert(ptr != nil);
	CurrProcess::process()->toc.releaseBlock(ptr);
}

void* realloc(void* ptr,UInt32 size)
{
	FatalAssert(size != 0);
	if(!ptr)
		return malloc(size);
	
	void*	newPtr = CurrProcess::process()->toc.allocateBlock(size,PP_READ_WRITE);
	memcpy(newPtr,ptr,size);
	CurrProcess::process()->toc.releaseBlock(ptr);
	
	return newPtr;
}

void* calloc(UInt32 nmemb,UInt32 memsize)
{
	void*	ptr = CurrProcess::process()->toc.allocateBlock(nmemb*memsize,PP_READ_WRITE);
	memset(ptr,0,nmemb*memsize);
	return ptr;
}

void srand(UInt32 newSeed)
{
	seed = newSeed;
}

UInt32 rand(void)
{
	seed = seed*1103515245 + 12345;
	return ((seed >> 16) & 0x7FFF);
}

// ****************** non-standard stuff ******************************
static ASCII8 num2hex_table[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

ASCII8Str num2str(Int32 n,ASCII8Str str)
{
	ASCII8Str	origStr = str;
	
	if(n < 0)
	{
		str[0] = '-';
		str++;
		n = -n;
	}
	
	unum2str(n,str);
	return origStr;
}

ASCII8Str unum2str(UInt32 n,ASCII8Str str)
{
	if(n == 0)
	{
		str[0] = '0';
		str[1] = '\0';
		return str;
	}
	
	UInt32	digit = 0;
	while(n)
	{
		str[digit++] = ((n % 10) + 48);
		n /= 10;
	}
	str[digit] = '\0';
	
	ReverseMem(str,digit);
	return str;
}

ASCII8Str num2str64(Int64 n,ASCII8Str str)
{
	ASCII8Str	origStr = str;
	if(n < 0)
	{
		str[0] = '-';
		str++;
		n = -n;
	}
	
	unum2str(n,str);
	return origStr;
}

ASCII8Str unum2str64(UInt64 n,ASCII8Str str)
{
	if(n == 0)
	{
		str[0] = '0';
		str[1] = '\0';
		return str;
	}
	
	UInt32	digit = 0;
	while(n)
	{
		str[digit++] = ((n % 10) + 48);
		n /= 10;
	}
	str[digit] = '\0';
	
	ReverseMem(str,digit);
	return str;
}

ASCII8Str num2hex(UInt32 n,ASCII8Str str)
{
	str[0] = '0';
	str[1] = 'x';
	str[2] = num2hex_table[(n >> 28) & 0x0000000F];
	str[3] = num2hex_table[(n >> 24) & 0x0000000F];
	str[4] = num2hex_table[(n >> 20) & 0x0000000F];
	str[5] = num2hex_table[(n >> 16) & 0x0000000F];
	str[6] = num2hex_table[(n >> 12) & 0x0000000F];
	str[7] = num2hex_table[(n >> 8) & 0x0000000F];
	str[8] = num2hex_table[(n >> 4) & 0x0000000F];
	str[9] = num2hex_table[n & 0x0000000F];
	str[10] = '\0';
	
	return str;
}

ASCII8Str num2hex64(UInt64 n,ASCII8Str str)
{
	str[0] = '0';
	str[1] = 'x';
	str[2] = num2hex_table[(n >> 60) & 0x0000000F];
	str[3] = num2hex_table[(n >> 56) & 0x0000000F];
	str[4] = num2hex_table[(n >> 52) & 0x0000000F];
	str[5] = num2hex_table[(n >> 48) & 0x0000000F];
	str[6] = num2hex_table[(n >> 44) & 0x0000000F];
	str[7] = num2hex_table[(n >> 40) & 0x0000000F];
	str[8] = num2hex_table[(n >> 36) & 0x0000000F];
	str[9] = num2hex_table[(n >> 32) & 0x0000000F];
	str[10] = num2hex_table[(n >> 28) & 0x0000000F];
	str[11] = num2hex_table[(n >> 24) & 0x0000000F];
	str[12] = num2hex_table[(n >> 20) & 0x0000000F];
	str[13] = num2hex_table[(n >> 16) & 0x0000000F];
	str[14] = num2hex_table[(n >> 12) & 0x0000000F];
	str[15] = num2hex_table[(n >> 8) & 0x0000000F];
	str[16] = num2hex_table[(n >> 4) & 0x0000000F];
	str[17] = num2hex_table[n & 0x0000000F];
	str[18] = '\0';
	
	return str;
}