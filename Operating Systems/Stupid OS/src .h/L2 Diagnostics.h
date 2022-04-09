#ifndef __L2_DIAGNOSTICS__
#define __L2_DIAGNOSTICS__

#include "Kernel Console.h"
#include "ADBMouse.h"
#include "NKThreads.h"

class L2DiagnosticsConsole	:	public VideoConsoleStream,
							public Thread
{
	Int32	posX;
	Int32	posY;
	Ptr		expectedCacheContents;
	Ptr		currCacheContents;
	UInt8*	flags;
	UInt32	numBlocks;
	
	L2DiagnosticsConsole(Ptr expectedCacheContents,Ptr currCacheContents,UInt8* flags,UInt32 numBlocks);
	
	// Stuff for Thread
	virtual	void		threadLoop();
	virtual	Boolean	threadQueryCanRun();
	
	// My own stuff
			void		draw();
			
	friend void L2Diagnostics();
};

void L2Diagnostics();

#endif /* __L2_DIAGNOSTICS__ */