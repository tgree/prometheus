/*
	Chip Debugger.cp
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
	Version History
	============
	Terry Greeniaus	-	Tuesday, 3 November 1998	-	Original creation of file
*/
#include "Chip Debugger.h"
#include "Kernel Console.h"
#include "NKVideo.h"
#include "NKVirtualMemory.h"
#include "ANSI.h"
#include "Video Driver.h"

static void	DisplayChipMenu();
static void	HandleChipMenu();
static void	SelectChip(Chip* theChip);
static UInt32	GetChipNumber(Chip* theChip);
static void	EditChip(Chip* theChip);
static UInt32	DisplayRegs(Chip* theChip);	// Returns the width of the longest register name
static UInt32	ReadReg(Chip* theChip,RegisterDescriptor* theReg);
static void	WriteReg(Chip* theChip,RegisterDescriptor* theReg,UInt32 regContents);
static void	SelectReg(RegisterDescriptor* theReg,UInt32 regsX,UInt32 regNumber);
static void	EditReg(Chip* theChip,RegisterDescriptor* theReg,UInt32 regsX,UInt32 regNumber);

static Chip*	chipList = nil;

Chip::Chip(ConstASCII8Str _name,RegisterDescriptor* _regs,void* _addr)
{
	nkVideo << "New chip: \"" << _name << "\"\n";
	name = _name;
	regs = _regs;
	addr = _addr;
	
	next = chipList;
	prev = nil;
	if(next)
		next->prev = this;
	
	chipList = this;
}

Chip::~Chip()
{
	// Not implemented yet
}

void DebugChips()
{
	cout << hexMsg;
	HandleChipMenu();
	cout << decMsg;
}

static void HandleChipMenu()
{
	Chip*	selectedChip = chipList;
	
	if(!selectedChip)
	{
		cout << redMsg << "No chips present!\n" << whiteMsg;
		return;
	}
	
	Boolean	redisplayMenu;
	
	for(;;)
	{
		DisplayChipMenu();
		SelectChip(selectedChip);
		
		redisplayMenu = false;
		while(!redisplayMenu)
		{
			ASCII8	keyPress;
			cin >> keyPress;
			
			switch(keyPress)
			{
				case KEYBOARD_RETURN:
				break;
				case 0x1E:	// Up arrow
					if(selectedChip->prev)
					{
						SelectChip(selectedChip);
						selectedChip = selectedChip->prev;
						SelectChip(selectedChip);
					}
				break;
				case 0x1F:	// Down arrow
					if(selectedChip->next)
					{
						SelectChip(selectedChip);
						selectedChip = selectedChip->next;
						SelectChip(selectedChip);
					}
				break;
				case 0x1D:	// Right arrow
					EditChip(selectedChip);
					redisplayMenu = true;
				break;
				case 0x1C:	// Left arrow
				break;
				case 0x1B:	// Escape
					return;
				break;
			}
		}
	}
}

static void DisplayChipMenu()
{
	videoConsole->fill(black8bit);
	videoConsole->moveTo(0,0);
	cout << greenMsg << "\t\tChip Debugger\n\n" << whiteMsg;
	
	Chip*	theChip = chipList;
	
	while(theChip)
	{
		cout << (UInt32)NKGetPhysical(theChip->addr,PROCESS_CURRENT) << ": " << theChip->name << "\n";
		theChip = theChip->next;
	}
}

static void SelectChip(Chip* theChip)
{
	UInt32	y = (2 + GetChipNumber(theChip))*videoConsole->charHeight() - 1;
	UInt32	x = 12*videoConsole->charWidth();
	UInt32	width = strlen(theChip->name)*videoConsole->charWidth();
	Rect		theRect = {x,y,x+width,y+videoConsole->charHeight()};
	
	videoConsole->invertRect(&theRect);
}

static UInt32 GetChipNumber(Chip* theChip)
{
	UInt32	n = 0;
	
	while(theChip->prev)
	{
		theChip = theChip->prev;
		n++;
	}
	
	return n;
}

static void EditChip(Chip* theChip)
{
	RegisterDescriptor*	theReg = theChip->regs;
	
	videoConsole->fill(black8bit);
	videoConsole->moveTo(0,0);
	cout << greenMsg << "\t\tEditing chip: \"" << whiteMsg << theChip->name << greenMsg << "\"\n\n" << whiteMsg;
	
	Boolean	redisplayChip;
	UInt32	regNumber = 0;
	for(;;)
	{
		UInt32	regsX = DisplayRegs(theChip) + 2;
		SelectReg(theReg,regsX,regNumber);
		
		redisplayChip = false;
		while(!redisplayChip)
		{
			ASCII8	keyPress;
			cin >> keyPress;
			
			switch(keyPress)
			{
				case KEYBOARD_RETURN:
					SelectReg(theReg,regsX,regNumber);
					EditReg(theChip,theReg,regsX,regNumber);
					SelectReg(theReg,regsX,regNumber);
				break;
				case ' ':
					if(!(theReg->flags & REG_WRITE_ONLY))
					{
						SelectReg(theReg,regsX,regNumber);
						UInt32 regContents = ReadReg(theChip,theReg);
						videoConsole->moveTo((regsX)*videoConsole->charWidth(),(regNumber+2)*videoConsole->charHeight());
						cout << "            ";
						videoConsole->moveTo((regsX)*videoConsole->charWidth(),(regNumber+2)*videoConsole->charHeight());
						switch(theReg->size)
						{
							case 1:
								cout << (UInt8)regContents;
							break;
							case 2:
								cout << (UInt16)regContents;
							break;
							case 4:
								cout << regContents;
							break;
						}
						SelectReg(theReg,regsX,regNumber);
					}
				break;
				case 0x1B:	// Escape
					redisplayChip = true;
					SelectReg(theReg,regsX,regNumber);
				break;
				case 0x1C:	// Left arrow
					return;
				break;
				case 0x1E:	// Up arrow
					if(regNumber)
					{
						SelectReg(theReg,regsX,regNumber);
						regNumber--;
						theReg--;
						SelectReg(theReg,regsX,regNumber);
					}
				break;
				case 0x1F:	// Down arrow
					if(theReg[1].name)
					{
						SelectReg(theReg,regsX,regNumber);
						regNumber++;
						theReg++;
						SelectReg(theReg,regsX,regNumber);
					}
				break;
			}
		}
	}
}

static UInt32 DisplayRegs(Chip* theChip)
{
	RegisterDescriptor*	theReg = theChip->regs;
	UInt32			maxLen = 0;
	
	videoConsole->moveTo(0,2*videoConsole->charHeight());
	while(theReg->name)
	{
		if(theReg->flags & REG_READ_ONLY)
			cout << redMsg << theReg->name << ":\n";
		else
			cout << greenMsg << theReg->name << ":\n";
		if(strlen(theReg->name) > maxLen)
			maxLen = strlen(theReg->name);
		theReg++;
	}
	
	cout << whiteMsg;
	theReg = theChip->regs;
	UInt32			n = 0;
	while(theReg->name)
	{
		videoConsole->moveTo((maxLen+2)*videoConsole->charWidth(),(n+2)*videoConsole->charHeight());
		
		if(!(theReg->flags & (REG_WRITE_ONLY | REG_SIDE_EFFECTS)))
		{
			switch(theReg->size)
			{
				case 1:
					cout << ReadUReg8((UReg8*)((UInt32)theChip->addr + theReg->offset));
				break;
				case 2:
					if(theReg->flags & REG_LE)
						cout << ReadUReg16LE((UReg16LE*)((UInt32)theChip->addr + theReg->offset));
					else
						cout << ReadUReg16BE((UReg16BE*)((UInt32)theChip->addr + theReg->offset));
				break;
				case 4:
					if(theReg->flags & REG_LE)
						cout << ReadUReg32LE((UReg32LE*)((UInt32)theChip->addr + theReg->offset));
					else
						cout << ReadUReg32BE((UReg32BE*)((UInt32)theChip->addr + theReg->offset));
				break;
				default:
					cout << "Bad register size";
				break;
			}
		}
		else if(theReg->flags & REG_WRITE_ONLY)
			cout << "Write-only";
		else if(theReg->flags & REG_SIDE_EFFECTS)
			cout << "Side-effects";
		theReg++;
		n++;
	}
	
	return maxLen;
}

static UInt32 ReadReg(Chip* theChip,RegisterDescriptor* theReg)
{
	if(!(theReg->flags & REG_WRITE_ONLY))
	{
		switch(theReg->size)
		{
			case 1:
				return ReadUReg8((UReg8*)((UInt32)theChip->addr + theReg->offset));
			break;
			case 2:
				if(theReg->flags & REG_LE)
					return ReadUReg16LE((UReg16LE*)((UInt32)theChip->addr + theReg->offset));
				else
					return ReadUReg16BE((UReg16BE*)((UInt32)theChip->addr + theReg->offset));
			break;
			case 4:
				if(theReg->flags & REG_LE)
					return ReadUReg32LE((UReg32LE*)((UInt32)theChip->addr + theReg->offset));
				else
					return ReadUReg32BE((UReg32BE*)((UInt32)theChip->addr + theReg->offset));
			break;
		}
	}
	
	return 0;
}

static void WriteReg(Chip* theChip,RegisterDescriptor* theReg,UInt32 regContents)
{
	if(!(theReg->flags & REG_READ_ONLY))
	{
		switch(theReg->size)
		{
			case 1:
				WriteUReg8(regContents,(UReg8*)((UInt32)theChip->addr + theReg->offset));
			break;
			case 2:
				if(theReg->flags & REG_LE)
					WriteUReg16LE(regContents,(UReg16LE*)((UInt32)theChip->addr + theReg->offset));
				else
					WriteUReg16BE(regContents,(UReg16BE*)((UInt32)theChip->addr + theReg->offset));
			break;
			case 4:
				if(theReg->flags & REG_LE)
					WriteUReg32LE(regContents,(UReg32LE*)((UInt32)theChip->addr + theReg->offset));
				else
					WriteUReg32BE(regContents,(UReg32BE*)((UInt32)theChip->addr + theReg->offset));
			break;
		}
	}
}

static void SelectReg(RegisterDescriptor* theReg,UInt32 regsX,UInt32 regNumber)
{
	UInt32	x = regsX*videoConsole->charWidth() - 1;
	UInt32	y = (2+regNumber)*videoConsole->charHeight() - 1;
	UInt32	width = (2 + 2*theReg->size)*videoConsole->charWidth();
	Rect		theRect = {x,y,x+width,y+videoConsole->charHeight()};
	
	if(theReg->flags & REG_WRITE_ONLY)
		width = 10*videoConsole->charWidth();
	if(theReg->flags & REG_SIDE_EFFECTS)
		width = 12*videoConsole->charWidth();
	
	videoConsole->invertRect(&theRect);
}

static void EditReg(Chip* theChip,RegisterDescriptor* theReg,UInt32 regsX,UInt32 regNumber)
{
	if(theReg->flags & REG_READ_ONLY)
		return;
	
	UInt32	regContents = 0;
	
	videoConsole->moveTo(regsX*videoConsole->charWidth(),(regNumber+2)*videoConsole->charHeight());
	cout << "            ";
	videoConsole->moveTo(regsX*videoConsole->charWidth(),(regNumber+2)*videoConsole->charHeight());
	
	if(!(theReg->flags & REG_WRITE_ONLY))
		regContents = ReadReg(theChip,theReg);
	
	switch(theReg->size)
	{
		case 1:
			cout << (UInt8)regContents;
		break;
		case 2:
			cout << (UInt16)regContents;
		break;
		case 4:
			cout << (UInt32)regContents;
		break;
	}
	
	Rect		charRect = {(regsX + 2)*videoConsole->charWidth() - 1,(regNumber + 2)*videoConsole->charHeight() - 1,(regsX + 3)*videoConsole->charWidth() - 1,(regNumber + 3)*videoConsole->charHeight() - 1};
	UInt32	pos = 0;
	
	for(;;)
	{
		ASCII8	keyPressed[2] = {0,0};
		Int32	movement = 0;
		videoConsole->invertRect(&charRect);
		videoConsole->moveTo(charRect.x1 + 1,charRect.y1 + 1);
		cin >> keyPressed[0];
		videoConsole->invertRect(&charRect);
		
		if('0' <= keyPressed[0] && keyPressed[0] <= '9')
		{
			cout << keyPressed;
			regContents = (regContents & ~(0x0000000F << (theReg->size*2 - pos - 1)*4)) | ( ((UInt32)keyPressed[0] - '0') << (theReg->size*2 - pos - 1)*4);
			movement = 1;
		}
		else if('A' <= keyPressed[0] && keyPressed[0] <= 'F')
		{
			cout << keyPressed;
			regContents = (regContents & ~(0x0000000F << (theReg->size*2 - pos - 1)*4)) | ( ((UInt32)keyPressed[0] - 'A' + 10) << (theReg->size*2 - pos - 1)*4);
			movement = 1;
		}
		else if('a' <= keyPressed[0] && keyPressed[0] <= 'f')
		{
			cout << keyPressed;
			regContents = (regContents & ~(0x0000000F << (theReg->size*2 - pos - 1)*4)) | ( ((UInt32)keyPressed[0] - 'a' + 10) << (theReg->size*2 - pos - 1)*4);
			movement = 1;
		}
		else if(keyPressed[0] == KEYBOARD_RETURN)
			break;
		else if(keyPressed[0] == 0x68)	// Left arrow
			movement = -1;
		else if(keyPressed[0] == 0x6C)	// Right arrow
			movement = 1;
		
		if(movement)
		{
			if(0 <= pos + movement && pos + movement < theReg->size*2)
			{
				pos += movement;
				charRect.x1 += movement*videoConsole->charWidth();
				charRect.x2 += movement*videoConsole->charWidth();
			}
		}
	}
	
	WriteReg(theChip,theReg,regContents);
	
	DisplayRegs(theChip);
	/*
	videoConsole->moveTo(regsX*videoConsole->charWidth(),(regNumber+2)*videoConsole->charHeight());
	if(!(theReg->flags & (REG_WRITE_ONLY | REG_SIDE_EFFECTS)))
	{
		regContents = ReadReg(theChip,theReg);
		switch(theReg->size)
		{
			case 1:
				cout << (UInt8)regContents;
			break;
			case 2:
				cout << (UInt16)regContents;
			break;
			case 4:
				cout << regContents;
			break;
		}
	}
	else if(theReg->flags & REG_WRITE_ONLY)
		cout << "Write-only";
	else if(theReg->flags & REG_SIDE_EFFECTS)
		cout << "Side-effects";
	*/
}