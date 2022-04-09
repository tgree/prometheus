/*
	NKTimers.cp
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
	Patrick Varilly		-	Sunday, 16 Jan 2000	-	Reworked so it almost never calls GetTime_ns() (too expensive!).  Now only
												periodic timers use this, so beware of them.
*/

#include "NKTimers.h"
#include "Assembly.h"
#include "NKAtomicOperations.h"
#include "NKMachineInit.h"
#include "NKDebuggerNub.h"
#include "NKThreads.h"
#include "NKVideo.h"
#include "NKProcessors.h"
#include "Time.h"

// A note on timers:  The timer functuonality here provided is really meant to be used only by the scheduler.  Anywhere else in the kernel, should
// you need accurate timing, set up a thread with kernel priority and sleep for the amount of time desired.  Only other running kernel threads
// (which are few if any) could possibly prevent your thread from executing as soon as the timer expires.

// For one, timers are fixed to one processor and fire in the processor in which they were added.  This is *necessary* for the scheduler to
// function properly in an MP system, so don't try to go around this.  As mentioned above, use a kernel thread.

// The flags on a timer
enum
{
	kTimer_Valid		= 0x00000001,
	kTimer_Periodic	= 0x00000002
};

// Special decrementor register values
const UInt32						decTriggerNow = 0xFFFFFFFF;
const UInt32						decMax = 0x7FFFFFFF;

// A lock for timer queue editing (to prevent processors clashing)
SpinLock							timerLock;

void NKInitTimers()
{
	NKInstallExceptionHandler(&NKTimer::DecHandler, decrementerException);
}

NKTimer::NKTimer()
{
	// It's not valid, and it's not periodic
	flags = 0;
	nsTime = 0;
	next = prev = nil;
}

NKTimer::~NKTimer()
{
	remove();
}

// Schedule a timer to expire nsDelay nanoseconds after this (this is obviously way inaccurate, but, planning ahead, future machines
// just might be able to execute a hundred instructions in less than one ns [imagine that!])
void
NKTimer::add( Float64 nsDelay )
{
	CriticalSection					critical(timerLock);
	
	if( flags & kTimer_Valid )
		removeMe();				// Turn it off before re-scheduling
	
	nsTime = GetTime_ns() + nsDelay;
	flags &= ~kTimer_Periodic;
	addMe();
}

// Schedule a timer to be called periodically every nsDelay nanoseconds.  Don't set this too low.  Also, beware this is expensive.
// You use this by repeatedly calling addPeriodic.  The timer will execute more or less periodically (due to overhead; it won't "lag" or "lose time"),
// or ASAP if addPeriodic is called more than nsDelay nanoseconds apart
void
NKTimer::addPeriodic( Float64 nsDelay )
{
	if( (flags & kTimer_Periodic) == 0 )
	{
		add(nsDelay);
		flags |= kTimer_Periodic;
	}
	else
	{
		CriticalSection					critical(timerLock);
		
		if( flags & kTimer_Valid )
			removeMe();				// Turn it off before re-scheduling
	
		nsTime += nsDelay;
		addMe();
	}
}

// Turn off a timer (it might be too late, though)
void
NKTimer::remove()
{
	CriticalSection					critical(timerLock);
	
	if( flags & kTimer_Valid )
		removeMe();
}

void
NKTimer::signal()
{
	// Does nothing.  However, at the end of every timer interrupt, the scheduler checks if it's necessary to switch threads, making the
	// raw NKTimer class sufficient for part of the scheduler
}

// Internal function:  actually puts the timer in the timer queue (sorted by ascending nsTime)
void
NKTimer::addMe()
{
	// We're supposed to be in a critical section by now
	Boolean				resched = false;
	ProcessorInfo*			info = NKGetThisProcessorInfo();
	
	processor = info->number;	// Save processor number
	if( !info->timerHead )
	{
		next = prev = nil;
		info->timerHead = this;
		resched = true;
	}
	else
	{
		NKTimer			*previous = nil, *cur = info->timerHead;
		while( cur )
		{
			if( cur->nsTime >= nsTime )
				break;
			previous = cur;
			cur = cur->next;
		}
		
		if( !previous )
		{
			prev = nil;
			next = info->timerHead;
			info->timerHead = this;
			next->prev = this;
			resched = true;
		}
		else
		{
			next = previous->next;
			if( next )
				next->prev = this;
			prev = previous;
			prev->next = this;
		}
	}
	
	if( resched )
		adjustDec();
	
	flags |= kTimer_Valid;
}

// Internal function: actually takes the timer off the timer queue
void
NKTimer::removeMe()
{
	// We're supposed to be in a critical section by now
	ProcessorInfo*			info = NKGetProcessorInfo(processor);	// Remove it from the original processor
	if( this == info->timerHead )
	{
		info->timerHead = next;
		if( next )
			next->prev = nil;
	}
	else
	{
		if( !info->timerHead )
			Panic( "Timing facilities are messed up!" );
		
		prev->next = next;
		if( next )
			next->prev = prev;
	}
	
	flags &= ~kTimer_Valid;
}

// Adjusts the decrementor register so as to fire a timer when is needed
void
NKTimer::adjustDec()
{
	// Assumes you're in critical
	ProcessorInfo*			info = NKGetThisProcessorInfo();
	if( !info->timerHead )
		_setDEC( decMax );
	else
	{
		Float64				curNS = GetTime_ns();
		if( info->timerHead->nsTime <= curNS )
		{
			// Change high bit from 0 to 1
			_setDEC( decMax );
			_setDEC( decTriggerNow );
		}
		else
		{
			// Get clock delay
			UInt64			clockDiff;
			clockDiff = nsecToClock(info->timerHead->nsTime-curNS);
			if( clockDiff > decMax )
				_setDEC( decMax );
			else
				_setDEC( clockDiff );
		}
	}
}

// This is the nuts and bolts of it all!  This loops, getting an expired timer off the queue and running it until no more
// expired timers exist.  Then it calls the scheduler to switch threads (if necessary) and sets up the decrementor
Boolean
NKTimer::DecHandler( PPCRegisters* savedRegs )
{
	// Loop until no more expired timers
	Float64					curNS = GetTime_ns();
	while(1)
	{
		NKTimer				*timer;
		ProcessorInfo			*info = NKGetThisProcessorInfo();
		{
			CriticalSection		critical(timerLock);
			
			if( !info->timerHead )
				break;
			if( info->timerHead->nsTime > curNS )
				break;
			
			// Found an expired timer, dequeue it
			timer = info->timerHead;
			info->timerHead = info->timerHead->next;
			if( info->timerHead )
				info->timerHead->prev = nil;
			timer->flags &= ~kTimer_Valid;
		}
		
		// Fire it
		timer->signal();
	}
	
	// Schedule if necessary
	NKCheckSchedule( savedRegs );
	
	// Set up next decrementer
	adjustDec();
	
	return true;
}

// Zapping the time base
// ================
// All processors must keep the same time, or be amazingly close to each other.  The following routine attempts to do so, by waiting for all processors
// to call it, then clearing the time base, hopefully doing this at exactly the same time on every processor.  Note that the decrementor clock speed
// (i.e. the time base clock speed) is the same throughout the system, as it's either fixed for the processor (i.e. 601s) or is dependent on the bus speed.

// Must find a better way to do this, CW won't let me get the address of a global UInt32 by saying: "addi  rX,rtoc,myGlobal", though that's exactly
// the code itself outputs when I ask for &myGlobal in C.  Ideas? -- Pat
// Yep: la rX,myGlobal(rtoc)

static UInt32					numProcessorsCheckedIn[2] = {0,0};

__asm__ void zapClockSync()
{
	// Assume we're in a critical section!!!!
	
	// Read number of processors in and sync, to minimize offsets by memory delays
	lwz		r3, machine(rtoc);
	lwz		r3, MachineInfo.numProcessors(r3);
	lwz		r4,numProcessorsCheckedIn(rtoc);		// Loads ea, I hope!
	sync;
	
	// Now atomically increment numProcessorsCheckedIn
loop:	lwarx	r5,r0,r4;
	addi		r5,r5,1;
	stwcx.	r5,r0,r4;
	bne-		loop;
	
	// Wait for numProcessorsCheckedIn to match number of processors
loop2:cmpw	r5,r3;
	lwz		r5,numProcessorsCheckedIn(rtoc);
	bne-		loop2;
	
	// Now zap clocks
	li		r4,0
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,1;
	beq-		@RTCTime;
@TBTime:
	mtspr	TBL_W,r4;
	mtspr	TBU_W,r4;
	mtspr	TBL_W,r4;
	blr;
@RTCTime:
	mtspr	RTCL_W,r4;
	mtspr	RTCU_W,r4;
	mtspr	RTCL_W,r4;
	blr;
}