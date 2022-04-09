/*
	SOSMouse.cp
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
	none
	
	Version History
	============
	Terry Greeniaus	-	Tuesday, 16 June 98	-	Made a real mouse out of it.
*/
#include "SOSMouse.h"
#include "Video Driver.h"
#include "Macros.h"

SOSMouse*	sosMouse;

void InitMouseHandler(void)
{
	sosMouse = new SOSMouse;		// This will start the mouse tracking automatically
}

SOSMouse::SOSMouse()
	: posX(0), posY(0), mainButtonPressed(false), firstTime(true), shielded(0), unshielding(false)
{
	// Initialize the mouse console and mouse bounds
	screenTop = screenLeft = 0;
	screenBottom = screenTop + video->height();
	screenRight = screenLeft + video->width();
	
	saveBackground();
	mousePosSet(0,0);
}

SOSMouse::~SOSMouse()
{
	shieldMouse();
}

void
SOSMouse::mouseMoved( Int8 deltaX, Int8 deltaY )
{
	/*
	// Don't move the cursor if we are unshielding
	if(unshielding)
		return;
	*/
	mousePosSet( posX+deltaX, posY+deltaY );	// This takes care of making sure the mouse doesn't go offscreen
}

void
SOSMouse::mousePosSet( Int32 newX, Int32 newY )
{
	/*
	if(unshielding)
		return;
	*/
	
	if(!shielded)
		eraseMouse();
	
	posX = newX;
	posY = newY;
	
	if( posX < screenLeft )
		posX = screenLeft;
	if( posX >= screenRight )
		posX = screenRight - 1;
	
	if( posY < screenTop )
		posY = screenTop;
	if( posY >= screenBottom )
		posY = screenBottom - 1;
	
	if(!shielded)
	{
		saveBackground();
		drawMouse();
	}
}

void
SOSMouse::mouseClicked( UInt8 buttonNum, Boolean clicked )
{
	if( buttonNum == 0 )
	{
		mainButtonPressed = clicked;
		if(!shielded && !unshielding)
			drawMouse();
	}
}

void
SOSMouse::shieldMouse()
{
	//unshielding = true;
	if(!shielded++)
		eraseMouse();
	//unshielding = false;
}

void
SOSMouse::unshieldMouse()
{
	FatalAssert(shielded > 0);
	//unshielding = true;
	if(!--shielded)
	{
		saveBackground();
		drawMouse();
	}
	//unshielding = false;
}

void
SOSMouse::drawMouse(void)
{
	UInt8 color = (mainButtonPressed ? green8bit : red8bit);
	
	// Top line
	if(posY >= 2)
		video->setPixel8(posX,posY-2,color);
	if(posY >= 1)
		video->setPixel8(posX,posY-1,color);
	
	// Left line
	if(posX >= 2)
		video->setPixel8(posX-2,posY,color);
	if(posX >= 1)
		video->setPixel8(posX-1,posY,color);
	
	// Right line
	if(posX < screenRight - 2)
		video->setPixel8(posX+1,posY,color);
	if(posX < screenRight - 3)
		video->setPixel8(posX+2,posY,color);
	
	// Bottom line
	if(posY < screenBottom - 2)
		video->setPixel8(posX,posY+2,color);
	if(posY < screenBottom - 1)
		video->setPixel8(posX,posY+1,color);
}

void
SOSMouse::eraseMouse(void)
{
	// Top line
	if(posY >= 2)
		video->setPixel8(posX,posY-2,background[0]);
	if(posY >= 1)
		video->setPixel8(posX,posY-1,background[1]);
	
	// Left line
	if(posX >= 2)
		video->setPixel8(posX-2,posY,background[2]);
	if(posX >= 1)
		video->setPixel8(posX-1,posY,background[3]);
	
	// Right line
	if(posX < screenRight - 2)
		video->setPixel8(posX+1,posY,background[4]);
	if(posX < screenRight - 3)
		video->setPixel8(posX+2,posY,background[5]);
	
	// Bottom line
	if(posY < screenBottom - 2)
		video->setPixel8(posX,posY+2,background[6]);
	if(posY < screenBottom - 1)
		video->setPixel8(posX,posY+1,background[7]);
}

void
SOSMouse::saveBackground(void)
{
	// Top line
	if(posY >= 2)
		background[0] = video->getPixel8(posX,posY-2);
	if(posY >= 1)
		background[1] = video->getPixel8(posX,posY-1);
	
	// Left line
	if(posX >= 2)
		background[2] = video->getPixel8(posX-2,posY);
	if(posX >= 1)
		background[3] = video->getPixel8(posX-1,posY);
	
	// Right line
	if(posX < screenRight - 2)
		background[4] = video->getPixel8(posX+1,posY);
	if(posX < screenRight - 3)
		background[5] = video->getPixel8(posX+2,posY);
	
	// Bottom line
	if(posY < screenBottom - 2)
		background[6] = video->getPixel8(posX,posY+2);
	if(posY < screenBottom - 1)
		background[7] = video->getPixel8(posX,posY+1);
}
