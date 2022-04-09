/*
	ShutDown.cp
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Monday, 1 Feb 2000	-	Changed to use SpinLocks instead of MutexLocks, since ShutDownHandlers
											can be created before threads are enabled.
*/
#include "ShutDown.h"
#include "NKAtomicOperations.h"
#include "NKThreads.h"

static SpinLock			shutDownLock;
static ShutDownHandler*	shutDownList;

ShutDownHandler::ShutDownHandler()
{
	SpinLocker	shutDownLocker(shutDownLock);
	next = shutDownList;
	shutDownList = this;
}

void ShutDownHandler::handleShutDown(Boolean isShutDown)
{
	SpinLocker		shutDownLocker(shutDownLock);
	ShutDownHandler*	handler = shutDownList;
	
	while(handler)
	{
		ShutDownHandler*	next = handler->next;	// In case handler->shutDown() messes up the handler object for some reason...
		handler->shutDown(isShutDown);
		handler = next;
	}
}
