#ifndef __SCREEN_SAVER__
#define __SCREEN_SAVER__

class ScreenSaver
{
	UInt8**	background;
	Rect		saveRect;
public:
	ScreenSaver(Rect saveRect);
	~ScreenSaver();
};

#endif /* __SCREEN_SAVER__ */