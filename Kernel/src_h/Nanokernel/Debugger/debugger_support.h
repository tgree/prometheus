/*
	debugger_suppory.h
	Copyright © 1998 by Sean Crist and Patrick Varilly

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
#ifndef DEBUGGER_SUPPORT
#define DEBUGGER_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif
/* The following is the prototype for our debugger parser (use parseLine
    below, though). */
Boolean parseDebug(void);

/* The following is the prototype for our lexical analyzer. */
Int16 getToken(void);

/* The following are the replacement for stdin. */
void init_input (ASCII8Str buf);
Int32 get_next_char( void );
void push_back_char( ASCII8 c );
Int32 parse_line( ASCII8Str line );

/* The following two routines are called by the lexical analyzer when it
   gets a hex or octal number, respectively, to convert it to an integer
   value from the string.  I couldn't find any standard utility for doing 
   this. */
Int32 hex_atoi (ASCII8Str s);
Int32 oct_atoi (ASCII8Str s);
Int32 atoi (ASCII8Str s);

#ifdef __cplusplus
}
#endif

#endif

