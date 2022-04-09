/*
	NKTimers.h
	Copyright © 2000 by Patrick Varilly

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
	Patrick Varilly		-	Sunday, 9 Jan 2000		-	Creation of file
*/

#ifndef __NK_TIMERS__
#define __NK_TIMERS__

#include "Time.h"
#include "NKInterruptVectors.h"

class NKTimer
{
public:
					NKTimer();
	virtual			~NKTimer();
	
	void				add( Float64 nsDelay );
	void				addPeriodic( Float64 nsDelay );
	void				remove();
	
	virtual void		signal();		// By default does nothing, but it triggers scheduler
								// to check if a thread switch is necessary
	
private:
	UInt32			flags;
	Float64			nsTime;
	NKTimer			*next, *prev;
	UInt32			processor;	// Number of processor where timer is running
	
	void				addMe();
	void				removeMe();
	
	static void		adjustDec();
	
protected:
	friend void NKInitTimers();
	static Boolean		DecHandler( PPCRegisters* savedRegs );
		// This gets called every decrementor interrupt.  All regs are saved, and all can be modified
		// (i.e. scheduling can be done here)
};

void NKInitTimers();
void zapClockSync();

#endif /* __NK_TIMERS__ */