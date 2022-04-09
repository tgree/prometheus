/*
	VT100 Console.h
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
	Terry Greeniaus	-	Saturday, 19 Dec. 98	-	Original creation of file
*/
#ifndef __VT100_CONSOLE__
#define __VT100_CONSOLE__

#include "Kernel Console.h"

class VT100Console	:	public Console
{
	IStream&		input;
	OStream&		output;
public:
	VT100Console(IStream& input,OStream& output);
	virtual ~VT100Console();
	
	// Stuff for OStream
	virtual	void	write(ConstPtr data,UInt32 len);
	virtual	void	handle_message(const StreamMessage& message);
	
	// Stuff for IStream
	virtual	void	read(Ptr data,UInt32 len);
	
	// Stuff for Console
	virtual	void	moveCursorRel(Int32 dX,Int32 dY);
	virtual	void	moveCursorAbs(UInt32 x,UInt32 y);
	virtual	void	moveCursorToStartOfLine();
	
	virtual	void	plainText();
	virtual	void	boldText();
	virtual	void	underlineText();
	virtual	void	inverseText();
	virtual	void	invisibleText();
	virtual	void	setTextMode(UInt32 newMode);
	
	virtual	void	clearToEndOfLine();
	virtual	void	clearFromStartOfLine();
	virtual	void	clearLine();
	virtual	void	clearToBottomOfScreen();
	virtual	void	clearFromTopOfScreen();
	virtual	void	clearScreen();
	
	virtual	void		reset();
	virtual	ASCII8	readChar();
};

#endif /* __VT100_CONSOLE__ */