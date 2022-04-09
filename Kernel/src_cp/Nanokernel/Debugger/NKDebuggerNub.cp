/*
	NKDebuggerNub.cp
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
	Terry Greeniaus	-	Wed, 16 Dec. 1998	-	Original creation of file
*/
#include "NKDebuggerNub.h"
#include "NKInterruptVectors.h"
#include "Streams.h"
#include "ANSI.h"

class SimpleDebuggerInterface	:	public NKDebuggerInterface
{
public:
	virtual	void	debugStr(ConstASCII8Str str) {nkVideo << redMsg << str << whiteMsg;}
};

NKDebuggerStream			dout;
NKDebuggerNub*			debuggerNub;
static NKDebuggerInterface*	simpleInterface;
static UInt32				debuggerNubData[ROUND_UP(4,sizeof(NKDebuggerNub))];
static UInt32				simpleInterfaceData[ROUND_UP(4,sizeof(SimpleDebuggerInterface))];

static void NKStartTrace();
static void NKEndTrace();
static void _EnterDebugger();

void NKInitDebuggerNub()
{
	simpleInterface = new((void*)simpleInterfaceData) SimpleDebuggerInterface;
	debuggerNub = new((void*)debuggerNubData) NKDebuggerNub;
}

NKDebuggerNub::NKDebuggerNub()
{
	currDebugger = nil;
	pos = 0;
	interruptRegs = nil;
	inDebugger = false;
}

NKDebuggerNub::~NKDebuggerNub()
{
}

void NKDebuggerNub::interrupt(UInt32 vecNum,PPCRegisters* regs)
{
	NKDebuggerInterface*	theDebugger = (currDebugger ? currDebugger : simpleInterface);
	
	inDebugger = true;
	interruptRegs = regs;
	switch(vecNum)
	{
		case machineCheckException:		theDebugger->machineCheck();		break;
		case dataStorageException:		theDebugger->dsi();				break;
		case instructionStorageException:	theDebugger->isi();				break;
		case alignmentException:			theDebugger->alignment();		break;
		case programException:			theDebugger->program();			break;
		case fpuUnavailableException:		theDebugger->fpuUnavail();		break;
		case systemCallException:
			if(regs->srr0 == FUNC_ADDR(_EnterDebugger) + 4)
			{
				regs->srr0 = regs->lr;
				theDebugger->userBreak();
			}
			else
				theDebugger->sysCall();
		break;
		case traceException:				theDebugger->trace();			break;
		default:						theDebugger->unimp(vecNum);		break;
	}
	inDebugger = false;
}

PPCRegisters* NKDebuggerNub::getRegisters()
{
	return interruptRegs;
}

void NKDebuggerNub::enableTrace()
{
	NKStartTrace();
}

void NKDebuggerNub::disableTrace()
{
	NKEndTrace();
}

void NKDebuggerNub::registerDebugger(NKDebuggerInterface* interface)
{
	if(!currDebugger)
		currDebugger = interface;
	else
		currDebugger->debugStr("Someone tried to steal the debugger Nub!\n");
}

void NKDebuggerNub::unregisterDebugger(NKDebuggerInterface* interface)
{
	if(currDebugger)
	{
		if(currDebugger == interface)
			currDebugger = nil;
		else
			currDebugger->debugStr("Someone tried to unregister the wrong debugger!\n");
	}
}

void NKDebuggerNub::debugger()
{
	_EnterDebugger();
}

void NKDebuggerNub::debugStr(ConstASCII8Str theStr)
{
	NKDebuggerInterface*	theDebugger = (currDebugger ? currDebugger : simpleInterface);
	Boolean				shouldEnter = false;
	while(*theStr)
	{
		while(pos < 1024 && *theStr)
		{
			str[pos++] = *theStr;
			if(*theStr++ == '\n')
			{
				shouldEnter = true;
				break;
			}
		}
		str[pos < 1024 ? pos : 1023] = 0;
		pos = 0;
		theDebugger->debugStr(str);
	}
	
	if(!inDebugger && shouldEnter)
		_EnterDebugger();
}

NKDebuggerInterface::~NKDebuggerInterface()
{
}

void NKDebuggerInterface::debugStr(ConstASCII8Str str)
{
	nkVideo << str;
}

void NKDebuggerInterface::userBreak()
{
	dout << "Simple debugger interface - user break.  Continuing...\n";
}

void NKDebuggerInterface::machineCheck()
{
	displayExceptionInfo("Machine check");
	for(;;)
		;
}

void NKDebuggerInterface::trace()
{
	dout << "Trace: " << debuggerNub->getRegisters()->srr0 << "\n";
}

void NKDebuggerInterface::dsi()
{
	displayExceptionInfo("DSI");
	if(debuggerNub->getRegisters()->dar < 4096)
		dout << "Don't use a nil pointer you moron!\n";
	for(;;)
		;
}

void NKDebuggerInterface::isi()
{
	displayExceptionInfo("ISI");
	if(debuggerNub->getRegisters()->srr0 < 4096)
		dout << "Don't use a nil pointer you moron!\n";
	for(;;)
		;
}

void NKDebuggerInterface::alignment()
{
	displayExceptionInfo("Alignment");
	for(;;)
		;
}

void NKDebuggerInterface::program()
{
	displayExceptionInfo("Program");
	for(;;)
		;
}

void NKDebuggerInterface::fpuUnavail()
{
	displayExceptionInfo("FPU Unavailable");
	for(;;)
		;
}

void NKDebuggerInterface::sysCall()
{
	displayExceptionInfo("System call");
	for(;;)
		;
}

void NKDebuggerInterface::unimp(UInt32 vecNum)
{
	displayExceptionInfo("Unimplemented vector");
	dout << "Vector number: " << vecNum << "\n";
	for(;;)
		;
}

void NKDebuggerInterface::displayExceptionInfo(ConstASCII8Str vecName)
{
	dout << "\n------------------------------\n";
	dout << "Interrupt: " << vecName << "\n";
	dout << "    DSISR: " << debuggerNub->getRegisters()->dsisr << "\n";
	dout << "    DAR:   " << debuggerNub->getRegisters()->dar << "\n";
	dout << "    PC:    " << debuggerNub->getRegisters()->srr0 << "\n";
	dout << "    SRR1:  " << debuggerNub->getRegisters()->srr1 << "\n";
	dout << "    LR:    " << debuggerNub->getRegisters()->lr << "\n";
	dout << "------------------------------\n\n";
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,ConstASCII8Str str)
{
	debuggerNub->debugStr(str);
	return s;
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,UInt32 n)
{
	ASCII8	str[11];
	debuggerNub->debugStr(num2hex(n,str));
	return s;
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,Int32 n)
{
	ASCII8	str[12];
	debuggerNub->debugStr(num2hex(n,str));
	return s;
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,UInt16 n)
{
	return operator<<(s,(UInt32)n);
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,Int16 n)
{
	return operator<<(s,(Int32)n);
}

const NKDebuggerStream& operator<<(const NKDebuggerStream& s,UInt8 n)
{
	return operator<<(s,(UInt32)n);
}

void _Assert(ConstASCII8Str file,ConstASCII8Str cond,UInt32 line,Boolean fatal,UInt32 lr)
{
	if(fatal)
		dout << "Fatal ";
	dout << "Kernel Assert in " << file << " at line " << line << ": " << cond << ", lr = " << lr << "\n";
	while(fatal)
		;
}

asm void NKStartTrace()
{
	mfmsr	r3;
	ori		r3,r3,0x0600;	// Turn on branch tracing and single step tracing.
	sync;
	mtmsr	r3;
	sync;
	blr;
}

asm void NKEndTrace()
{
	mfmsr	r3;
	rlwinm	r3,r3,0,23,20;
	sync;
	mtmsr	r3;
	sync;
	blr;
}

asm void _EnterDebugger()
{
	sc;
	blr;
}