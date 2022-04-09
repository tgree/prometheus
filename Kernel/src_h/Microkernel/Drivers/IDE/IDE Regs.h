/*
	IDE Regs.h
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
	Terry Greeniaus	-	Wednesday, 17 June 98	-	Original creation of file
*/
#ifndef __IDE_REGS__
#define __IDE_REGS__

#error Don't include this file anymore.  Use the new IDE architecture.

// This is used in PowerBooks (5300, Duo 2300)
typedef struct IDERegs32
{
	// Offset 0
	volatile UInt32		data;
	
	// Offset 4
	union
	{
		UReg8	error_read;
		UReg8	features_write;
	};
	const UInt8		rsrv1[3];
	
	// Offset 8
	UReg8				sectorCount;
	const UInt8			rsrv2[3];
	
	// Offset 12
	UReg8				sector;
	const UInt8			rsrv3[3];
	
	// Offset 16
	UReg8				cylinderLow;
	const UInt8			rsrv4[3];
	
	// Offset 20
	UReg8				cylinderHigh;
	const UInt8			rsrv5[3];
	
	// Offset 24
	UReg8				head;
	const UInt8			rsrv6[3];
	
	// Offset 28
	union
	{
		UReg8	status_read;
		UReg8	command_write;
	};
	const UInt8			rsrv7[3];
	
	// Offset 32
	const UInt8			rsrv8[24];
	
	// Offset 56
	union
	{
		UReg8	alternateStatus_read;
		UReg8	deviceControl_write;
	};
}IDERegs32;

// This is used in PC9801 - whatever that is...
typedef struct IDERegsPC9801
{
	// Offset 0
	UReg16BE				data;
	
	// Offset 2
	union
	{
		UReg8	error_read;
		UReg8	features_write;
	};
	
	// Offset 3
	UReg8				alternateStatus_epson;
	
	// Offset 4
	UReg8				sectorCount;
	const UInt8			rsrv2;
	
	// Offset 6
	UReg8				sector;
	const UInt8			rsrv3;
	
	// Offset 8
	UReg8				cylinderLow;
	const UInt8			rsrv4;
	
	// Offset 10
	UReg8				cylinderHigh;
	const UInt8			rsrv5;
	
	// Offset 12
	UReg8				head;
	const UInt8			rsrv6;
	
	// Offset 14
	union
	{
		UReg8	status_read;
		UReg8	command_write;
	};
	const UInt8			rsrv7;
	
	// Offset 16
	const UInt8			rsrv8[252];
	
	// Offset 268
	union{
		UReg8	alternateStatus_read;
		UReg8	deviceControl_write;
	};
}IDERegsPC9801;

#endif /* __IDE_REGS__ */