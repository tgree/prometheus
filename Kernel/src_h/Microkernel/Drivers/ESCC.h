/*
	ESCC.h
	Copyright ? 1998 by Terry Greeniaus

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
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __ESCC__
#define __ESCC__

#include "Streams.h"

enum
{
	channelA	=	1,		// Modem port
	channelB	=	0		// Printer port
};

typedef struct esccChannel
{
	UReg8		command;
	const UInt8	rsrv[15];
	UReg8		data;
	const UInt8	rsrv1[15];
}esccChannel;

typedef struct esccDevice
{
	esccChannel	channelB;
	esccChannel	channelA;
}esccDevice;

extern IOStreamWrapper	modemPort;
extern IOStreamWrapper	printerPort;

void		InitESCC(void);
UInt16	BaudRateToTimeConstant(UInt16 baudRate);

#endif /* __ESCC__ */