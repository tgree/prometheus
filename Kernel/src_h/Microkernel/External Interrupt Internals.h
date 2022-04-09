/*
	Driver.h
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
	???
	
	Version History
	============
	Patrick Varilly			January, 1998			Original creation of file
	Patrick Varilly			Wed, 21 Jan 98			Original history tagging of file. Total restructing of interrupt
		---> system. Note that this change is completely transparent to outside software (except for perhaps a minor
		---> performance improvement).
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
/*
	External Interrupt Internals.h
	
	Functions valuable only to the guts of the interrupt system
	
	Copyright © 1997-1998 by The Pandora Team. All rights reserved worldwide.
	Permission to use and modify this file is given solely to the Pandora Team until further notice
*/
#ifndef __EXTERNAL__INTERRUPT__INTERNALS__
#define __EXTERNAL__INTERRUPT__INTERNALS__

#include "NKInterruptVectors.h"
#include "External Interrupt.h"

// A table that matches an interrupt type with its corresponding InterruptHandler
typedef struct
{
	InterruptHandler*	handler;
	Int32			type;
} HandlerTable;

// A small wrapper class that lets a function be called when an interrupt happens
typedef void (*WrapperFunction)(void);

class WrapperInterruptHandler : public InterruptHandler
{
private:
	WrapperFunction		wrapper;
public:
	WrapperInterruptHandler( WrapperFunction wrapper ) : InterruptHandler(-1) { this->wrapper = wrapper; }
	
	virtual void handleInterrupt( void ) { (*wrapper)(); }
};

ASCII8Str getInterruptName( Int32 interruptType );
Boolean searchHandlerTable( HandlerTable *table, Int32 type, UInt32 numEntries, UInt32* entryNum );
Boolean addEntryToHandlerTable( HandlerTable *table, Int32 interruptType, UInt32 entryNum, InterruptHandler *handler );

#endif /* __EXTERNAL__INTERRUPT__INTERNALS__ */