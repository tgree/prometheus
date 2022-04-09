/*
	OGRE.cp
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
#include "SOS.h"
#include "NKVideo.h"
#include "Video Driver.h"
#include "OGRE.h"
#include "Gonzales.h"
#include "ANSI.h"
#include "Memory Utils.h"

UInt8** background = nil;

void PlayOgre()
{
	cout << "OGRE... brought to you by Pandora!\n";
	
	UInt32 consoleWidth = (CONSOLE_WIDTH + 1)*gonzales5->width();
	UInt32 consoleHeight = (CONSOLE_HEIGHT + 1)*gonzales5->height();
	
	background = new (UInt8*[consoleHeight+2]);
	for(UInt32 y = 0;y < consoleHeight + 2;y++)
		background[y] = new UInt8[consoleWidth + 2];
	
	for(Int32 y = 0;y < consoleHeight + 2;y++)
		MemCopy(video->logicalAddr() + (y + CONSOLE_Y - 1)*video->rowBytes() + CONSOLE_X - 1,background[y],consoleWidth + 2);
	
	Rect	consoleRect = {CONSOLE_X - 1,CONSOLE_Y - 1,CONSOLE_X + consoleWidth + 1,CONSOLE_Y + consoleHeight + 1};
	NKBlackOutRect(&consoleRect,white8bit);
	
	OGREConsole	ogreConsole(&consoleRect);
	ogreConsole.startGame();
	
	for(Int32 y = 0;y < consoleHeight + 2;y++)
		MemCopy(background[y],video->logicalAddr() + (y + CONSOLE_Y - 1)*video->rowBytes() + CONSOLE_X - 1,consoleWidth + 2);
	
	for(UInt32 y = 0;y < consoleHeight + 2;y++)
		delete [] background[y];
	delete [] background;
}

OGREConsole::OGREConsole(const Rect* rect):
	VideoConsoleStream(rect,false,gonzales5)
{
	foreColor(green8bit);
	for(Int32 i=0;i < NUM_TREES;i++)
	{
		treeX[i] = (rand() % CONSOLE_WIDTH);
		treeY[i] = (rand() % CONSOLE_HEIGHT);
		moveToChar(treeX[i],treeY[i]);
		write(TREE_CHAR,1);
	}
	
	numOgres = NUM_OGRES;
	for(Int32 i=0;i < NUM_OGRES;i++)
		ogreAlive[i] = false;
	foreColor(red8bit);
	for(Int32 i=0;i < NUM_OGRES;i++)
	{
		do
		{
			ogreX[i] = (rand() % CONSOLE_WIDTH);
			ogreY[i] = (rand() % CONSOLE_HEIGHT);
		}while(treeCollision(ogreX[i],ogreY[i]) || ogreCollision(ogreX[i],ogreY[i]));
		ogreAlive[i] = true;
		moveToChar(ogreX[i],ogreY[i]);
		write(OGRE_CHAR,1);
	}
	
	foreColor(white8bit);
	do
	{
		playerX = (rand() % CONSOLE_WIDTH);
		playerY = (rand() % CONSOLE_HEIGHT);
	}while(treeCollision(playerX,playerY) || ogreCollision(playerX,playerY));
	moveToChar(playerX,playerY);
	write(PLAYER_CHAR,1);
	
	gameRunning = false;
}

void OGREConsole::movePlayer(Int32 deltaX,Int32 deltaY)
{
	moveToChar(playerX,playerY);
	write(" ",1);
	
	playerX += deltaX;
	playerY += deltaY;
	if(playerX < 0 || playerX >= CONSOLE_WIDTH || playerY < 0 || playerY >= CONSOLE_HEIGHT)
		gameOver("You fell off the earth!\n");
	else if(treeCollision(playerX,playerY))
		gameOver("You walked into a poisonous tree!\n");
	else
	{
		moveToChar(playerX,playerY);
		write(PLAYER_CHAR,1);
	}
}

void OGREConsole::moveOgres()
{
	for(Int32 i=0;i<NUM_OGRES;i++)
	{
		if(ogreAlive[i])
		{
			moveToChar(ogreX[i],ogreY[i]);
			write(" ",1);
			
			UInt32 newX = ogreX[i];
			UInt32 newY = ogreY[i];
			
			if(playerX < ogreX[i])
				newX--;
			else if(playerX > ogreX[i])
				newX++;
			
			if(playerY < ogreY[i])
				newY--;
			else if(playerY > ogreY[i])
				newY++;
			
			if(newX == playerX && newY == playerY)
				gameOver("An ogre got you!\n");
			else if(treeCollision(newX,newY))
			{
				ogreAlive[i] = false;
				numOgres--;
			}
			else if(ogreCollision(newX,newY))
			{
				ogreAlive[i] = false;
				for(Int32 j = 0;j < NUM_OGRES;j++)
				{
					if(newX == ogreX[j] && newY == ogreY[j])
						ogreAlive[j] = false;
				}
				numOgres -= 2;
				moveToChar(newX,newY);
				write(" ",1);
			}
			else
			{
				ogreX[i] = newX;
				ogreY[i] = newY;
				moveToChar(ogreX[i],ogreY[i]);
				foreColor(red8bit);
				write(OGRE_CHAR,1);
				foreColor(white8bit);
			}
		}
	}
	
	if(!numOgres)
		gameOver("You won!\n");
}

Boolean OGREConsole::treeCollision(UInt32 x,UInt32 y)
{
	for(Int32 i=0;i<NUM_TREES;i++)
	{
		if(treeX[i] == x && treeY[i] == y)
			return true;
	}
	
	return false;
}

Boolean OGREConsole::ogreCollision(UInt32 x,UInt32 y)
{
	for(Int32 i=0;i<NUM_OGRES;i++)
	{
		if(ogreX[i] == x && ogreY[i] == y && ogreAlive[i])
			return true;
	}
	
	return false;
}

void OGREConsole::gameOver(ConstASCII8Str msg)
{
	fill(black8bit);
	moveToChar(10,10);
	*this << msg << "\nPress any key to continue...";
	
	ASCII8	theChar;
	cin >> theChar;
	
	gameRunning = false;
}

void OGREConsole::moveToChar(UInt32 x,UInt32 y)
{
	moveTo(x*gonzales5->width(),y*gonzales5->height());
}

void OGREConsole::startGame()
{
	gameRunning = true;
	while(gameRunning)
	{
		ASCII8	theChar;
		Boolean	goodMove = true;
		
		cin >> theChar;
		switch(theChar)
		{
			case '8':
			case 'k':	movePlayer(0,-1);	break;
			case '9':	movePlayer(1,-1);	break;
			case '6':
			case 'l':	movePlayer(1,0);	break;
			case '3':	movePlayer(1,1);	break;
			case '2':
			case 'j':	movePlayer(0,1);	break;
			case '1':	movePlayer(-1,1);	break;
			case '4':
			case 'h':	movePlayer(-1,0);	break;
			case '7':	movePlayer(-1,-1);	break;
			case '5':
			case ' ':					break;
			default:	goodMove = false;	break;
		}
		
		if(goodMove)
			moveOgres();
	}
}
