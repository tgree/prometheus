/*
	debugger_lexan.cp
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
	Other sources			Project				Author			Notes
	===========			======				=====			====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "ANSI.h"
#include "debugger_support.h"
#include "debugger_parser.h"

#define isHex(x)		(((x >= '0') && (x <= '9')) || ((x >= 'a') && (x <= 'f')) || ((x >= 'A') && (x <= 'F')))
static Int32 parseHex( void );

// Lexical analyzer
//
// The rules are:
// * Ignore whitespace
// * Recognize numbered registers (r0 through r31)
// * Recognize octal constants (beginning with ')
// * Recognize hex constants (as in $DEADBEEF or 0xDEADBEEF)
// * Recognize decimal constants
// * Recognize << and >> as single tokens (damned Bison can't do this)
// * Recognize commands in the symbol table
// * Return everything else character by character

typedef struct
{
	ASCII8Str	tokName;
	Int32	tokNum;
} SymTable;

SymTable		commands[] = {		// Commands
						{ "il", tC_IL },		{ "ip", tC_IP },
						{ "st", tC_ST },	{ "help", tC_HELP },
						{ "rs", tC_RS },	{ "sb", tC_SB },
						{ "sh", tC_SH },	{ "sw", tC_SW },
						{ "tl",tC_TL},		{ "dm", tC_DM },
						
							// Special Registers
						{ "pc", tR_PC },	{ "lr", tR_LR },
						{ "sp", tR_SP },	{ "rtoc", tR_RTOC },
						
							// Terminator
						{ "", 0 } };

Int32		yylval;

Int16 getToken(void)
{
	ASCII8			c, nextC, nextNextC;
	
	while( ((c = get_next_char()) == ' ') || (c == '\t') )
		;			// Ignore whitespace
	
	if( c == 0 )
		return 0;
	
	// * Recognize numbered registers (r0 through r31)
	else if( c == 'r' )
	{
		// Could be numbered register, could not.
		nextC = get_next_char();
		if( (nextC < '0') || (nextC > '9') )
		{
			// So it wasn't. Maybe it's a command, so we go there
			push_back_char( nextC );
			goto command;
		} 
		else
		{
			// Now it's sure to be a register
			nextNextC = get_next_char();
			if( (nextNextC < '0') || (nextNextC > '9') )
			{
				// One digit register
				push_back_char(nextNextC);
				yylval = nextC - '0';
				return tREGISTER;
			}
			else
			{
				// Two digit register
				yylval = (nextC - '0')*10 + nextNextC - '0';
				return tREGISTER;
			}
		}
	}
	
	// * Recognize segment registers (sr0 through sr15)
	else if( c == 's' )
	{
		// Could be numbered register, could not.
		nextC = get_next_char();
		if(nextC != 'r')
		{
			push_back_char(nextC);
			goto command;
		}
		
		nextC = get_next_char();
		if( (nextC < '0') || (nextC > '9') )
		{
			// So it wasn't. Maybe it's a command, so we go there
			push_back_char( nextC );
			goto command;
		} 
		else
		{
			// Now it's sure to be a register
			nextNextC = get_next_char();
			if( (nextNextC < '0') || (nextNextC > '9') )
			{
				// One digit register
				push_back_char(nextNextC);
				yylval = nextC - '0';
				return tSEGREGISTER;
			}
			else
			{
				// Two digit register
				yylval = (nextC - '0')*10 + nextNextC - '0';
				return tSEGREGISTER;
			}
		}
	}
	
	// * Recognize octal constants (beginning with ')
	else if( c == '\'' )
	{
		// Could be an octal, could not
		nextC = get_next_char();
		if( (nextC < '0') || (nextC > '7') )
		{
			// So it wasn't
			push_back_char( nextC );
			return c;
		}
		else
		{
			// Now it's sure to be an octal
			ASCII8			octalStr[15];
			ASCII8Str			strPtr = octalStr;
			*strPtr++ = nextC;
			
			while( ((nextC = get_next_char()) >= '0') && (nextC <= '7') )
				*strPtr++ = nextC;
			
			push_back_char( nextC );		// Push back the char that ended the octal
			*strPtr = 0;
			
			yylval = oct_atoi( octalStr );
			return tCONSTANT;
		}
	}
	
	// * Recognize hex constants (as in $DEADBEEF or 0xDEADBEEF)
	else if( c == '$' )
	{
		// Could be hex, could not
		nextC = get_next_char();
		push_back_char( nextC );			// Get a peek of what's ahead
		if( !isHex( nextC ) )
			// So it wasn't
			return c;
		else
			// Parse the hex and return
			return parseHex();
	}
	if( c == '0' )
	{
		// This might a hex (0xABCD) or a decimal
		nextC = get_next_char();
		if( nextC == 'x' )
			// This was a hex
			return parseHex();
		else
		{
			// This happened to be a decimal
			push_back_char( nextC );
			goto decimal;
		}
	}
	
	// * Recognize decimal constants
	else if( (c >= '0') && (c <= '9') )
	{
		// This is a decimal
		ASCII8			decStr[15];
		ASCII8Str			strPtr = decStr;
decimal:
		*strPtr++ = c;
		
		while( ((nextC = get_next_char()) >= '0') && (nextC <= '9') )
			*strPtr++ = nextC;
		
		push_back_char( nextC );		// Push back the char that ended the decimal
		*strPtr = 0;
		
		yylval = atoi( decStr );
		return tCONSTANT;
	}
	
	// * Recognize << and >> as single tokens (damned Bison can't do this)
	else if( (c == '<') )
	{
		nextC = get_next_char();
		if( nextC == '<' )
			return tSHIFTLEFT;
		else
		{
			push_back_char( nextC );
			return c;
		}
	}
	else if( (c == '>') )
	{
		nextC = get_next_char();
		if( nextC == '>' )
			return tSHIFTRIGHT;
		else
		{
			push_back_char( nextC );
			return c;
		}
	}
	
	// * Recognize commands in the symbol table
	else if( (c >= 'a') && (c <= 'z') )
	{
		ASCII8			str[100];
		ASCII8Str			strPtr;
		Int32			i;

command:	
		strPtr = str;
		
		*strPtr++ = c;
		while( ((nextC = get_next_char()) >= 'a') && (nextC <= 'z') )
			*strPtr++ = nextC;
		
		push_back_char( nextC );		// Push back the char that ended the command
		*strPtr = 0;
		
		for( i = 0; strcmp( commands[i].tokName, "" ); i++ )
		{
			if( strcmp( commands[i].tokName, str ) == 0 )
				break;
		}
		
		if( commands[i].tokNum != 0 )
			return commands[i].tokNum;
		else
		{
			// Yuck! Backtrace the entire string but the first character!
			for( i = strlen( str )-1; i > 0; i-- )
				push_back_char( str[i] );
			
			return str[0];
		}
	}
	
	// * Return everything else character by character
	return c;
}

Int32 parseHex( void )
{
	ASCII8		c;
	ASCII8		hexStr[15];
	ASCII8Str		strPtr = hexStr;
	
	c = get_next_char();
	while( isHex(c) )
	{
		*strPtr++ = c;
		c = get_next_char();
	}
	
	push_back_char( c );		// Push back the char that ended the hex
	*strPtr = 0;
	
	yylval = hex_atoi( hexStr );
	return tCONSTANT;
}
