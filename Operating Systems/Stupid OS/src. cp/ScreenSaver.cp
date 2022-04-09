#include "ScreenSaver.h"
#include "Video Driver.h"
#include "Memory Utils.h"

ScreenSaver::ScreenSaver(Rect _saveRect)
{
	saveRect = _saveRect;
	
	UInt32	rectHeight = saveRect.y2 - saveRect.y1;
	UInt32	rectWidth = saveRect.x2 - saveRect.x1;
	background = new (UInt8*[rectHeight]);
	FatalAssert(background != nil);
	for(UInt32 y=0;y<rectHeight;y++)
	{
		background[y] = new UInt8[rectWidth];
		FatalAssert(background[y] != nil);
	}
	
	for(UInt32 y=0;y<rectHeight;y++)
		MemCopy(video->logicalAddr() + (y + saveRect.y1)*video->rowBytes() + saveRect.x1,background[y],rectWidth);
}

ScreenSaver::~ScreenSaver()
{
	UInt32	rectHeight = saveRect.y2 - saveRect.y1;
	UInt32	rectWidth = saveRect.x2 - saveRect.x1;
	
	for(UInt32 y=0;y<rectHeight;y++)
	{
		MemCopy(background[y],video->logicalAddr() + (y + saveRect.y1)*video->rowBytes() + saveRect.x1,rectWidth);
		delete [] background[y];
	}
	delete [] background;
}