/*
	NKSerialDebugger.cp
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
#include "NKSerialDebugger.h"
#include "NKMachineInit.h"
#include "ANSI.h"
#include "NKVirtualMemory.h"
#include "NKProcessors.h"
#include "NKThreads.h"
#include "NKInterruptVectors.h"
#include "NKDisassembly.h"

static UInt32			serialDebuggerData[ROUND_UP(4,sizeof(NKSerialDebugger))];
static NKSerialDebugger*	serialDebugger;

// Taken from VT100 Console.cp
static const ASCII8	plainMode[] =					"\x1B[0m";
static const ASCII8	boldMode[] =					"\x1B[1m";
static const ASCII8	underlineMode[] =				"\x1B[4m";
static const ASCII8	inverseMode[] =				"\x1B[7m";
static const ASCII8	invisibleMode[] =				"\x1B[8m";
static const ASCII8	clearToEndOfLineCode[] =			"\x1B[0K";
static const ASCII8	clearFromStartOfLineCode[] =		"\x1B[1K";
static const ASCII8	clearLineCode[] =				"\x1B[2K";
static const ASCII8	clearToBottomOfScreenCode[] =	"\x1B[0J";
static const ASCII8	clearFromTopOfScreenCode[] =		"\x1B[1J";
static const ASCII8	clearScreenCode[] =				"\x1B[2J";
static const ASCII8	resetCode[] =					"\033c";

Boolean NKInitSerialDebugger()
{
	if(machine.sccAddr)
	{
		serialDebugger = new((void*)serialDebuggerData) NKSerialDebugger((esccDevice*)machine.sccAddr);
		serialDebugger->serial << clearScreenCode;
		serialDebugger->doBackgroundDisplay();
		serialDebugger->serial << boldMode;
		serialDebugger->textBox << "NKSerialDebugger v. 1.0\n";
		serialDebugger->serial << plainMode;
		serialDebugger->textBox << "Last build Jan. 31, 2000, by Terry Greeniaus\nRunning on modem port at 38400 N81\nVT100-style terminal assumed.\n\n";
		machine.sccInUseForDebugger = true;
		debuggerNub->registerDebugger(serialDebugger);
		debuggerNub->debugger();
		return true;
	}
	
	return false;
}

NKSerialDebugger::NKSerialDebugger(esccDevice* sccRegs)
{
	NKInitSerialStream(serial,sccRegs,19200);
	UInt32*	p = (UInt32*)&lastRegisters;
	for(UInt32 i=0;i<sizeof(PPCRegisters)/4;i++)
		*p++ = 0xDEADBEEF;
	regMode = 0;
}

NKSerialDebugger::~NKSerialDebugger()
{
	debuggerNub->unregisterDebugger(this);
}

void NKSerialDebugger::debugStr(ConstASCII8Str str)
{
	//serial << str;
	textBox << str;
}

void NKSerialDebugger::doBackgroundDisplay()
{
	NKInitSerialStreamTextBox(textBox,serial,33,1,100,20);
	
	serial << boldMode;
	serial << decMsg;
	for(UInt32 i=0;i<16;i++)
	{
		moveTo(1,i+1);
		serial << "r" << i << "=";
		moveTo(17,i+1);
		serial << "r" << (i+16) << "=";
	}
	
	moveTo(1,18);
	serial << "lr=";
	moveTo(1,19);
	serial << "ctr=";
	moveTo(1,20);
	serial << "xer=";
	moveTo(1,21);
	serial << "fpscr=";
	moveTo(1,22);
	serial << "cr=";
	moveTo(1,23);
	serial << "srr0=";
	moveTo(1,24);
	serial << "srr1=";
	moveTo(1,25);
	serial << "dar=";
	moveTo(1,26);
	serial << "dsisr=";
}

void NKSerialDebugger::doDisplay()
{
	PPCRegisters*	regs = debuggerNub->getRegisters();
	
	serial << plainMode;
	serial << hexMsg;
	for(UInt32 i=0;i<16;i++)
	{
		if(shouldDoValue(lastRegisters.r[i],regs->r[i],lastHilite.r[i]))
		{
			moveTo(5,i+1);
			setMode(lastRegisters.r[i],regs->r[i]);
			serial << regs->r[i];
		}
		lastHilite.r[i] = regMode;
		
		if(shouldDoValue(lastRegisters.r[i+16],regs->r[i+16],lastHilite.r[i+16]))
		{
			moveTo(21,i+1);
			setMode(lastRegisters.r[i+16],regs->r[i+16]);
			serial << regs->r[i+16];
		}
		lastHilite.r[i+16] = regMode;
	}
	if(shouldDoValue(lastRegisters.lr,regs->lr,lastHilite.lr))
	{
		moveTo(7,18);
		setMode(lastRegisters.lr,regs->lr);
		serial << regs->lr;
	}
	lastHilite.lr = regMode;
	if(shouldDoValue(lastRegisters.ctr,regs->ctr,lastHilite.ctr))
	{
		moveTo(7,19);
		setMode(lastRegisters.ctr,regs->ctr);
		serial << regs->ctr;
	}
	lastHilite.ctr = regMode;
	if(shouldDoValue(lastRegisters.xer,regs->xer,lastHilite.xer))
	{
		moveTo(7,20);
		setMode(lastRegisters.xer,regs->xer);
		serial << regs->xer;
	}
	lastHilite.xer = regMode;
	if(shouldDoValue(lastRegisters.fpscr,regs->fpscr,lastHilite.fpscr))
	{
		moveTo(7,21);
		setMode(lastRegisters.fpscr,regs->fpscr);
		serial << regs->fpscr;
	}
	lastHilite.fpscr = regMode;
	if(shouldDoValue(lastRegisters.cr,regs->cr,lastHilite.cr))
	{
		moveTo(7,22);
		setMode(lastRegisters.cr,regs->cr);
		serial << regs->cr;
	}
	lastHilite.cr = regMode;
	if(shouldDoValue(lastRegisters.srr0,regs->srr0,lastHilite.srr0))
	{
		moveTo(7,23);
		setMode(lastRegisters.srr0,regs->srr0);
		serial << regs->srr0;
	}
	lastHilite.srr0 = regMode;
	if(shouldDoValue(lastRegisters.srr1,regs->srr1,lastHilite.srr1))
	{
		moveTo(7,24);
		setMode(lastRegisters.srr1,regs->srr1);
		serial << regs->srr1;
	}
	lastHilite.srr1 = regMode;
	if(shouldDoValue(lastRegisters.dar,regs->dar,lastHilite.dar))
	{
		moveTo(7,25);
		setMode(lastRegisters.dar,regs->dar);
		serial << regs->dar;
	}
	lastHilite.dar = regMode;
	if(shouldDoValue(lastRegisters.dsisr,regs->dsisr,lastHilite.dsisr))
	{
		moveTo(7,26);
		setMode(lastRegisters.dsisr,regs->dsisr);
		serial << regs->dsisr;
	}
	lastHilite.dsisr = regMode;
	
	setMode(0,0);
	lastRegisters = *regs;
	
	if(displayDisassembly)
	{
		ASCII8	str[60];
		Disassemble(*(UInt32*)regs->srr0,str);
		textBox << str << "\n";
	}
}

Boolean NKSerialDebugger::shouldDoValue(UInt32 oldValue,UInt32 newValue,UInt32 oldMode)
{
	if(oldValue != newValue)
		return true;
	if(oldValue == newValue && oldMode == 1)
		return true;
	return false;
}

void NKSerialDebugger::setMode(UInt32 oldVal,UInt32 newVal)
{
	if(oldVal == newVal && regMode != 0)
	{
		regMode = 0;
		serial << plainMode;
	}
	else if(oldVal != newVal && regMode != 1)
	{
		regMode = 1;
		serial << inverseMode;
	}
}

void NKSerialDebugger::moveTo(UInt32 x,UInt32 y)
{
	serial.moveTo(serial,x,y);
}

void NKSerialDebugger::userBreak()
{
	doDisplay();
	/*
	ProcessorInfo*	pInfo = NKGetThisProcessorInfo();
	serial << "\aUser break on processor " << pInfo->number << "\r";
	if(pInfo->thread)
		serial << "Current thread: \"" << NKGetThisProcessorInfo()->thread->threadName() << "\"";
	else
		serial << "Current thread: \"Kernel Boot Setup Thread\"";
	serial << "\rType 'g' to continue.\r";
	*/
	moveTo(0,28);
	serial << "\aUser break - Type 'g' to continue.";
	for(;;)
	{
		ASCII8	inputStr[1025] = {0};
		UInt32	inputLen = 0;
		
		moveTo(0,29);
		serial << clearLineCode;
		serial << "> ";
		serial >> inputStr;
		
		if(!strcmp(inputStr,"g",false))
			break;
		else if(!strcmp(inputStr,"t",false))
		{
			debuggerNub->getRegisters()->srr1 |= 0x0600;
			displayDisassembly = false;
			break;
		}
		else if(!strcmp(inputStr,"td",false))
		{
			debuggerNub->getRegisters()->srr1 |= 0x0600;
			displayDisassembly = true;
			break;
		}
		else
		{
			moveTo(0,28);
			serial << clearLineCode;
			serial << "Syntax error";
		}
	}
	
	moveTo(0,28);
	serial << clearToBottomOfScreenCode;
}

void NKSerialDebugger::trace()
{
	if(serial.charAvailable(serial))
	{
		if(serial.read(serial) == ' ')
		{
			debuggerNub->getRegisters()->srr1 &= ~0x0600;
			userBreak();
			return;
		}
	}
	doDisplay();
}
