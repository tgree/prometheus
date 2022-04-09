/*
	NKFonts.h
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
#ifndef __NK_FONTS__
#define __NK_FONTS__

#include "Kernel Types.h"

class NKFont
{
	UInt32				_width;	// Assume fixed width font for now
	UInt32				_height;
protected:
	NKFont(UInt32 width,UInt32 height);
public:
	virtual ~NKFont();
	
	virtual	void	drawChar(ASCII7 c,Int8* address,Int32 rowBytes,UInt32 foreColor,UInt32 backColor,UInt32 depth,Boolean eraseBackground = true) = 0;
			UInt32	width()	{return _width;}
			UInt32	height()	{return _height;}
};

typedef void (*CharBlitterProcPtr)(register Int8* addr,register Int32 rowBytes,register UInt32 color);

class CompiledFont	:	public NKFont
{
	const CharBlitterProcPtr*	blitTable8;
	const CharBlitterProcPtr*	blitTable16;
	const CharBlitterProcPtr*	blitTable24;
public:
	CompiledFont(UInt32 width,UInt32 height,const CharBlitterProcPtr* blitTable8,const CharBlitterProcPtr* blitTable16 = nil,const CharBlitterProcPtr* blitTable24 = nil);
	
	virtual	void	drawChar(ASCII7 c,Int8* address,Int32 rowBytes,UInt32 foreColor,UInt32 backColor,UInt32 depth,Boolean eraseBackground);
};

void NKInitFonts(void);

#endif /* __NK_FONTS__ */