/*
	ShutDown.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Sunday, 21 June 98	-	Added variable to shutDown() stating if it is
											a ShutDown() or a Restart()
*/
#ifndef __SHUTDOWN__
#define __SHUTDOWN__

class ShutDownHandler
{
	class ShutDownHandler*	next;
	
	static	void	handleShutDown(Boolean isShutDown);
public:
	ShutDownHandler();
	virtual	void	shutDown(Boolean isShutDown) = 0;
	
	friend void ShutDown();
	friend void Restart();
};

#endif