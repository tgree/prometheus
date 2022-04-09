/*
	UnicodeUtils.h
	Copyright © 1999 by Patrick Varilly

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
	Patrick Varilly		-	Tuesday, 30 March 99	-	Creation of file
*/

#ifndef __UNICODE_UTILS__
#define __UNICODE_UTILS__

#include "Kernel Types.h"

UniChar UnicodeLower( UniChar c );
void UnicodeToASCII( UniStr unicode, UInt32 length, ASCII8Str ascii );
void ASCIIToUnicode( ConstASCII8Str ascii, UInt32 length, UniStr unicode );

#endif /*__UNICODE_UTILS__*/