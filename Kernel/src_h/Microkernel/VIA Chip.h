/*
	VIA Chip.h
	Copyright © 1998 by Patrick Varilly

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
	pmu_defs.h			MkLinux DR3 alpha 4		???				Interrupt status bits taken from here.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Sunday, 21 June 98	-	Made all the rsrv fields const, also added
											511 byte buffer at the end for stacked chips.
											Also added interrupt status bits from pmu_defs.h
*/
#ifndef __VIA__CHIP__
#define __VIA__CHIP__

enum {					// IFR/IER bits
	ifCA2	= 			0, 	// CA2 interrupt
	ifCA1	= 			1, 	// CA1 interrupt
	ifSR		= 			2, 	// SR shift register done
	ifCB2	= 			3, 	// CB2 interrupt
	ifCB1	= 			4, 	// CB1 interrupt
	ifT2		= 			5, 	// T2 timer2 interrupt
	ifT1		= 			6, 	// T1 timer1 interrupt
	ifIRQ		= 			7 	// any interrupt
};

typedef struct VIA_Chip
{
	UReg8		dataB;			// Offset 0
	const UInt8	rsrv1[511];
	UReg8		handshakeDataA;	// Offset 0x0200
	const UInt8	rsrv2[511];
	UReg8		dataDirectionB;		// Offset 0x0400
	const UInt8	rsrv3[511];
	UReg8		dataDirectionA;		// Offset 0x0600
	const UInt8	rsrv4[511];
	UReg8		timer1CounterLow;	// Offset 0x0800
	const UInt8	rsrv5[511];
	UReg8		timer1CounterHigh;	// Offset 0x0A00
	const UInt8	rsrv6[511];
	UReg8		timer1LatchLow;	// Offset 0x0C00
	const UInt8	rsrv7[511];
	UReg8		timer1LatchHigh;	// Offset 0x0E00
	const UInt8	rsrv8[511];
	UReg8		timer2CounterLow;	// Offset 0x1000
	const UInt8	rsrv9[511];
	UReg8		timer2CounterHigh;	// Offset 0x1200
	const UInt8	rsrv10[511];
	UReg8		shift;			// Offset 0x1400
	const UInt8	rsrv11[511];
	UReg8		auxillaryControl;	// Offset 0x1600
	const UInt8	rsrv12[511];
	UReg8		peripheralControl;	// Offset 0x1800
	const UInt8	rsrv13[511];
	UReg8		interruptFlag;		// Offset 0x1A00
	const UInt8	rsrv14[511];
	UReg8		interruptEnable;	// Offset 0x1C00
	const UInt8	rsrv15[511];
	UReg8		dataA;			// Offset 0x1E00
	const UInt8	rsrv16[511];
} VIA_Chip;

#endif /* !__VIA__CHIP__ */