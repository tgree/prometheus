/*
	NKDisassembly.h
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
	Patrick Varilly		-	Mon, 19 Jan 98		-	Original creation of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
/*
	NKDisassembly.h
	
	A fast and safe disassembler for PowerPC code. (Interrupt safe)
	
	Copyright © 1998 by Patrick Varilly. All rights reserved worldwide.
	Permission to use and modify this file is given solely to the Pandora Team until further notice
*/
#ifndef __NK__DISASSEMBLY__
#define __NK__DISASSEMBLY__

/*
	Disassemble: A routine to disassemble a PowerPC opcode
	Parameters:
		opcode		a 32-bit PowerPC opcode.
		string		a pointer to a string at least 51-characters wide (counting the terminating null) on which the
					disassembled output will be place.
	Returns:
		A pointer to the output string.
*/
ASCII8Str Disassemble( UInt32 opcode, ASCII8Str string );

#endif /* !__NK__DISASSEMBLY__ */