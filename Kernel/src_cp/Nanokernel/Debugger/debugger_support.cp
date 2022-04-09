/*
	debugger_support.cp
	Copyright © 1998 by Sean Crist

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
	???
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Modified GNU license??  Is this OK?
*/

/*
   Copyright 1998 by Sean Crist

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

//#include "Memory Manager.h"
#include "debugger_support.h"
#include "NKDebuggerInternal.h"

/* Routines related to getting characters from the input string. */

ASCII8*	placeCounter;

void init_input (ASCII8Str buf)
{
  placeCounter = buf;
}

Int32 get_next_char( void )
{
	return *placeCounter++;
}

void push_back_char( ASCII8 c )
{
	*--placeCounter = c;
}

Int32 parse_line( ASCII8Str line )
{
	init_input( line );
	return parseDebug();
}

/* 
hex_atoi and oct_atoi don't check for bad characters in the strings
since the lexical analyzer can be reasonably expected to send only good 
strings.
*/

Int32 hex_atoi (ASCII8Str s)
{
  Int32 i, n, m;

  n=0;
  for (i=0; s[i] != '\0'; ++i)  {
/* Convert the character to an int value. */
    if (s[i] >= '0' && s[i] <= '9') {
      m = s[i] - '0'; }
    else if (s[i] >= 'a' && s[i] <= 'f') {
      m = (s[i] - 'a') + 10; }
    else {
      m = (s[i] - 'A') + 10; }
/* Add this character to our running total. */
    n = (16 * n) + m;
  }

  return (n);
}

Int32 oct_atoi (ASCII8Str s)
{
  Int32 i, n, m;

  n=0;
  for (i=0; s[i] != '\0'; ++i)  {
/* Convert the character to an int value. */
    m = s[i] - '0';
/* Add this character to our running total. */
    n = (8 * n) + m;
  }

  return (n);
}

Int32 atoi (ASCII8Str s)
{
  Int32 i;
  Int32 n;

  n=0;
  for (i=0; s[i] >= '0' && s[i] <= '9'; ++i)
    n = 10 * n + s[i] - '0';

  return(n);
}

