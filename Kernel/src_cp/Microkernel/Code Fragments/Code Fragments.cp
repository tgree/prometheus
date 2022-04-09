/*
	Code Fragments.cp
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
#include "NKAtomicOperations.h"
#include "NKMachineInit.h"
#include "ANSI.h"
#include "Code Fragments.h"
#include "PEF.h"

void InitCodeFragments()
{
	machine.pefFragment = new PEFFragment(machine.pefLoaderInfo,(Int8*)machine.codeStart,(Int8*)machine.dataStart);
}

CodeFragment::CodeFragment(ConstASCII8Str _name,FileDescriptor* fd)
{
	fileDesc = fd;
	fragName = new ASCII8[strlen(_name)+1];
	fragName[0] = 0;
	strcat(fragName,_name);
	
	Process*		process = CurrProcess::process();
	SpinLocker	locker(process->codeFragListLock);
	next = process->codeFragList;
	process->codeFragList = this;
}

CodeFragment::~CodeFragment()
{
	Process*		process = CurrProcess::process();
	SpinLocker	locker(process->codeFragListLock);
	CodeFragment*	currFragment = process->codeFragList;
	CodeFragment*	prev = nil;
	
	while(currFragment)
	{
		if(currFragment == this)
		{
			if(prev)
				prev->next = next;
			else
				process->codeFragList = next;
			break;
		}
		prev = currFragment;
		currFragment = currFragment->next;
	}
	
	delete [] fragName;
}

ConstASCII8Str CodeFragment::name()
{
	return fragName;
}

CodeFragment* CodeFragment::getFragment(ConstASCII8Str fragName)
{
	if(!strcmp(fragName,"Kernel"))
		return machine.pefFragment;
	
	Process*		process = CurrProcess::process();
	SpinLocker	lock(process->codeFragListLock);
	CodeFragment*	currFragment = process->codeFragList;
	
	while(currFragment)
	{
		if(!strcmp(fragName,currFragment->name()))
			return currFragment;
		currFragment = currFragment->next;
	}
	
	return nil;
}
