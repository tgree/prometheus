/*
	Kernel Console.h
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
#ifndef __KERNEL_CONSOLE__
#define __KERNEL_CONSOLE__

#include "Kernel Types.h"
#include "Streams.h"
#include "NKFonts.h"

enum
{
	// Stuff for text modes (these can be or'ed together)
	plain		=	0,
	bold		=	1,
	underline	=	2,
	inverse	=	4,
	invisible	=	8
};

class Console	:	public ASCIIIOStream
{
protected:
	UInt32		textMode;
	
	Console();
	virtual ~Console();
public:
	// For these functions, the width of a character is defined as the width of 'W'
	virtual	void	moveCursorRel(Int32 dX,Int32 dY)	=	0;	// Moves the cursor DOWN dY and RIGHT dX characters
	virtual	void	moveCursorAbs(UInt32 x,UInt32 y)	=	0;	// Moves the cursor to absolute character position (x,y)
	virtual	void	moveCursorToStartOfLine()		=	0;	// Moves the cursor to the start of the line
	
	// Text style modes - not necessary to overload if you check "textMode" every time you write
	virtual	void		plainText();		// No style
	virtual	void		boldText()	;		// Bold
	virtual	void		underlineText();	// Underline
	virtual	void		inverseText();		// Inverted text
	virtual	void		invisibleText();		// Invisible, useful for inputting passwords
	virtual	UInt32	getTextMode();		// Returns the textMode
	virtual	void		setTextMode(UInt32 newMode);	// Sets the textMode
	
	// Clearing modes
	virtual	void	clearToEndOfLine()		=	0;	// Clears to the end of the line from the cursor
	virtual	void	clearFromStartOfLine()	=	0;	// Clears from the start of the line to the cursor
	virtual	void	clearLine()			=	0;	// Clears the entire line
	virtual	void	clearToBottomOfScreen()	=	0;	// Clears to the bottom of the screen from the cursor
	virtual	void	clearFromTopOfScreen()	=	0;	// Clears from the top of the screen to the cursor
	virtual	void	clearScreen()			=	0;	// Clears the entire screen
	
	// Special stuff
	virtual	void		reset()		=	0;	// Resets the console to the initial state
	virtual	ASCII8	readChar()	=	0;	// Reads a single character from the console, without drawing a cursor.  Waits for the character.
};

class VideoConsoleStream	:	public Console
{
	Rect			bounds;
	UInt32		cursX;
	UInt32		cursY;
	Boolean		scrolls;
	NKFont*		font;
	UInt32		_foreColor;
	UInt32		_backColor;
	
			void	newLine(void);
protected:
	
	// Stuff for OStream
	virtual	void	write(ConstPtr data,UInt32 len);
	virtual	void	handle_message(const StreamMessage& message);
	
	// Stuff for IStream
	virtual	void	read(Ptr data,UInt32 len);
public:
	VideoConsoleStream(const Rect* _bounds,Boolean _scrolls,NKFont* _font);	// This should be made private again after USB is fully debugged
	virtual ~VideoConsoleStream();
	
			void		fillRect(UInt32 value,const Rect* rect);
			void		fill(UInt32 value);
			void		invertRect(const Rect* rect);

			void		moveTo(UInt32 x,UInt32 y);
			void		getPos(UInt32* x,UInt32* y);
			
			UInt32	charWidth()	{return font->width();}
			UInt32	charHeight()	{return font->height();}
	
			void		foreColor(UInt32 color)	{_foreColor = color;}
			void		backColor(UInt32 color)	{_backColor = color;}
			void		setPixel8(UInt32 x,UInt32 y,UInt8 color);
			UInt8	getPixel8(UInt32 x,UInt32 y);
			
	// Stuff for Console
	virtual	void		moveCursorRel(Int32 dX,Int32 dY);
	virtual	void		moveCursorAbs(UInt32 x,UInt32 y);
	virtual	void		moveCursorToStartOfLine();
	virtual	void		clearToEndOfLine();
	virtual	void		clearFromStartOfLine();
	virtual	void		clearLine();
	virtual	void		clearToBottomOfScreen();
	virtual	void		clearFromTopOfScreen();
	virtual	void		clearScreen();
	virtual	void		reset();
	virtual	ASCII8	readChar();
	
	friend void InitVideoConsole(const Rect* _bounds,Boolean _scrolls,NKFont* _font);
};

extern VideoConsoleStream* videoConsole;

#endif /* __KERNEL_CONSOLE__ */