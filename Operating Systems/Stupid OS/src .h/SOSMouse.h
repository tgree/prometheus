#ifndef __SOS_MOUSE__
#define __SOS_MOUSE__

#include "ADBMouse.h"
#include "NKVideo.h"

class SOSMouse : public MouseHandler
{
protected:
	Int32		posX,posY;
	Int32		screenTop,screenLeft,screenBottom,screenRight;
	UInt32		shielded;
	Boolean		mainButtonPressed, firstTime;
	Boolean		unshielding;
	UInt8		background[8];
					
	virtual void		mouseMoved( Int8 deltaX, Int8 deltaY );		// Called when mouse is moved
	virtual void		mousePosSet( Int32 newX, Int32 newY );		// Called when mouse position is changed (can happen internally or with, say, a graphics tablet?)
	virtual void		mouseClicked( UInt8 buttonNum, Boolean clicked );	// Called when a mouse button is pressed
	
	void				drawMouse();
	void				eraseMouse();
	void				saveBackground();
public:
					SOSMouse();
	virtual ~SOSMouse();
	
	virtual	void		shieldMouse();								// Called when the mouse should be shielded for drawing
	virtual	void		unshieldMouse();							// Called when the mouse should be unshielded for drawing
			Int32	getX() {return posX;}
			Int32	getY() {return posY;}
};

extern SOSMouse*	sosMouse;

void InitMouseHandler(void);

#endif /* __SOS_MOUSE__ */