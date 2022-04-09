/*
	OSs.cp
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
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "NKProcesses.h"
#include "ANSI.h"
#include "Kernel Console.h"
#include "PEF.h"
#include "OSs.h"
#include "File Systems.h"
#include "NKThreads.h"
#include "Login.h"

struct OSThread	:	Thread
{
	CodeFragment*	osFrag;
	User*		osUser;
	
	OSThread(CodeFragment* frag,Process* parent,User* osUser);
	
	virtual	void	threadLoop();
};

UInt32 InitOSs(User* theUser)
{
	Boolean			selectiveLoad = theUser->propertyExists("operating-systems");
	if(selectiveLoad)
		if(!theUser->numValues("operating-systems"))
			return 0;
	
	DirectoryDescriptor*	osParDir = kernelDirectory->parent();
	DirectoryDescriptor*	osDir = osParDir->subDir("Operating Systems");
	delete osParDir;
	if(osDir)
	{
		FileIterator*	iterator = osDir->newFileIterator();
		for(;*iterator;iterator->advance())
		{
			if(!strncmp("OS_",(*iterator)->name(),3))
			{
				Boolean	canLoad = true;
				if(selectiveLoad)
				{
					UInt32 numValues = theUser->numValues("operating-systems");
					canLoad = false;
					for(UInt32 i=0;i<numValues;i++)
					{
						if(!strcmp((*iterator)->name(),theUser->getValue("operating-systems",i)))
						{
							canLoad = true;
							break;
						}
					}
				}
				if(canLoad)
				{
					theUser->numOSs++;
					theUser->console << "Loading OS: \"" << (*iterator)->name() << "\"" << newLine;
					FileIStream*	iStream = (*iterator)->openForRead();
					CodeFragment* osFrag;
					Process* osProcess = new Process(0,nil);
					{
						ProcessWindow	window(osProcess);
						osFrag = new PEFFragment(*iStream);
					}
					theUser->osProcess = osProcess;
					delete iStream;
					
					theUser->console << "Creating/starting OS thread" << newLine;
					OSThread*	temp;
					temp = new OSThread(osFrag,osProcess,theUser);
					temp->resume();
				}
			}
		}
		
		delete iterator;
		delete osDir;
	}
	else
		theUser->console << "Directory \"Operating Systems\" doesn't exist!" << newLine;
	
	return theUser->numOSs;
}

OSThread::OSThread(CodeFragment* frag,Process* parent,User* _osUser):
	Thread(65*1024,1,parent,"OS Main Thread")
{
	osFrag = frag;
	osUser = _osUser;
}

void OSThread::threadLoop()
{
	osFrag->main(reinterpret_cast<UInt32>(osUser));
}