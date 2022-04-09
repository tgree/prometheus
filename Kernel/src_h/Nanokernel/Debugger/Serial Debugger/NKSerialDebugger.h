/*
	NKSerialDebugger.h
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
	Terry Greeniaus	-	Sun, 30 Jan. 2000	-	Original creation of file
*/
#ifndef __NKSERIAL_DEBUGGER__
#define __NKSERIAL_DEBUGGER__

#include "ESCC.h"
#include "NKDebuggerNub.h"
#include "NKSerialStream.h"
#include "NKInterruptVectors.h"

class NKSerialDebugger	:	public NKDebuggerInterface
{
	NKSerialStream		serial;
	NKSerialStreamTextBox	textBox;
	
	PPCRegisters		lastRegisters;
	PPCRegisters		lastHilite;
	UInt32			regMode;
	Boolean			displayDisassembly;
	
			void		doBackgroundDisplay();
			Boolean	shouldDoValue(UInt32 oldValue,UInt32 newValue,UInt32 oldMode);
			void		setMode(UInt32 oldValue,UInt32 newValue);
			void		doDisplay();
			void		moveTo(UInt32 x,UInt32 y);
protected:
	NKSerialDebugger(esccDevice* sccRegs);
	virtual ~NKSerialDebugger();
	
	virtual	void	debugStr(ConstASCII8Str theStr);
	virtual	void	userBreak();
	virtual	void	trace();
	friend Boolean NKInitSerialDebugger();	// Returns true if inited
};

#endif /* __NKSERIAL_DEBUGGER__ */