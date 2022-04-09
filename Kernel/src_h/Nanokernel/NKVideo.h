/*
	NKVideo.h
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
#ifndef __NK_VIDEO__
#define __NK_VIDEO__

#include "Kernel Types.h"
#include "NKFonts.h"
#include "NKStream.h"

struct NKVideo	:	public NKStream
{
	UInt32		cursX;
	UInt32		cursY;
	Rect			bounds;
	NKFont*		font;
	Boolean		scrolls;
	UInt32		foreColor;
	UInt32		backColor;
};

extern NKVideo	nkVideo;

void	NKInitVideo(NKVideo* video,const Rect* bounds,Boolean scrolls,NKFont* font);
void	NKRemapVideo(void);	// Called by NKInitVirtualMemory to remap the nanokernel video driver when page tables have been enabled
void	NKBlackOutRect(Rect* _rect, UInt32 color = 0xFFFFFFFF); // Blacks out a given rectangle on the screen
void	NKInvertRect(Rect* _rect);	// Inverts a given rectangle on the screen
void	NKScrollVideoVertical(Rect* _rect,Int32 dy);	// Scrolls video bytes DOWN by dy pixels

#endif /* __NK_VIDEO__ */