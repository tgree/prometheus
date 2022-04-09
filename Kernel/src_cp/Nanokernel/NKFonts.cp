/*
	NKFonts.cp
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
#include "NKFonts.h"
#include "Gonzales.h"
#include "ADBMouse.h"

void NKInitFonts(void)
{
	InitGonzales();
}

NKFont::NKFont(UInt32 width,UInt32 height)
{
	_width = width;
	_height = height;
}

NKFont::~NKFont()
{
}

CompiledFont::CompiledFont(UInt32 _width,UInt32 _height,const CharBlitterProcPtr* _blitTable8,const CharBlitterProcPtr* _blitTable16,const CharBlitterProcPtr* _blitTable24):
	NKFont(_width,_height)
{
	blitTable8 = _blitTable8;
	blitTable16 = _blitTable16;
	blitTable24 = _blitTable24;
}

void CompiledFont::drawChar(ASCII7 c,Int8* address,Int32 rowBytes,UInt32 foreColor,UInt32 backColor,UInt32 depth,Boolean eraseBackground)
{
	MouseShield	shield;
	switch(depth)
	{
		case 8:
			if(blitTable8)
			{
				if(eraseBackground)
					(*blitTable8[0])(address,rowBytes,backColor);
				(*blitTable8[c & 0x7F])(address,rowBytes,foreColor);
			}
		break;
		case 16:
			if(blitTable16)
			{
				if(eraseBackground)
					(*blitTable16[0])(address,rowBytes,backColor);
				(*blitTable16[c & 0x7F])(address,rowBytes,foreColor);
			}
		break;
		case 24:
			if(blitTable24)
			{
				if(eraseBackground)
					(*blitTable24[0])(address,rowBytes,backColor);
				(*blitTable24[c & 0x7F])(address,rowBytes,foreColor);
			}
		break;
	}
}
