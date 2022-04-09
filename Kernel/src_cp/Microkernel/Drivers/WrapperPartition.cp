/*
	WrapperPartition.cp
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

#include "WrapperPartition.h"

WrapperPartition::WrapperPartition(BlockDevicePartition* partition, UInt32 firstSector, UInt32 numSectors):
	BlockDevicePartition(firstSector,numSectors,partition->myDevice)
{
	this->partition = partition;
}

WrapperPartition::~WrapperPartition()
{
}

IOCommand*
WrapperPartition::readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors)
{
	if( (sector + numSectors) > this->numSectors )
		return nil;
	return partition->readSectorsAsync( p, sector+firstSector, numSectors );
}

IOCommand*
WrapperPartition::writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors)
{
	if( (sector + numSectors) > this->numSectors )
		return nil;
	return partition->writeSectorsAsync( p, sector+firstSector, numSectors );
}