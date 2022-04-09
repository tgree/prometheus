/*
	Benchmarks.h
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
	Terry Greeniaus	-	Monday, 3 August 98	-	Original creation of file
*/
#ifndef __BENCHMARRKS__
#define __BENCHMARRKS__

void	BenchMarkDecrementor(register struct VIA_Chip* via,register UInt32* deltaTime,register UInt16 numViaClocks);
void	BenchMarkBDNZ(register UInt32* deltaTime,register UInt32 numBranches);
void	BenchMarkSystemBus(register UInt32* deltaTime,register void* cacheInhibitedWordAddr,register UInt32 numLoads);

#endif
