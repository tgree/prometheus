/*
	Stupid ANSI Stuff.cp
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
	ATA Demo.c			Apple Developer Support	Vinne Moscaritolo	DrvrRefToName taken from here
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "Types.h"
#include "ANSI.h"

Int32 strcmp(ConstASCII8Str s1,ConstASCII8Str s2,Boolean caseSensitive)
{
	register long	diff = 0;
	
	while(*s1 && *s2)
	{
		char	charS1 = *s1++;
		char	charS2 = *s2++;
		
		if((diff = charS1 - charS2) != 0)
		{
			if(caseSensitive)
				return diff;
			else if(!(charS1 >= 'A' && charS1 <= 'z' && charS2 >= 'A' && charS2 <= 'z' && (diff == 32 || diff == -32)))
				return diff;
		}
	}
	
	if(*s1)
		return 1;
	
	if(*s2)
		return -1;
	
	return 0;
}

Int32 strncmp(ConstASCII8Str s1,ConstASCII8Str s2,UInt32 n)
{
	register long	diff = 0;
	
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

char* strcpy(char* dest,const char* src)
{
	char*	p = dest;
	do
	{
		*p++ = *src;
	} while( *src++ );
	
	return dest;
}

UInt32 strlen(const char* src)
{
	const char*	p = src;
	long			len = 0;
	while( *p )
	{
		p++;
		len++;
	}
	
	return len;
}

char* strcat(char* dest,const char* src)
{
	char*	p = dest + strlen(dest);
	do
	{
		*p++ = *src;
	} while( *src++ );
	
	return dest;
}

void* memcpy(void* s1,const void* s2,unsigned long n)
{
	void* olds1 = s1;
	while(n--)
		*((UInt8*)s1)++ = *((UInt8*)s2)++;
	return olds1;
}
