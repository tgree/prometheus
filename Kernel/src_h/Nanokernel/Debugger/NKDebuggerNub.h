/*
	NKDebuggerNub.h
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
#ifndef __NKDEBUGGER_NUB__
#define __NKDEBUGGER_NUB__

#include "NKMemoryManager.h"
#include "Assembly.h"

// Derive your debugger from NKDebuggerInterface to get access to stuff
class NKDebuggerInterface	:	public KernelObject
{
protected:
	virtual ~NKDebuggerInterface();
	
	// You should overload this.  It is called when you need to draw a debugStr message.  Do not stop for this however.
	virtual	void	debugStr(ConstASCII8Str theStr);
	
	// None of these NEED to be overloaded, but you will probable want to
	virtual	void	userBreak();			// This is called when the debugger is dropped into, via NMI or programmer calling it.
	virtual	void	machineCheck();		// This is called when a machine check interrupt happens
	virtual	void	trace();				// This is called when a trace interrupt happens
	virtual	void	dsi();				// This is called when a dsi interrupt happens
	virtual	void	isi();					// This is called when an isi interrupt happens
	virtual	void	alignment();			// This is called when an alignment interrupt happens
	virtual	void	program();			// This is called when a program interrupt happens
	virtual	void	fpuUnavail();			// This is called when an fpu unavailable interrupt happens
	virtual	void	sysCall();				// This is called when an sc instruction is executed
	virtual	void	unimp(UInt32 vecNum);	// This is called when a bad exception vector is called (ie. never)
	
	// This will display a standard error message with register info
	virtual	void	displayExceptionInfo(ConstASCII8Str vecName);
	friend class NKDebuggerNub;
};

class NKDebuggerNub	:	public KernelObject
{
	ASCII8					str[1024];
	UInt32					pos;
	class NKDebuggerInterface*	currDebugger;
	class PPCRegisters*			interruptRegs;
	Boolean					inDebugger;
	
	NKDebuggerNub();
	~NKDebuggerNub();
	
public:
	// This is called by internal interrupt stuff when an interrupt comes along.  Don't call it yourself
	// unless you are trying to emulate a debugger interrupt or something stupid.
	void	interrupt(UInt32 num,PPCRegisters* regs);
	
	// Use this to get at the registers for this exception
	PPCRegisters*	getRegisters();
	
	// Tracing
	void		enableTrace();		// Causes a trace() on the next instruction
	void		disableTrace();		// Turns off tracing
	
	// Getting the debugger
	void		registerDebugger(NKDebuggerInterface* interface);
	void		unregisterDebugger(NKDebuggerInterface* interface);
	
	// Enter the debugger
	void		debugger();
	
	// Display a message in the debugger
	void		debugStr(ConstASCII8Str str);
	
	friend void NKInitDebuggerNub();
};

extern NKDebuggerNub*	debuggerNub;

class NKDebuggerStream
{
};

extern NKDebuggerStream	dout;

const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,ConstASCII8Str str);
const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,UInt32 n);
const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,Int32 n);
const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,UInt16 n);
const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,Int16 n);
const NKDebuggerStream&	operator<<(const NKDebuggerStream& s,UInt8 n);

void _Assert(ConstASCII8Str file,ConstASCII8Str cond,UInt32 line,Boolean fatal,UInt32 lr);

#define Panic(str)		do{dout << "Kernel Panic:" str "\n"; for(;;){}}while(0)
#define Assert(cond)	do{if(!(cond)) _Assert(__FILE__,#cond,__LINE__,false,_getLR());}while(0)
#define FatalAssert(cond)	do{if(!(cond)) _Assert(__FILE__,#cond,__LINE__,true,_getLR());}while(0)

#endif /* __NKDEBUGGER_NUB__ */