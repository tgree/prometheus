/*
	OGRE.h
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
	Terry Greeniaus	-	Wednesday, 17 June 98	-	Have fun!
*/
#ifndef __OGRE__
#define __OGRE__

#include "Kernel Console.h"

#define NUM_TREES	50
#define NUM_OGRES	10

#define PLAYER_CHAR	"*"
#define TREE_CHAR		"O"
#define OGRE_CHAR		"#"

#define CONSOLE_WIDTH	50
#define CONSOLE_HEIGHT	30
#define CONSOLE_X		10
#define CONSOLE_Y		10

class OGREConsole	:	public VideoConsoleStream
{
	UInt32		treeX[NUM_TREES];
	UInt32		treeY[NUM_TREES];
	
	Boolean		ogreAlive[NUM_OGRES];
	UInt32		ogreX[NUM_OGRES];
	UInt32		ogreY[NUM_OGRES];
	
	UInt32		playerX;
	UInt32		playerY;
	
	UInt32		numOgres;
	Boolean		gameRunning;
	
	void		movePlayer(Int32 deltaX,Int32 deltaY);
	void		moveOgres();
	Boolean	treeCollision(UInt32 x,UInt32 y);	// Returns the tree number+1
	Boolean	ogreCollision(UInt32 x,UInt32 y);	// Returns the tree number+1
	void		gameOver(ConstASCII8Str msg);
	void		moveToChar(UInt32 x,UInt32 y);
public:
	OGREConsole(const Rect* rect);
	
	void		startGame();
};

void PlayOgre(void);

#endif /* __OGRE__ */
