/*
	Time.cp
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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Monday, 3 August 98	-	Reworked so that it uses Benchmarks.cp file to get the clock speeds
	Patrick Varilly		-	Sunday, 16 Jan 2000	-	Revised to use Float64s instead of UInt64s (64-bit division is *horribly* expensive, about 10
												times more so than converting a UInt64 to a Float64, doing fp division and converting back)
*/
#include "Assembly.h"
#include "Macros.h"
#include "NKMachineInit.h"
#include "Time.h"
#include "Streams.h"
#include "VIA Chip.h"
#include "NKVirtualMemory.h"
#include "NKProcessors.h"
#include "Benchmarks.h"

/* PLL processor-bus frequency table:
Numbers in [] are found only in the Motorola "clockmodes.pdf" file available from their web site - they are NOT found in the individual hardware specs of that particular processor.
Numbers in () are found only in the hardware specs for the particular processor and NOT in the Motorola "clockmodes.pdf" file available from their web site.
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
Processor		0000	0001	0010	0011	0100	0101	0110	0111	1000	1001	1010	1011	1100	1101	1110	1111
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
602*		-		-		-		[BYP]	[2.0]		[2.0]		-		-		[3.0]		-		-		-		-		-		-		[OFF]
603			1.0		1.0		(1.0)		BYP		2.0		2.0		-		-		3.0		3.0		-		-		4.0		[4.0]		-		OFF
603e			1.0		1.0		1.0		BYP		2.0		2.0		2.5		-		3.0		-		4.0		-		1.5		-		3.5		OFF
603e-PID7v	-		-		-		BYP		2.0		2.0		2.5		4.5		3.0		5.5		4.0		5.0		-		6.0		3.5		OFF
603e-PID7t	-		-		-		BYP		2.0		2.0		2.5		4.5		3.0		5.5		4.0		5.0		-		6.0		3.5		OFF
604			[1.0]		1.0		[1.0]		BYP		2.0		2.0		-		-		3.0		(3.0)		-		-		1.5		(1.5)		-		[OFF]
604e-PID9v	[1.0]		[1.0]		[1.0]		BYP		2.0		[2.0]		2.5		-		3.0		-		4.0		(5.0)		1.5		(6.0)		(3.5)		OFF
604e-PID9q¥	(1.0)		(1.0)		(2.0)		(BYP)	(2.0)		(6.5)		(2.5)		(4.5)		(3.0)		(5.5)		(4.0)		(5.0)		(1.5)		(6.0)		(3.5)		(OFF)
740/750¥	-		(7.5)		(7.0)		(BYP)	-		(6.5)		(10.0)¦	(4.5)		(3.0)		(5.5)		(4.0)		(5.0)		(8.0)		(6.0)		(3.5)		(OFF)
7400		(9.0)		(7.5)		(7.0)		(BYP)	(2.0)		(6.5)		(2.5)		(4.5)		(3.0)		(5.5)		(4.0)		(5.0)		(8.0)		(6.0)		(3.5)		(OFF)
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
*couldn't find a hardware spec file for the 602 processor
¥604e-PID9q(Mach V), 740/750(Arthur) and 7400(Max) are not listed in "clockmodes.pdf" files
¦only on certain 10x 750 processors (IBM only I think)
--------------------------------------------------------------------------------------------------------------------------------------------------------------------
*/
static const UInt32	pll_num_601[16]			=	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	};
static const UInt32	pll_denom_601[16]			=	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	};
static const UInt32	pll_num_602[16]			=	{	0,	0,	0,	0,	2,	2,	0,	0,	3,	0,	0,	0,	0,	0,	0,	0	};
static const UInt32	pll_denom_602[16]			=	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	};
static const UInt32	pll_num_603[16]			=	{	1,	1,	1,	0,	2,	2,	0,	0,	3,	3,	0,	0,	4,	4,	0,	0	};
static const UInt32	pll_denom_603[16]			=	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	};
static const UInt32	pll_num_603e[16]			=	{	1,	1,	1,	0,	2,	2,	5,	0,	3,	0,	4,	0,	3,	0,	7,	0	};
static const UInt32	pll_denom_603e[16]			=	{	1,	1,	1,	1,	1,	1,	2,	1,	1,	1,	1,	1,	2,	1,	2,	1	};
static const UInt32	pll_num_603e_PID7v[16]		=	{	0,	0,	0,	0,	2,	2,	5,	9,	3,	11,	4,	5,	0,	6,	7,	0	};
static const UInt32	pll_denom_603e_PID7v[16]	=	{	1,	1,	1,	1,	1,	1,	2,	2,	1,	2,	1,	1,	1,	1,	2,	1	};
static const UInt32	pll_num_603e_PID7t[16]		=	{	0,	0,	0,	0,	2,	2,	5,	9,	3,	11,	4,	5,	0,	6,	7,	0	};
static const UInt32	pll_denom_603e_PID7t[16]	=	{	1,	1,	1,	1,	1,	1,	2,	2,	1,	2,	1,	1,	1,	1,	2,	1	};
static const UInt32	pll_num_604[16]			=	{	1,	1,	1,	0,	2,	2,	0,	0,	3,	3,	0,	0,	3,	3,	0,	0	};
static const UInt32	pll_denom_604[16]			=	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	2,	2,	1,	1	};
static const UInt32	pll_num_604e_PID9v[16]		=	{	1,	1,	1,	0,	2,	2,	5,	0,	3,	0,	4,	5,	3,	6,	7,	0	};
static const UInt32	pll_denom_604e_PID9v[16]	=	{	1,	1,	1,	1,	1,	1,	2,	1,	1,	1,	1,	1,	2,	1,	2,	1	};
static const UInt32	pll_num_604e_PID9q[16]		=	{	1,	1,	2,	0,	2,	13,	5,	9,	3,	11,	4,	5,	3,	6,	7,	0	};
static const UInt32	pll_denom_604e_PID9q[16]	=	{	1,	1,	1,	1,	1,	2,	2,	2,	1,	2,	1,	1,	2,	1,	2,	1	};
static const UInt32	pll_num_750[16]			=	{	0,	15,	7,	0,	0,	13,	10,	9,	3,	11,	4,	5,	8,	6,	7,	0	};
static const UInt32	pll_denom_750[16]			=	{	1,	2,	1,	1,	1,	2,	1,	2,	1,	2,	1,	1,	1,	1,	2,	1	};
static const UInt32	pll_num_620[16]			=	{	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0	};
static const UInt32	pll_denom_620[16]			=	{	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1,	1	};
static const UInt32	pll_num_7400[16]			=	{	9,	15,	7,	0,	2,	13,	5,	9,	3,	11,	4,	5,	8,	6,	7,	0	};
static const UInt32	pll_denom_7400[16]			=	{	1,	2,	1,	1,	1,	2,	2,	2,	1,	2,	1,	1,	1,	1,	2,	1	};
static const UInt32*	pll_num[]					=	{	nil,
												pll_num_601,			// This machine has no HID1 register to read the PLL
												pll_num_602,			// This machine has no HID1 register to read the PLL
												pll_num_603,			// This machine has no HID1 register to read the PLL
												pll_num_604,			// This machine has no HID1 register to read the PLL
												nil,
												pll_num_603e,
												pll_num_603e_PID7v,
												pll_num_750,
												pll_num_604e_PID9v,
												pll_num_604e_PID9q,
												nil,
												pll_num_7400
											};
static const UInt32*	pll_denom[]				=	{	nil,
												pll_denom_601,			// This machine has no HID1 register to read the PLL
												pll_denom_602,			// This machine has no HID1 register to read the PLL
												pll_denom_603,			// This machine has no HID1 register to read the PLL
												pll_denom_604,			// This machine has no HID1 register to read the PLL
												nil,
												pll_denom_603e,
												pll_denom_603e_PID7v,
												pll_denom_750,
												pll_denom_604e_PID9v,
												pll_denom_604e_PID9q,
												nil,
												pll_denom_7400
											};

// This is the speed of the VIA clock on Power Mac computers, in Hz
#define	VIA_CLOCK_SPEED	783360	//785411	// -	on my 7500, the VIA chip appears to go at 783.308615 KHz, an error of 0.00656%
											// -	on my B&W, the VIA chips appears to go at 785.411 KHz, an error of  0.26182% (That's pretty big, it gets the 300 MHz processor off by .8 MHz - 800000 Hz - bad for timers...)
											// -	on my PowerCenter 150, the VIA chips appears to go at 783.288 KHz, an error of 0.00919%
											// - the spec says the VIA runs at 783360 Hz
											
// These are the number of cycles to loop for when benchmarking
#define	DECREMENTOR_VIA_CYCLES	65535
#define	BDNZ_LOOPS				10000000
#define	LOAD_LOOPS				1000000

static UInt32		decHz;
static UInt32		loopMultiplier[] = {	0,	// Who knows
									2,	// 601
									0,	// Who knows
									2,	// 603	*** multiplier is a guess
									1	// 604
								};

// Bus benchmark multipliers.  These convert a benchmark vector into a bus speed.  Note that the denominator must NOT exceed 100,000,000, and should
// ALWAYS be a divisor of 1,000,000,000.
static UInt32			bus_mark_num[]	=	{	1,			// Who knows
											1050399204,	// 601	-	10.50399204	(10.5?)
											1,			// 602	-	?
											1,			// 603	-	?
											950009322,	// 604	-	9.50009322	(9.5?)
											1,			// Who knows
											1,			// 603e	-	?
											1,			// 603ev	-	?
											500006000,	// 750	-	5.000060001	(5.0?)
											1,			// 604e	-	?
											1			// 604ev	-	?
										};
static UInt32			bus_mark_denom[]	=	{	1,			// Who knows
											100000000,	// 601
											1,			// 602
											1,			// 603
											100000000,	// 604
											1,			// Who knows
											1,			// 603e
											1,			// 603ev
											100000000,	// 750
											1,			// 604e
											1			// 604ev
										};

void InitTime(void)
{
	machine.viaDevice0.logicalAddr = (VIA_Chip*)NKIOMap(machine.viaDevice0.physicalAddr,machine.viaDevice0.len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	UInt32	deltaTime;
	
	BenchMarkDecrementor(machine.viaDevice0.logicalAddr,&deltaTime,DECREMENTOR_VIA_CYCLES);
	
	decHz = ((UInt64)VIA_CLOCK_SPEED * deltaTime) / (UInt64)(DECREMENTOR_VIA_CYCLES);
	
	// Calculate the ratio between clocks and nanoseconds
	UInt32	num,denom;
	if((_getPVR() & 0xFFFF0000) == 0x00010000)
	{
		// 601 machine - this should come out as 4085/4096
		num = ((4096000000000ULL + ((UInt64)decHz+1)/2ULL)/((UInt64)decHz)); 
		denom = 4096;
	}
	else
	{
		// Non - 601 machine.  Convert the ratio so that it has a numerator of 4000. This makes the
		// ratio look like 4000/d -- where d is actually the bus speed in MHz!
		// Which means we should really use 40000/d or 400000/d making d the bus speed in
		// MHz to 1 or 2 decimals of precision.
		//
		// Umm, now we use 400000/d instead. - The *100 stuff in here tells you how many decimals of
		// precision you get for the bus speed in MHz. (i.e. 10 == 1 decimal, 100 = 2, 1000 = 3, etc.)
		
		// A lesson in math:	-	Let us assume that the bus speed is always a multiple of .01 MHz == 10000 Hz.
		//				-	Let us assume that the decrementor speed is always 1/4 of the bus speed
		//				-	It follows that the decrementor speed is always a multiple of 10000/4 Hz == 2500 Hz.  So we can
		//					safely round off the decrementor frequency to the nearest multiple of 2500.  Usually, this brings
		//					us closer to the real decrementor frequency, rather than farther
		decHz = ROUND(2500,decHz);
		num = 4000*100;
		denom = (decHz*100 + 125000)/250000;
	}
	
	machine.clockRatio.clocksToNS = (Float64)num / (Float64)denom;
	machine.clockRatio.nsToClocks = (Float64)denom / (Float64)num;
	
	UInt32 decMHzTimes10 = (decHz + 50000)/100000;
	UInt32 decMHzDecimal = (decMHzTimes10 % 10);
	UInt32 decMHz = (decMHzTimes10 / 10);
	cout << "\nDecrementor freq: " << decMHz << "." << decMHzDecimal << " MHz (" << decHz << " Hz)\n";
	cout << "Clock-to-ns ratio: " << num << "/" << denom << "\n";
	NKGetThisProcessorInfo()->decHz = decHz;
	
	UInt32	procSpeedHz;
	if((NKGetThisProcessorInfo()->pvr >> 16) <= 4)
	{
		BenchMarkBDNZ(&deltaTime,BDNZ_LOOPS);
		procSpeedHz = ((UInt64)BDNZ_LOOPS * 1000000000ULL / (UInt64)clockToNsec(deltaTime))*loopMultiplier[_getPVR() >> 16];
	}
	else
	{
		UInt32	pll_config = (_getHID1() >> 28);
		procSpeedHz = (4*decHz*pll_num[_getPVR() >> 16][pll_config]/pll_denom[_getPVR() >> 16][pll_config]);
	}
	procSpeedHz = ROUND(10000,procSpeedHz);	// Since we are assuming a processor frequency in a multiple of 0.01 MHz
	UInt32 procSpeedMHzTimes10 = (procSpeedHz + 50000)/100000;
	UInt32 procSpeedMHzDecimal = (procSpeedMHzTimes10 % 10);
	UInt32 procSpeedMHz = (procSpeedMHzTimes10 / 10);
	cout << "Processor freq: " << procSpeedMHz << "." << procSpeedMHzDecimal << " MHz (" << procSpeedHz << " Hz)\n";
	
	NKGetThisProcessorInfo()->hz = procSpeedHz;
	
	UInt32 busSpeedHz = decHz*4;
	UInt32 busSpeedMHzTimes10 = (busSpeedHz + 50000)/100000;
	UInt32 busSpeedMHzDecimal = (busSpeedMHzTimes10 % 10);
	UInt32 busSpeedMHz = (busSpeedMHzTimes10 / 10);
	cout << "Bus speed: " << busSpeedMHz << "." << busSpeedMHzDecimal << " MHz (" << busSpeedHz << " Hz)\n";
	machine.busClock = busSpeedHz;
}

void Wait_ns(Float64 nsec)
{
	Float64		curTime = GetTime_ns(), endTime = curTime+nsec;
	while( GetTime_ns() < endTime )
		;
}

void Wait_us(Float64 usec)
{
	Wait_ns(usec*1000.);
}

void Wait_ms(Float64 msec)
{
	Wait_ns(msec*1000000.);
}

void Wait_s(Float64 sec)
{
	Wait_ns(sec*1000000000.);
}

// CW translates the following *SOOOOOOOOOO* badly, even with full optimizations on, it's worth asmifying!
// For example, for right-shifting a 64-bit number by 32 bits, it uses a runtime function called __shr2u, instead
// of just taking the upper half!!!
// In any case, I had already asmified this for an early version of GetTime_ns()
#if 0
UInt64 GetClock(void)
{
	UInt64 time = _getClock();
	if( (NKGetThisProcessorInfo()->pvr & 0xFFFF0000) == 0x00010000)
	{
		// 601 processor is a bit different.  Upper measures seconds and lower measures nano-seconds
		time = ( ((time & 0xFFFFFFFF00000000) >> 32) * 1000000000ULL) + (time & 0x00000000FFFFFFFF);
	}
	
	return time;
}
#else
__asm__ UInt64 GetClock(void)
{
	// Get current clock into r3-r4
	mfpvr			(r3);
	srwi				(r3,r3,16);
	cmpwi			r3,1;
	beq				@RTCTime;
	
@TBTime:
	mfspr			r3,TBU_R;
	mfspr			r4,TBL_R;
	mfspr			r5,TBU_R;
	cmpw			r3,r5;
	beqlr+;
	b				@TBTime;
	
@RTCTime:
	mfspr			r5,RTCU_R;
	mfspr			r6,RTCL_R;
	mfspr			r7,RTCU_R;
	cmpw			r5,r7;
	bne				@RTCTime;
	
	// Convert RTC time into real clock ticks.  RTC stores seconds in upper and ns in lower.  Thus multiply upper by 1*10^9 (to convert to ns), this being
	// 0x3B9ACA00 in hex, and add to lower
	li				r0,0;
	lis				r7,0x3B9A;
	ori				r7,r7,0xCA00
	mullw			r4,r5,r7;
	mulhwu			r3,r5,r7;
	addc				r4,r4,r6;
	adde				r3,r3,r0;
	
	blr
}
#endif