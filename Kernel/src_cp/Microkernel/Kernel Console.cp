/*
	Kernel Console.cp
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
#include "NKVideo.h"
#include "Kernel Console.h"
#include "Video Driver.h"
#include "Macros.h"
#include "ADBMouse.h"

VideoConsoleStream*	videoConsole = nil;

void InitVideoConsole(const Rect* _bounds,Boolean _scrolls,NKFont* _font)
{
	// Must be called after InitVideo()
	videoConsole = new VideoConsoleStream(_bounds,_scrolls,_font);
}

Console::Console()
{
	textMode = plain;
}

Console::~Console()
{
}

void Console::plainText()
{
	textMode = plain;
}

void Console::boldText()
{
	textMode |= bold;
}

void Console::underlineText()
{
	textMode |= underline;
}

void Console::inverseText()
{
	textMode |= inverse;
}

void Console::invisibleText()
{
	textMode |= invisible;
}

UInt32 Console::getTextMode()
{
	return textMode;
}

void Console::setTextMode(UInt32 newMode)
{
	textMode |= newMode;
}

VideoConsoleStream::VideoConsoleStream(const Rect* _bounds,Boolean _scrolls,NKFont* _font):
	Stream("video console")
{
	font = _font;
	
	MouseShield	shield;
	bounds = *_bounds;
	NKBlackOutRect(&bounds,white8bit);
	bounds.x1++;
	bounds.x2--;
	bounds.y1++;
	bounds.y2--;
	//fill(0xFFFFFFFF);
	NKBlackOutRect(&bounds);
	bounds.x1++;
	bounds.x2--;
	bounds.y1++;
	bounds.y2--;
	cursX = bounds.x1;
	cursY = bounds.y1;
	scrolls = _scrolls;
	
	_foreColor = white8bit;
	_backColor = black8bit;
}

VideoConsoleStream::~VideoConsoleStream()
{
}

void VideoConsoleStream::handle_message(const StreamMessage& message)
{
	switch(message.type)
	{
		case lineMessage:
			newLine();
		break;
		case colorMessage:
			foreColor(message.data);
		break;
		default:
			ASCIIOStream::handle_message(message);
		break;
	}
}

void VideoConsoleStream::fillRect(UInt32 value,const Rect* _rect)
{
	Rect			rect = {_rect->x1 + bounds.x1,_rect->y1 + bounds.y1,_rect->x2 + bounds.x1,_rect->y2 + bounds.y1};
	UInt32		pixSize = video->pixSize();
	UInt32		rowBytes = video->rowBytes();
	Int8*		baseAddr = video->logicalAddr();
	
	UInt32		mask = ((1 << pixSize) - 1);
	UInt32		leftByte = (rect.x1 < bounds.x1) ? bounds.x1*pixSize/8 : ((rect.x1 > bounds.x2) ? bounds.x2*pixSize/8 : rect.x1*pixSize/8);
	UInt32		rightByte = (rect.x2 < bounds.x1) ? bounds.x1*pixSize/8 : ((rect.x2 > bounds.x2) ? bounds.x2*pixSize/8 : rect.x2*pixSize/8);
	UInt32		topRow = (rect.y1 < bounds.y1) ? bounds.y1 : ((rect.y1 > bounds.y2) ? bounds.y2 : rect.y1);
	UInt32		bottomRow = (rect.y2 < bounds.y1) ? bounds.y1 : ((rect.y2 > bounds.y2) ? bounds.y2 : rect.y2);
	
	if(leftByte > rightByte)
		return;
	if(topRow > bottomRow)
		return;
	
	MouseShield	shield;
	for(Int32 y = topRow;y < bottomRow;y++)
	{
		switch(pixSize)
		{
			case 8:
				for(Int32 x = leftByte;x < rightByte;x++)
					*(UInt8*)(baseAddr + x + y*rowBytes) = (value & mask);
			break;
			case 16:
				for(Int32 x = leftByte;x < rightByte;x += 2)
					*(UInt16*)(baseAddr + x + y*rowBytes) = (value & mask);
			break;
			case 32:
				for(Int32 x = leftByte;x < rightByte;x += 4)
					*(UInt32*)(baseAddr + x + y*rowBytes) = (value & mask);
			break;
		}
	}
}

void VideoConsoleStream::fill(UInt32 value)
{
	Rect		rect = {0,0,bounds.x2 - bounds.x1,bounds.y2 - bounds.y1};
	fillRect(value,&rect);
}

void VideoConsoleStream::invertRect(const Rect* _rect)
{
	Rect			rect = {_rect->x1 + bounds.x1,_rect->y1 + bounds.y1,_rect->x2 + bounds.x1,_rect->y2 + bounds.y1};
	UInt32		pixSize = video->pixSize();
	UInt32		rowBytes = video->rowBytes();
	Int8*		baseAddr = video->logicalAddr();
	
	UInt32		leftByte = (rect.x1 < bounds.x1) ? bounds.x1*pixSize/8 : ((rect.x1 > bounds.x2) ? bounds.x2*pixSize/8 : rect.x1*pixSize/8);
	UInt32		rightByte = (rect.x2 < bounds.x1) ? bounds.x1*pixSize/8 : ((rect.x2 > bounds.x2) ? bounds.x2*pixSize/8 : rect.x2*pixSize/8);
	UInt32		topRow = (rect.y1 < bounds.y1) ? bounds.y1 : ((rect.y1 > bounds.y2) ? bounds.y2 : rect.y1);
	UInt32		bottomRow = (rect.y2 < bounds.y1) ? bounds.y1 : ((rect.y2 > bounds.y2) ? bounds.y2 : rect.y2);
	
	if(leftByte > rightByte)
		return;
	if(topRow > bottomRow)
		return;
	
	MouseShield	shield;
	for(Int32 y = topRow;y < bottomRow;y++)
	{
		switch(pixSize)
		{
			case 8:
				for(Int32 x = leftByte;x < rightByte;x++)
					*(UInt8*)(baseAddr + x + y*rowBytes) = ~*(UInt8*)(baseAddr + x + y*rowBytes);
			break;
			case 16:
				for(Int32 x = leftByte;x < rightByte;x += 2)
					*(UInt16*)(baseAddr + x + y*rowBytes) = ~*(UInt16*)(baseAddr + x + y*rowBytes);
			break;
			case 32:
				for(Int32 x = leftByte;x < rightByte;x += 4)
					*(UInt32*)(baseAddr + x + y*rowBytes) = ~*(UInt32*)(baseAddr + x + y*rowBytes);
			break;
		}
	}
}

void VideoConsoleStream::moveTo(UInt32 x,UInt32 y)
{
	cursX = x + bounds.x1;
	cursY = y + bounds.y1;
}

void VideoConsoleStream::write(const Int8* data,UInt32 len)
{
	const ASCII8*		c = (const ASCII8*)data;
	UInt32			rowBytes = video->rowBytes();
	Int8*			baseAddr = video->logicalAddr();
	
	for(Int32 i=0;i<len;i++)
	{
		if(*c == '\n' || *c == '\r')
			newLine();
		else if(*c == '\t')
			write("    ",ROUND_UP(4,((cursX - bounds.x1)/font->width()) + 1) - (cursX - bounds.x1)/font->width());
		else
		{
			if(!(textMode & inverse))
			{
				Rect	theRect = {cursX - bounds.x1 - 1,cursY - bounds.y1 - 1,cursX + charWidth() - bounds.x1 - 1,cursY + charHeight() - bounds.y1 - 1};
				fillRect(black8bit,&theRect);
			}
			font->drawChar(*c,baseAddr + cursX + cursY*rowBytes,rowBytes,_foreColor,_backColor,8);
			if(textMode & underline)
			{
				Rect	theRect = {cursX - bounds.x1,cursY + charHeight() - 1 - bounds.y1,cursX + charWidth() - bounds.x1,cursY + charHeight() - bounds.y1};
				fillRect(0,&theRect);
			}
			if(textMode & inverse)
			{
				Rect	theRect = {cursX - bounds.x1 - 1,cursY - bounds.y1 - 1,cursX + charWidth() - bounds.x1 - 1,cursY + charHeight() - bounds.y1 - 1};
				invertRect(&theRect);
			}
			if(textMode & bold)
				font->drawChar(*c,baseAddr + cursX + 1 + cursY*rowBytes,rowBytes,_foreColor,_backColor,8,false);
			cursX += font->width();
			if(cursX + font->width() > bounds.x2)
				newLine();
		}
		c++;
	}
}

void VideoConsoleStream::newLine(void)
{
	cursX = bounds.x1;
	cursY += font->height();
	if(cursY + font->height() > bounds.y2)
	{
		if(scrolls)
		{
			cursY -= font->height();
			NKScrollVideoVertical(&bounds,-font->height());
			Rect	blackRect = {bounds.x1,cursY,bounds.x2,bounds.y2};
			NKBlackOutRect(&blackRect);
		}
		else
		{
			cursY = bounds.y1;
			NKBlackOutRect(&bounds);
		}
	}
}

void VideoConsoleStream::read(Ptr data,UInt32 remainLen)
{
	ASCII8	cursor = 1;
	ASCII8	theChar;
	UInt32	x;
	UInt32	y;
	UInt32	savedTextMode;
	UInt32	len = 0;
	
	while(remainLen--)
	{
		// First we need to draw the cursor
		getPos(&x,&y);
		savedTextMode = textMode;
		textMode = 0;
		write(&cursor,1);
		textMode = savedTextMode;
		cin >> theChar;
		if(theChar == 0x08)	// Delete
		{
			if(len)
			{
				data--;
				remainLen++;
				if(!(textMode & invisible))
				{
					moveTo(x,y);
					textMode = 0;
					write(" ",1);
					textMode = savedTextMode;
					moveTo(x - charWidth(),y);
				}
				else
					moveTo(x,y);
			}
			else
				moveTo(x,y);
		}
		else if(theChar == KEYBOARD_RETURN)
		{
			moveTo(x,y);
			textMode = 0;
			write(" ",1);
			textMode = savedTextMode;
			*data = 0;
			break;
		}
		else
		{
			if(!(textMode & invisible))
			{
				moveTo(x,y);
				write(&theChar,1);
			}
			else
				moveTo(x,y);
			*data++ = theChar;
			len++;
		}
	}
}

void VideoConsoleStream::getPos(UInt32* x,UInt32* y)
{
	*x = cursX - bounds.x1;
	*y = cursY - bounds.y1;
}

void VideoConsoleStream::setPixel8(UInt32 x,UInt32 y,UInt8 color)
{
	video->setPixel8(x+bounds.x1,y+bounds.y1,color);
}

UInt8 VideoConsoleStream::getPixel8(UInt32 x,UInt32 y)
{
	return video->getPixel8(x+bounds.x1,y+bounds.y1);
}

void VideoConsoleStream::moveCursorRel(Int32 dX,Int32 dY)
{
	cursX += dX*charWidth();
	cursY += dY*charHeight();
	if(cursX < bounds.x1)
		cursX = bounds.x1;
	if(cursX > bounds.x2 - charWidth())
		cursX = ROUND_DOWN(charWidth(),bounds.x2 - charWidth());
	if(cursY < bounds.y1)
		cursY = bounds.y1;
	if(cursY > bounds.y2 - charHeight())
		cursY = ROUND_DOWN(charHeight(),bounds.y2 - charHeight());
}

void VideoConsoleStream::moveCursorAbs(UInt32 x,UInt32 y)
{
	cursX = bounds.x1 + x*charWidth();
	cursY = bounds.y1 + y*charHeight();
	if(cursX > bounds.x2 - charWidth())
		cursX = ROUND_DOWN(charWidth(),bounds.x2 - charWidth());
	if(cursY > bounds.y2 - charHeight())
		cursY = ROUND_DOWN(charHeight(),bounds.y2 - charHeight());
}

void VideoConsoleStream::moveCursorToStartOfLine()
{
	cursX = bounds.x1;
}

void VideoConsoleStream::clearToEndOfLine()
{
	Rect	theRect = {cursX - 1,cursY - 1,bounds.x2,cursY + charHeight() - 1};
	NKBlackOutRect(&theRect);
}

void VideoConsoleStream::clearFromStartOfLine()
{
	Rect	theRect = {bounds.x1,cursY - 1,cursX - 1,cursY + charHeight() - 1};
	NKBlackOutRect(&theRect);
}

void VideoConsoleStream::clearLine()
{
	Rect	theRect = {bounds.x1,cursY - 1,bounds.x2,cursY + charHeight() - 1};
	NKBlackOutRect(&theRect);
}

void VideoConsoleStream::clearToBottomOfScreen()
{
	Rect	theRect = {bounds.x1,cursY - 1,bounds.x2,bounds.y2};
	NKBlackOutRect(&theRect);
}

void VideoConsoleStream::clearFromTopOfScreen()
{
	Rect	theRect = {bounds.x1,bounds.y1,bounds.x2,cursY - 1};
	NKBlackOutRect(&theRect);
}

void VideoConsoleStream::clearScreen()
{
	NKBlackOutRect(&bounds);
}

void VideoConsoleStream::reset()
{
	clearScreen();
	moveCursorAbs(0,0);
}

ASCII8 VideoConsoleStream::readChar()
{
	ASCII8	theChar;
	cin >> theChar;
	return theChar;
}
