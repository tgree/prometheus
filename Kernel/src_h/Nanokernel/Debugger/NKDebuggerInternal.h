/*
	NKDebuggerInternal.h
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
#ifdef __cplusplus
extern "C" {
#endif

// Special register aliases
#define REG_PC	32
#define REG_LR		33
#define REG_SP	1
#define REG_RTOC	2

UInt32 getRegValue( UInt32 regNum );
void setRegValue( UInt32 regNum, UInt32 newRegValue );
void restart( void );
void goFrom( UInt32 start );
void print( ASCII8Str str );
void printNum( UInt32 num );
void disassembleAt( UInt32 addr, Boolean centered );
void printHelp( void );
void stackTrace( void );
void step( Boolean over );
void displayMemory( UInt32 addr );

#ifdef __cplusplus
}
#endif