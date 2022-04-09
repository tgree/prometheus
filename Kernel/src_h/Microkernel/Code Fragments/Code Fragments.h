/*
	Code Fragments.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __CODE_FRAGMENTS__
#define __CODE_FRAGMENTS__

class CodeFragment
{
	class CodeFragment*	next;	// The next CodeFragment
protected:
	class FileDescriptor*	fileDesc;
	ASCII8Str				fragName;	// The name of this fragment
	
	CodeFragment(ConstASCII8Str name,FileDescriptor* fileDesc);
public:
	virtual ~CodeFragment();
	
	virtual	void*			getSymAddr(ConstASCII8Str symName) = 0;	// Gets the specified symbol address
	virtual	void				main(UInt32 arg1) = 0;			// Calls the main() routine if the fragment has one, does nothing otherwise.  arg1 gets passed to main()
			ConstASCII8Str	name();
	static	CodeFragment*		getFragment(ConstASCII8Str fragName);
};

void InitCodeFragments();

#endif /* __CODE_FRAGMENTS__ */