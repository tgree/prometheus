/*
	Login.h
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
	Terry Greeniaus	-	Friday, 7 August 98	-	Original creation of file
*/
#ifndef __LOGIN__
#define __LOGIN__

#include "NKMemoryManager.h"
#include "NKThreads.h"
#include "Kernel Console.h"

class User	:	public KernelObject
{
	class User*			nextUser;
	struct UserProperty*	properties;
	ASCII8Str				userName;
	Thread*				loginThread;
protected:
	User(ConstASCII8Str userName,Console& userConsole,UInt32 loginLocation);
	~User();
	
	UserProperty*	addProperty(ConstASCII8Str propertyName,UInt32 numValues);
	UserProperty*	getProperty(ConstASCII8Str propertyName);
public:
	Console&				console;
	Process*				osProcess;
	UInt32				numOSs;
	UInt32				loginLocation;
	
	Boolean			propertyExists(ConstASCII8Str propertyName);	// Returns true if the specified property exists
	UInt32			numValues(ConstASCII8Str propertyName);		// Returns the number of values in the specified property
	ConstASCII8Str		getValue(ConstASCII8Str propertyName,UInt32 n);	// Returns the nth value of the specified property
	ConstASCII8Str		name();									// Returns the user's name
	
	friend static User*	ParseUserFile(class FileIStream* s,ConstASCII8Str username,ConstASCII8Str password,class Console& console,UInt32 loginLocation);
	friend void		Login();
	friend void		LoginToStream(class Console& con,UInt32 loginLocation);
	friend void		Logout(class Process* osProcess,class User* theUser);
};

enum
{
	// Values for User::loginLocation
	kUnknownLogin	=	0,	// Don't know where they logged in from
	kDesktopLogin	=	1,	// Logged in to the computer they are sitting in front of
	kPrinterLogin	=	2,	// Logged in through the printer port (direct serial connection)
	kModemLogin	=	3,	// Logged in through the modem port (direct serial connection)
	kNetworkLogin	=	4	// Logged in over a network, possibly through a modem (note that this is different than a modem port direct connection) or ethernet
};

#define KEYBOARD_LOGIN		1
#define PRINTER_LOGIN		0
#define MODEM_LOGIN		0

#endif /* __LOGIN__ */