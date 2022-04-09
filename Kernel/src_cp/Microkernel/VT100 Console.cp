/*
	VT100 Console.cp
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
#include "VT100 Console.h"
#include "Streams.h"
#include "ANSI.h"

static const ASCII8	plainMode[] =					{0x1B,'[','0','m'};
static const ASCII8	boldMode[] =					{0x1B,'[','1','m'};
static const ASCII8	underlineMode[] =				{0x1B,'[','4','m'};
static const ASCII8	inverseMode[] =				{0x1B,'[','7','m'};
static const ASCII8	invisibleMode[] =				{0x1B,'[','8','m'};
static const ASCII8	clearToEndOfLineCode[] =			{0x1B,'[','0','K'};
static const ASCII8	clearFromStartOfLineCode[] =		{0x1B,'[','1','K'};
static const ASCII8	clearLineCode[] =				{0x1B,'[','2','K'};
static const ASCII8	clearToBottomOfScreenCode[] =	{0x1B,'[','0','J'};
static const ASCII8	clearFromTopOfScreenCode[] =		{0x1B,'[','1','J'};
static const ASCII8	clearScreenCode[] =				{0x1B,'[','2','J'};
static const ASCII8	resetCode[] =					{0x1B,'c'};

VT100Console::VT100Console(IStream& istream,OStream& ostream):
	input(istream),
	output(ostream)
{
}

VT100Console::~VT100Console()
{
}

void VT100Console::write(ConstPtr data,UInt32 len)
{
	output.write(data,len);
}

void VT100Console::handle_message(const StreamMessage& message)
{
	if(message.type == lineMessage)
		output.write("\r\n",2);
	else
		Console::handle_message(message);
}

void VT100Console::read(Ptr data,UInt32 len)
{
	UInt32	numChars = 0;
	
	while(len--)
	{
		// Read a single character
		input.read(data,1);
		
		if(*data == 0x08)	// Delete
		{
			if(numChars)
			{
				data--;
				numChars--;
				len++;
				moveCursorRel(-1,0);
				output.write(" ",1);
				moveCursorRel(-1,0);
			}
		}
		else if(*data == KEYBOARD_RETURN)
		{
			*data = 0;
			break;
		}
		else
		{
			if(!(textMode & invisible))
				output.write(data,1);	// Echo it
			data++;
			numChars++;
		}
	}
}

ASCII8 VT100Console::readChar()
{
	ASCII8	theChar;
	input.read(&theChar,1);
	return theChar;
}

void VT100Console::moveCursorRel(Int32 dX,Int32 dY)
{
	ASCII8	theStr[15] = {0x1B,'['};
	
	if(dX > 0)	// Move right
	{
		unum2str(dX,theStr + 2);
		strcat(theStr,"C");
		output.write(theStr,strlen(theStr));
	}
	else if(dX < 0)	// Move left
	{
		unum2str(-dX,theStr + 2);
		strcat(theStr,"D");
		output.write(theStr,strlen(theStr));
	}
	if(dY > 0)	// Move down
	{
		unum2str(dY,theStr + 2);
		strcat(theStr,"B");
		output.write(theStr,strlen(theStr));
	}
	else if(dY < 0)	// Move up
	{
		unum2str(-dY,theStr + 2);
		strcat(theStr,"A");
		output.write(theStr,strlen(theStr));
	}
}

void VT100Console::moveCursorAbs(UInt32 x,UInt32 y)
{
	ASCII8	theStr[2] = {0x1B,'['};
	ASCII8	numStr[12];
	
	output.write(theStr,2);
	unum2str(y,numStr);
	output.write(numStr,strlen(numStr));
	output.write(";",1);
	unum2str(x,numStr);
	output.write(numStr,strlen(numStr));
	output.write("H",1);
}

void VT100Console::moveCursorToStartOfLine()
{
	output.write("\r",1);
}


void VT100Console::plainText()
{
	Console::plainText();
	output.write(plainMode,sizeof(plainMode));
}

void VT100Console::boldText()
{
	Console::boldText();
	output.write(boldMode,sizeof(boldMode));
}

void VT100Console::underlineText()
{
	Console::underlineText();
	output.write(underlineMode,sizeof(underlineMode));
}

void VT100Console::inverseText()
{
	Console::inverseText();
	output.write(inverseMode,sizeof(inverseMode));
}

void VT100Console::invisibleText()
{
	Console::invisibleText();
	output.write(invisibleMode,sizeof(invisibleMode));
}

void VT100Console::setTextMode(UInt32 newMode)
{
	output.write(plainMode,sizeof(plainMode));
	if(newMode & bold)
		output.write(boldMode,sizeof(boldMode));
	if(newMode & underline)
		output.write(underlineMode,sizeof(underlineMode));
	if(newMode & inverse)
		output.write(inverseMode,sizeof(inverseMode));
	if(newMode & invisible)
		output.write(invisibleMode,sizeof(invisibleMode));
	
	Console::setTextMode(newMode);
}

void VT100Console::clearToEndOfLine()
{
	output.write(clearToEndOfLineCode,sizeof(clearToEndOfLineCode));	
}

void VT100Console::clearFromStartOfLine()
{
	output.write(clearFromStartOfLineCode,sizeof(clearFromStartOfLineCode));
}

void VT100Console::clearLine()
{
	output.write(clearLineCode,sizeof(clearLineCode));
}

void VT100Console::clearToBottomOfScreen()
{
	output.write(clearToBottomOfScreenCode,sizeof(clearToBottomOfScreenCode));
}

void VT100Console::clearFromTopOfScreen()
{
	output.write(clearFromTopOfScreenCode,sizeof(clearFromTopOfScreenCode));
}

void VT100Console::clearScreen()
{
	output.write(clearScreenCode,sizeof(clearScreenCode));
}

void VT100Console::reset()
{
	output.write(resetCode,sizeof(resetCode));
}
