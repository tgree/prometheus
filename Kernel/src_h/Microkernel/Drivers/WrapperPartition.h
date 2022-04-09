/*
	WrapperPartition.h
	Copyright © 1999 by Patrick Varilly

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
	Patrick Varilly		-	Monday, 29 March 99	-	Creation of file
*/

#ifndef __WRAPPER_PARTITION__
#define __WRAPPER_PARTITION__

#include "Block Device.h"

class WrapperPartition : public BlockDevicePartition
{
protected:
	BlockDevicePartition		*partition;
	
public:
						WrapperPartition(BlockDevicePartition* partition, UInt32 firstSector, UInt32 numSectors);
	virtual				~WrapperPartition();
	
	virtual	IOCommand*	readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);
	virtual	IOCommand*	writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);
};

#endif __WRAPPER_PARTITION__