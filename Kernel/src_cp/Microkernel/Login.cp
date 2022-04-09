/*
	Login.cp
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
#include "Login.h"
#include "File Systems.h"
#include "Kernel Console.h"
#include "NKThreads.h"
#include "Video Driver.h"
#include "OSs.h"
#include "ANSI.h"
#include "Time.h"
#include "ESCC.h"
#include "VT100 Console.h"

static	User*		ParseUserFile(FileIStream* s,ConstASCII8Str username,ConstASCII8Str password,Console& console);	// Returns a User* if the user is found, nil otherwise
		void			LoginToStream(IStream& input,Console& con);

class LoginThread	:	public Thread
{
public:
	Console&	con;
	UInt32	loginLocation;
	
	LoginThread(Console& console,ConstASCII8Str name,UInt32 _loginLocation):
		Thread(20480,1,name),
		con(console),loginLocation(_loginLocation)
	{
	}
	
	virtual	void	threadLoop()
	{
		LoginToStream(con,loginLocation);
	}
};

typedef struct UserProperty
{
	struct UserProperty*	next;
	ASCII8Str				propertyName;
	UInt32				numValues;
	ASCII8Str				value[];
}UserProperty;

MutexLock			usersLock;
static User*			users;

MutexLock			killLock;
Process*				killProcess;


void Login()
{
	// Start a vt100 console on the modem port and enable it for logging in
#if MODEM_LOGIN
	Console*		vt100Console = new VT100Console(modemPort,modemPort);
	(new LoginThread(*vt100Console,"VT100 Login",kModemLogin))->start();
#endif
	
	// Start a vt100 console on the printer port and enable it for logging in
#if PRINTER_LOGIN
	vt100Console = new VT100Console(printerPort,printerPort);
	(new LoginThread(*vt100Console,"VT100 Login",kPrinterLogin))->start();
#endif
	
	// Enable the kernel video console for logging in
#if KEYBOARD_LOGIN
	(new LoginThread(*videoConsole,"Desktop Login",kDesktopLogin))->resume();
#endif
	
	// Let the login threads do their things now!
	CurrThread::suspend();
}

void LoginToStream(Console& con,UInt32 loginLocation)
{
	for(;;)
	{
		con.clearScreen();
		con.moveCursorAbs(0,0);
		con << "Prometheus " << microKernelVers << " login screen" << newLine;
		
		// See if we could find the kernel folder
		if(!kernelDirectory)
		{
			con << "Couldn't find Kernel Booter folder!" << newLine;
			CurrThread::suspend();
		}
		
		// See if we could find the Boot Info folder
		//con << "Getting kernelDirectory's parent" << newLine;
		DirectoryDescriptor*	dir = kernelDirectory->parent();
		//con << "Getting Boot Info folder" << newLine;
		DirectoryDescriptor*	bootInfo = dir->subDir("Boot Info");
		//con << "Deleting temp directory" << newLine;
		delete dir;
		if(!bootInfo)
		{
			con << "Couldn't find \"Boot Info\" folder!" << newLine;
			CurrThread::suspend();
		}
		//con << "Getting users.txt file" << newLine;
		FileDescriptor*			usersFile = bootInfo->subFile("users.txt");
		if(!usersFile)
		{
			con << "Couldn't find \"users.txt\" file!" << newLine;
			CurrThread::suspend();
		}
		//con << "Deleting temp directory" << newLine;
		delete bootInfo;
		
		//con << "Opening users.txt file" << newLine;
		FileIStream*	stream = usersFile->openForRead();
		ASCII8		username[100];
		ASCII8		password[100];
		User*		theUser = nil;
		
		while(!theUser)
		{
			con << newLine << "Username: ";
			con.read(username,99);
			con << newLine << "Password: ";
			con.invisibleText();
			con.read(password,99);
			con.plainText();
			con << newLine;
			
			theUser = ParseUserFile(stream,username,password,con,loginLocation);
			if(!theUser)
			{
				con << "Bad login" << newLine;
				stream->setPos(0);
			}
		}
		
		delete stream;
		
		if(theUser)
		{
			theUser->loginThread = CurrThread::thread();
			
			// Load any OS's
			con << newLine << "Loading Operating Systems" << newLine;
			InitOSs(theUser);
			if(theUser->numOSs)
			{
				CurrThread::suspend();	// The end of the login thread
				while(theUser->numOSs)
				{
					delete killProcess;
					killLock.unlock();
				}
			}
			else
			{
				con << "This user has no available operating systems" << newLine;
				Wait_s(2);
			}
			killLock.unlock();
			delete theUser;
		}
	}
}

void Logout(Process* osProcess,User* theUser)
{
	killLock.lock();
	theUser->numOSs--;
	killProcess = osProcess;
	theUser->loginThread->resume();
	CurrThread::suspend();
}

enum
{
	stateBetweenRecords	= 0,	// Between user records
	stateUserPassword		= 1,	// Start the user password line
	stateProperty			= 2	// Start a property line
};

static	User*	ParseUserFile(FileIStream* s,ConstASCII8Str username,ConstASCII8Str password,Console& console,UInt32 loginLocation)
{
	UInt32		len = s->eos();
	ASCII8		theChar;
	Int8			state = stateBetweenRecords;
	Boolean		userNameMatch = false;
	User*		theUser = nil;
	
	while(len)
	{
		s->read(&theChar,1);
		len--;
		switch(theChar)
		{
			case ';':	// Comment line
				while(len)
				{
					s->read(&theChar,1);
					len--;
					if(theChar == '\n' || theChar == '\r')
						break;
				}
			break;
			case '\n':	// Empty line
			case '\r':
				if(theUser)	// End of a record - if we just parsed the users record return true
					return theUser;
				state = stateBetweenRecords;
			break;
			case '-':	// End of user info
				if(theUser)	// End of file - someone forgot a '\n' after the last user's record - not a fatal one though!
					cout << "Syntax error: Last user record in \"users.txt\" file didn't end with an empty line\n";
				return theUser;
			break;
			default:	// A user record
				if(state == stateBetweenRecords)
				{
					// This is the first line of a user record - the username line
					ASCII8		fileUserName[100] = {theChar,'\0'};
					UInt32		fileUserNameLen = 1;
					while(len)
					{
						s->read(&theChar,1);
						len--;
						if(theChar == '\n' || theChar == '\r')
							break;
						fileUserName[fileUserNameLen++] = theChar;
						fileUserName[fileUserNameLen] = '\0';
					}
					if(!strcmp(fileUserName,username))
						userNameMatch = true;
					state = stateUserPassword;
				}
				else if(state == stateUserPassword && userNameMatch)	// This is the user's password
				{
					ASCII8		filePassword[100] = {theChar,'\0'};
					UInt32		filePasswordLen = 1;
					while(len)
					{
						s->read(&theChar,1);
						len--;
						if(theChar == '\n' || theChar == '\r')
							break;
						filePassword[filePasswordLen++] = theChar;
						filePassword[filePasswordLen] = '\0';
					}
					if(strcmp(filePassword,password))
						return nil;	// Bad password, done since we can't have duplicate users with different passwords
					theUser = new User(username,console,loginLocation);
					state = stateProperty;
				}
				else if(state == stateProperty && theUser)		// A property we should add to the user info
				{
					ASCII8		propertyName[100] = {theChar,'\0'};
					UInt32		propertyNameLen = 1;
					UInt32		numValues = 0;
					while(len)
					{
						s->read(&theChar,1);
						len--;
						if(theChar == '\n' || theChar == '\r' || theChar == '\t')
							break;
						propertyName[propertyNameLen++] = theChar;
						propertyName[propertyNameLen] = '\0';
					}
					
					// Count the values
					UInt32		fPos;
					if(theChar == '\t')
					{
						numValues++;
						fPos = s->getPos();
						while(len)
						{
							s->read(&theChar,1);
							len--;
							if(theChar == '\t')
								numValues++;
							else if(theChar == '\n' || theChar == '\r')
								break;
						}
					}
					
					// Create the property
					UserProperty*	theProperty = theUser->addProperty(propertyName,numValues);
					if(numValues)
					{
						s->setPos(fPos);
						for(UInt32 i=0;i<numValues;i++)
						{
							ASCII8		valueName[100] = {0};
							UInt32		valueNameLen = 0;
							
							do
							{
								s->read(&theChar,1);
								if(theChar != '\t' && theChar != '\n' && theChar != '\r')
								{
									valueName[valueNameLen++] = theChar;
									valueName[valueNameLen] = '\0';
								}
							}while(theChar != '\t' && theChar != '\n' && theChar != '\r');
							
							theProperty->value[i] = new ASCII8[valueNameLen + 1];
							strcpy(theProperty->value[i],valueName);
						}
					}
				}
				else	// Some line we should skip
				{
					state = stateProperty;
					while(len)
					{
						s->read(&theChar,1);
						len--;
						if(theChar == '\n' || theChar == '\r')
							break;
					}
				}
			break;
		}
	}
	
	cout << "\nSyntax Error: \"users.txt\" file did not end with '-'\n";
	return false;
}

User::User(ConstASCII8Str _userName,Console& userConsole,UInt32 _loginLocation):
	console(userConsole)
{
	properties = nil;
	userName = new ASCII8[strlen(_userName) + 1];
	strcpy(userName,_userName);
	
	MutexLocker	usersLocker(usersLock);
	nextUser = users;
	users = this;
	numOSs = 0;
	loginLocation = _loginLocation;
}

User::~User()
{
	{
		MutexLocker	usersLocker(usersLock);
		User*		theUser = users;
		if(theUser == this)
			users = nextUser;
		else
		{
			while(theUser->nextUser != this)
				theUser = theUser->nextUser;
			theUser->nextUser = nextUser;
		}
	}
	
	UserProperty*	property = properties;
	while(property)
	{
		delete [] property->propertyName;
		UserProperty*	nextProperty = property->next;
		delete [] (UInt32*)property;
		property = nextProperty;
	}
	
	delete [] userName;
}

UserProperty* User::addProperty(ConstASCII8Str propertyName,UInt32 numValues)
{
	UInt32*	memPtr;
	memPtr = new UInt32[sizeof(UserProperty)/4 + numValues];
	UserProperty*	property = (UserProperty*)memPtr;
	
	property->next = nil;
	property->propertyName = new ASCII8[strlen(propertyName)+1];
	strcpy(property->propertyName,propertyName);
	property->numValues = numValues;
	
	if(properties)
	{
		UserProperty*	lastProperty = properties;
		while(lastProperty->next)
			lastProperty = lastProperty->next;
		lastProperty->next = property;
	}
	else
		properties = property;
	
	return property;
}

UserProperty* User::getProperty(ConstASCII8Str propertyName)
{
	UserProperty*	property = properties;
	while(property)
	{
		if(!strcmp(property->propertyName,propertyName))
			return property;
		property = property->next;
	}
	
	return nil;
}

Boolean User::propertyExists(ConstASCII8Str propertyName)
{
	return (getProperty(propertyName) != nil);
}

UInt32 User::numValues(ConstASCII8Str propertyName)
{
	UserProperty*	property = getProperty(propertyName);
	Assert(property != nil);
	return property->numValues;
}

ConstASCII8Str User::getValue(ConstASCII8Str propertyName,UInt32 n)
{
	UserProperty*	property = getProperty(propertyName);
	Assert(property != nil);
	Assert(n < property->numValues);
	return property->value[n];
}

ConstASCII8Str User::name()
{
	return userName;
}
