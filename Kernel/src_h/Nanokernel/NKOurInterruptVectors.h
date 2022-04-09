/*
	NKOurInterruptVectors.h
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
	Patrick Varilly		-	Sunday, 28 March 99	-	Created file
*/
#ifndef __NKOUR_INTERRUPT_VECTORS__
#define __NKOUR_INTERRUPT_VECTORS__

void	NKOurInterruptVectorsInit(void);
void	NKOurInterruptVectorsInitOnThisProcessor(void);	// Call this ONCE per processor to initialize our vectors on that processor

#endif /* __NKOUR_INTERRUPT_VECTORS__ */
