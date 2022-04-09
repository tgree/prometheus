/*
	Video Driver.h
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
#ifndef __VIDEO_DRIVER__
#define __VIDEO_DRIVER__

#include "NKDebuggerNub.h"
#include "Driver.h"

typedef struct VideoParams
{
	Int8*		logicalAddr;
	Int8*		physicalAddr;
	UInt32		rowBytes;
	UInt32		height;
	UInt32		width;
	UInt32		pixSize;
}VideoParams;

class VideoDriver	:	public Driver
{
protected:
	VideoParams	videoParams;
	
	virtual ~VideoDriver() {}
	VideoDriver(ConstASCII8Str name):Driver(name)	{}
	
	virtual	void			initialize()	{}
	virtual	void			start()	{}
	virtual	void			stop()	{}
public:
	virtual	Int8*		logicalAddr()	{return videoParams.logicalAddr;}
	virtual	Int8*		physicalAddr()	{return videoParams.physicalAddr;}
	virtual	UInt32		rowBytes()	{return videoParams.rowBytes;}
	virtual	UInt32		height()		{return videoParams.height;}
	virtual	UInt32		width()		{return videoParams.width;}
	virtual	UInt32		pixSize()		{return videoParams.pixSize;}
	
	virtual	void			setPixel8(UInt32 x,UInt32 y,UInt8 color)
	{
		FatalAssert((x >= 0) && (x < width()));
		FatalAssert((y >= 0) && (y < height()));
		*((UInt8*)(logicalAddr() + y*rowBytes() + x)) = color;
	}
	
	virtual	UInt8		getPixel8(UInt32 x,UInt32 y)
	{
		FatalAssert((x >= 0) && (x < width()));
		FatalAssert((y >= 0) && (y < height()));
		return *((UInt8*)(logicalAddr() + y*rowBytes() + x));
	}
};

enum
{
	green8bit	=	0xBF,
	red8bit	=	0xD8,
	white8bit	=	0x00,
	black8bit	=	0xFF
};

extern VideoDriver*	video;

void InitVideo(void);


#endif /*__VIDEO_DRIVER__*/