/*
	Command Line.cp
	Copyright © 1998 by Terry Greeniaus and Patrick Varilly

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
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Monday, 3 August 98	-	Fixed problem with delete key not working
*/
#include "SOS.h"
#include "NKMachineInit.h"
#include "Command Line.h"
#include "ADB.h"
#include "Memory Utils.h"
#include "Kernel Console.h"
#include "Assembly.h"
#include "NKInterruptVectors.h"
#include "NKDebuggerNub.h"
#include "ANSI.h"
#include "Apple HFS.h"
#include "File Systems.h"
#include "NKVideo.h"
#include "Code Fragments.h"
#include "PCI Interrupts.h"
#include "OGRE.h"
#include "Login.h"
#include "SOSMouse.h"
#include "NKProcessors.h"
#include "Mace.h"
#include "Chip Debugger.h"
#include "ESCC.h"
#include "NKThreads.h"
#include "USB.h"
#include "NKVirtualMemory.h"
#include "NKMemoryManager.h"
#include "L2CR.h"
#include "SWIM3.h"

#define	DEFAULT_PROMPT	"cin"
#define	DIRECTORY_INFO	0
#define	TERM_BUFFER_SIZE	10240

static	void	processCommand(ConstASCII8Str command );
static	void	Debugger();
static	void	StartTrace();
static	void	EndTrace();
static	void	EatStack(void);
static	void	Help(void);
static	void	Dummy( void );
static	void	TestDebugStr(void);
static	void	MachineCheck(void);
static	void	ListDirectory(void);
static	void	ChangeDirectory(ConstASCII8Str args);
static	void	More(ConstASCII8Str args);
static	void	SymAddr(ConstASCII8Str symName);
static	void	OSSymAddr(ConstASCII8Str symName);
static	void	Reverse(ConstASCII8Str args);
static	void	HzTick(void);
static	void	LogoutFromStupidOS();
static	void	DisplayProcessorSpeed();
static	void	SniffEthernet();
static	void	MaceInfo();
static	void	Chips();
static	void	WriteToModem(ConstASCII8Str args);
static	void	Term();
static	void	Underline();
static	void	Inverse();
static	void	Bold();
static	void	Plain();
static	void	Sleep();
static	void L2CRRead();
static	void FDEject();
static	void FDRead();
static	void	LineCount(ConstASCII8Str args);
static	UInt32 LineCountFile(FileDescriptor *file);
static	UInt32 LineCountDir(DirectoryDescriptor *dir);
static	void ReadF3019000Byte();
static	void ReadF3019000Word();
static	void ReadF3019000ByteTimes4();

typedef void (*VoidProcPtr)(void);
typedef void (*ArgsProcPtr)(ConstASCII8Str args);

typedef struct Command
{
	ConstASCII8Str		text;
	VoidProcPtr		procTVector;
}Command;

typedef struct ArgsCommand
{
	ConstASCII8Str		text;
	ArgsProcPtr		procTVector;
}ArgsCommand;

Command	theCommands[]	=		{	{"restart", Restart},
								{"shutdown",ShutDown},
								{"eatstack",EatStack},
								{"starttrace",StartTrace},
								{"endtrace",EndTrace},
								{"help",Help},
								{"debug",Debugger},
								{"debugstr",TestDebugStr},
								{"machinecheck",MachineCheck},
								{"dir",ListDirectory},
								{"hztick",HzTick},
								{"ogres",PlayOgre},
								{"logout",LogoutFromStupidOS},
								{"speed",DisplayProcessorSpeed},
								{"sniff",SniffEthernet},
								{"maceinfo",MaceInfo},
								{"debugchips",Chips},
								{"term",Term},
								{"underline",Underline},
								{"inverse",Inverse},
								{"bold",Bold},
								{"plain",Plain},
								{"sleep",Sleep},
								{"usbstat",USBStat},
								{"fdeject",FDEject},
								{"fdread",FDRead},
								{"memstat",MemStat}
							};

// All argCommands must end with a space!
ArgsCommand	argCommands[] =	{	{"cd ",ChangeDirectory},
								{"more ",More},
								{"symaddr ",SymAddr},
								{"ossymaddr",OSSymAddr},
								{"reverse ",Reverse},
								{"modem ",WriteToModem},
								{"lc ",LineCount}
							};

extern User*		stupidOSUser;
extern Process*	stupidOSProcess;

void InitCommandLine(void)
{
	ASCII8			commandLine[100] = {0};
	ConstASCII8Str		prompt = DEFAULT_PROMPT;
	if(stupidOSUser->propertyExists("os_stupidos_prompt"))
		prompt = stupidOSUser->getValue("os_stupidos_prompt",0);
	
	MemZero( (void*)commandLine, 100 );
	
	stupidOSUser->console << newLine << prompt << "> ";
	for(;;)
	{
		stupidOSUser->console.read(commandLine,99);
		stupidOSUser->console << newLine;
		processCommand(commandLine);
		stupidOSUser->console << newLine << prompt << "> ";
	}
}

void processCommand(ConstASCII8Str command )
{
	for(Int32 i=0;i<sizeof(theCommands)/sizeof(theCommands[0]);i++)
	{
		if(!strcmp(command,theCommands[i].text))
		{
			(*theCommands[i].procTVector)();
			return;
		}
	}
	
	for(Int32 i=0;i<sizeof(argCommands)/sizeof(argCommands[0]);i++)
	{
		UInt32 argCommandStrLen = strlen(argCommands[i].text);
		if(!strncmp(command,argCommands[i].text,argCommandStrLen))
		{
			(*argCommands[i].procTVector)(command + argCommandStrLen);
			return;
		}
	}
	
	stupidOSUser->console << "Unrecognized command" << newLine;
}

void Debugger()
{
	debuggerNub->debugger();
}

void StartTrace()
{
	debuggerNub->enableTrace();
}

void EndTrace()
{
	debuggerNub->disableTrace();
}

void EatStack(void)
{
	// Exhaust all stack space by recursion.  This is to test the guard page at the end of the stack.
	stupidOSUser->console << "sp = " << _getSP() << newLine;
	EatStack();
}

static void Help(void)
{
	stupidOSUser->console << "Available commands:" << newLine;
	for(Int32 i=0;i<sizeof(theCommands)/sizeof(theCommands[0]);i++)
		stupidOSUser->console << "   " << theCommands[i].text << newLine;
	for(Int32 i=0;i<sizeof(argCommands)/sizeof(argCommands[0]);i++)
		stupidOSUser->console << "   " << argCommands[i].text << newLine;
}

static void TestDebugStr(void)
{
	dout << "This is a test of DebugStr.\rPrometheus' debugger even supports multi-line\rDebugStr messages!\n";
}

static void MachineCheck(void)
{
	UInt32 oldMSR = DisableDR();
	*((UInt32*)0xFFFFFFFC) = 0;	// Write to byte at 768MB (.75 GB) - nonexistant physical memory
	_dcbf((void*)0xFFFFFFFC);
	SetMSR(oldMSR);
}

DirectoryDescriptor*	currDir = nil;

static void ListDirectory(void)
{
	if(currDir)
	{
		FileIterator*	fileIterator = currDir->newFileIterator();
		for(;*fileIterator;fileIterator->advance())
			stupidOSUser->console << (*fileIterator)->name() << newLine;
		delete fileIterator;
		
		DirectoryIterator*	directoryIterator = currDir->newDirectoryIterator();
		for(;*directoryIterator;directoryIterator->advance())
			stupidOSUser->console << "/" << (*directoryIterator)->name() << newLine;
		delete directoryIterator;
	}
	else
	{
		FileSystem*	fs = machine.fileSystems;
		
		while(fs)
		{
			stupidOSUser->console << "/" << fs->name() << newLine;
			fs = fs->next();
		}
	}
}

static void ChangeDirectory(ConstASCII8Str args)
{
	if(currDir)
	{
		if(args[0] == '.')
		{
			DirectoryDescriptor*	oldDir = currDir;
			currDir = currDir->parent();
			delete oldDir;
		}
		else
		{
			DirectoryDescriptor*	oldDir = currDir;
			currDir = currDir->subDir(args);
			if(currDir)
				delete oldDir;
			else
			{
				currDir = oldDir;
				stupidOSUser->console << "No such directory" << newLine;
			}
		}
	}
	else
	{
		FileSystem*	fs = machine.fileSystems;
		
		while(fs)
		{
			if(!strcmp(fs->name(),args,false))
			{
				currDir = fs->root();
				return;
			}
			fs = fs->next();
		}
		
		stupidOSUser->console << "No such volume" << newLine;
	}
}

static void More(ConstASCII8Str args)
{
	if(currDir)
	{
		Boolean dumpText = (*args == '.');
		if(dumpText)
			args++;
		
		FileDescriptor*	file = currDir->subFile(args);
		if(file)
		{
			FileIStream*		iStream = file->openForRead();
			UInt32			numBytes = iStream->eos();
			
			if(numBytes)
			{
				ASCII8Str			buffer = new ASCII8[numBytes];
#if DIRECTORY_INFO
				UInt64			startTimeNs = GetTime_ns();
#endif
				iStream->read(buffer,numBytes);
#if DIRECTORY_INFO
				UInt64			endTimeNs = GetTime_ns();
#endif
				if(dumpText)
				{
					ASCII8		theChar[2] = {0,0};
					for(Int32 i=0;i<numBytes;i++)
					{
						theChar[0] = buffer[i];
						if(theChar[0] == '\r')	// Perform translation
							stupidOSUser->console << newLine;
						else
							stupidOSUser->console << theChar;
					}
				}
#if DIRECTORY_INFO
				stupidOSUser->console << newLine << newLine << newLine << "----------------------" << newLine << "File size: " << numBytes << newLine;
				stupidOSUser->console << "Read time: " << (endTimeNs - startTimeNs)/1000000 << " ms" << newLine;
				stupidOSUser->console << "Transfer rate: " << ((UInt64)(((UInt64)numBytes)*1000000000ULL)/(UInt64)(1024ULL*(endTimeNs - startTimeNs))) << " K/s" << newLine;
#endif
				delete [] buffer;
			}
			else
				stupidOSUser->console << "File has no data fork!" << newLine;
			
			delete iStream;	
			delete file;
		}
		else
			stupidOSUser->console << "No such file" << newLine;
	}
	else
		stupidOSUser->console << "Select volume first" << newLine;
}

static void SymAddr(ConstASCII8Str args)
{
	stupidOSUser->console << "Addr of " << args << ": " << (UInt32)machine.pefFragment->getSymAddr(args) << newLine;
}

static void OSSymAddr(ConstASCII8Str args)
{
	CodeFragment*	osFrag = CodeFragment::getFragment("OS_Stupid OS");
	if(!osFrag)
	{
		stupidOSUser->console << "Couldn't find OS_Stupid OS fragment.\n";
		return;
	}
	stupidOSUser->console << "Addr of " << args << ": " << (UInt32)osFrag->getSymAddr(args) << newLine;
}

static void Reverse(ConstASCII8Str args)
{
	UInt32		len = strlen(args);
	ASCII8Str		string = new ASCII8[len + 1];
	MemCopy(args,string,len + 1);
	ReverseMem(string,len);
	stupidOSUser->console << string << newLine;
	delete [] string;
}

static void HzTick(void)
{
	registerPCIInterrupt(nil,PMAC_DEV_HZTICK);
}

static void LogoutFromStupidOS()
{
	if(sosMouse)
	{
		sosMouse->shieldMouse();
		delete sosMouse;
	}
	Logout(stupidOSProcess,stupidOSUser);
}

static void DisplayProcessorSpeed()
{
	UInt32	hz = NKGetThisProcessorInfo()->hz;
	stupidOSUser->console << "Processor frequency" << newLine;
	stupidOSUser->console << hz << " Hz" << newLine;
	UInt32	mhzTimes10 = (hz + 50000)/100000;
	UInt32	mhzDecimal = (mhzTimes10 % 10);
	UInt32	mhz = (mhzTimes10 / 10);
	stupidOSUser->console << mhz << "." << mhzDecimal << " MHz" << newLine;
	
	stupidOSUser->console << newLine << "Decrementor frequency" << newLine;
	hz = NKGetThisProcessorInfo()->decHz;
	stupidOSUser->console << hz << " Hz" << newLine;
	mhzTimes10 = (hz + 50000)/100000;
	mhzDecimal = (mhzTimes10 % 10);
	mhz = (mhzTimes10 / 10);
	stupidOSUser->console << mhz << "." << mhzDecimal << " MHz" << newLine;
	
	stupidOSUser->console << newLine << "Bus speed" << newLine;
	hz = NKGetThisProcessorInfo()->busHz;
	stupidOSUser->console << hz << " Hz" << newLine;
	mhzTimes10 = (hz + 50000)/100000;
	mhzDecimal = (mhzTimes10 % 10);
	mhz = (mhzTimes10 / 10);
	stupidOSUser->console << mhz << "." << mhzDecimal << " MHz" << newLine;
}

static void SniffEthernet(void)
{
	stupidOSUser->console << "Sniffing Ethernet Packet" << newLine;
	Sniff();
}

static void MaceInfo(void)
{
	stupidOSUser->console << "Mace info" << newLine;
	DumpRegs();
}

static void Chips(void)
{
	DebugChips();
}

static void WriteToModem(ConstASCII8Str args)
{
	modemPort.write(args,strlen(args));
}

struct TerminalReadThread	:	public Thread
{
	class Terminal*	terminal;
	
	TerminalReadThread(Terminal* t);
	virtual	void		threadLoop(void);
			UInt32	processEscapeCode(ConstASCII8Str escCode,Int32* n1,Int32* n2);
};

struct TerminalWriteThread	:	public Thread
{
	class Terminal*	terminal;
	
	TerminalWriteThread(Terminal* t);
	virtual	void		threadLoop(void);
};

struct Terminal	:	public OStream
{
	volatile UInt8*			circBuff;
	volatile UInt8*			dataPtr;
	volatile UInt8*			freePtr;
	Thread*				terminalGoThread;
	
	TerminalReadThread*	readThread;
	TerminalWriteThread*	writeThread;
	Boolean				quit;
	
	Terminal();
	~Terminal();
			void	go();
	
	virtual	void	write(ConstPtr data,UInt32 len);
};

TerminalReadThread::TerminalReadThread(Terminal* t):
	Thread(10240,1,"Terminal read thread")
{
	terminal = t;
}

void TerminalReadThread::threadLoop()
{
	ASCII8	theChar[] = {0,0};
	ASCII8	escapeCode[20];
	ASCII8*	nextEscChar;
	Boolean	doingEscapeCode = false;
	
	for(;;)
	{
		while (terminal->dataPtr == terminal->freePtr)
			sleepMS(10);
		while(terminal->dataPtr != terminal->freePtr)
		{
			theChar[0] = *terminal->dataPtr++;
			if(terminal->dataPtr >= terminal->circBuff + TERM_BUFFER_SIZE)
				terminal->dataPtr = terminal->circBuff;
			
			if(theChar[0] == 0x1B)	// Escape code
			{
				doingEscapeCode = true;
				nextEscChar = escapeCode;
			}
			else if(doingEscapeCode)
			{
				*nextEscChar++ = theChar[0];
				*nextEscChar = '\0';
				Int32	n1;
				Int32	n2;
				UInt32	code = processEscapeCode(escapeCode,&n1,&n2);
				if(code != -1)
				{
					doingEscapeCode = false;
					switch(code)
					{
						default:	// Unhandled
							nkVideo << "Unhandled escape code " << escapeCode << "\n";
						break;
						case 33:	// Turn off character attributes
						case 34:
							videoConsole->plainText();
						break;
						case 35:	// Bold
							videoConsole->boldText();
						break;
						case 37:	// Underline
							videoConsole->underlineText();
						break;
						case 38:	// Blinking
						case 39:	// Inverse
							videoConsole->inverseText();
						break;
						case 42:	// Move cursor up n1 lines
							videoConsole->moveCursorRel(0,-n1);
						break;
						case 43:	// Move cursor down n1 lines
							videoConsole->moveCursorRel(0,n1);
						break;
						case 44:	// Move cursor right n1 lines
							videoConsole->moveCursorRel(n1,0);
						break;
						case 45:	// Move cursor left n1 lines
							videoConsole->moveCursorRel(-n1,0);
						break;
						case 46:	// Move cursor to upper left corner
						case 48:
							videoConsole->moveCursorAbs(0,0);
						break;
						case 47:	// Move cursor to screen location (n1,n2)
						case 49:
							videoConsole->moveCursorAbs(n2,n1);
						break;
						case 63:	// Clear line from cursor right
						case 64:
							videoConsole->clearToEndOfLine();
						break;
						case 65:	// Clear from cursor left
							videoConsole->clearFromStartOfLine();
						break;
						case 66:	// Clear entire line
							videoConsole->clearLine();
						break;
						case 67:	// Clear screen from cursor down
						case 68:
							videoConsole->clearToBottomOfScreen();
						break;
						case 69:	// Clear from cursor up
							videoConsole->clearFromTopOfScreen();
						break;
						case 70:	// Clear entire sceen
							videoConsole->clearScreen();
						break;
						case 79:	// Reset terminal
							videoConsole->reset();
						break;
					}
				}
			}
			else if(theChar[0] == '\r')
				videoConsole->moveCursorToStartOfLine();
			else
				stupidOSUser->console << theChar;
		}
	}
	
	for(;;)
		;
}

ASCII8*		escapeCodes[]	=	{	"[20h",	// 0 - Set new line mode
								"[?1h",	// 1 - Set cursor key to application
								"[?3h",	// 2 - Set number of columns to 132
								"[?4h",	// 3 - Set smooth scrolling
								"[?5h",	// 4 - Set reverse video on screen
								"[?6h",	// 5 - Set origin to relative
								"[?7h",	// 6 - Set auto-wrap mode
								"[?8h",	// 7 - Set auto-repeat mode
								"[?9h",	// 8 - Set interlacing mode
								"[20l",	// 9 - Set line feed mode
								"[?1l",	// 10 - Set cursor key to cursor
								"[?2l",	// 11 - Set VT52 (versus ANSI)
								"[?3l",	// 12 - Set number of columns to 80
								"[?4l",	// 13 - Set jump scrolling
								"[?5l",	// 14 - Set normal video on screen
								"[?6l",	// 15 - Set origin to absolute
								"[?7l",	// 16 - Reset auto-wrap mode
								"[?8l",	// 17 - Reset auto-repeat mode
								"[?9l",	// 18 - Reset auto-interlacing mode
								"=",		// 19 - Set alternate keypad mode
								">",		// 20 - Set numeric keypad mode
								"(A",	// 21 - Set United Kingdom G0 character set
								")A",	// 22 - Set United Kingdom G1 character set
								"(B",	// 23 - Set United States G0 character set
								")B",	// 24 - Set United States G1 character set
								"(0",	// 25 - Set G0 special chars. & line set
								")0",	// 26 - Set G1 special chars. & line set
								"(1",	// 27 - Set G0 alternate character ROM
								")1",	// 28 - Set G1 alternate character ROM
								"(2",	// 29 - Set G0 alt char ROM and spec. graphics
								")2",	// 30 - Set G1 alt char ROM and spec. graphics
								"N",		// 31 - Set single shift 2
								"O",		// 32 - Set single shift 3
								"[m",	// 33 - Turn off character attributes
								"[0m",	// 34 - Turn off character attributes
								"[1m",	// 35 - Turn bold mode on
								"[2m",	// 36 - Turn low intensity mode on
								"[4m",	// 37 - Turn underline mode on
								"[5m",	// 38 - Turn blinking mode on
								"[7m",	// 39 - Turn reverse video on
								"[8m",	// 40 - Turn invisible text mode on
								"[*;*r",	// 41 - Set top and bottom lines of a window
								"[*A",	// 42 - Move cursor up * lines
								"[*B",	// 43 - Move cursor down * lines
								"[*C",	// 44 - Move cursor right * lines
								"[*D",	// 45 - Move cursor left * lines
								"[H",	// 46 - Move cursor to upper left corner
								"[*;*H",	// 47 - Move cursor to screen location (*,*)
								"[f",		// 48 - Move cursor to upper left corner
								"[*;*f",	// 49 - Move cursor to screen location (*,*)
								"D",		// 50 - Move/scroll window up one line
								"M",		// 51 - Move/scroll window down one line
								"E",		// 52 - Move to next line
								"7",		// 53 - Sace cursor position and attributes
								"8",		// 54 - Restore cursor position and attributes
								"H",		// 55 - Set a tab at the current column
								"[g",		// 56 - Clear a tab at the current column
								"[0g",	// 57 - Clear a tab at the current column
								"[3g",	// 58 - Clear all tabs
								"#3",	// 59 - Double-height letters, top half
								"#4",	// 60 - Double-height letters, bottom half
								"#5"	,	// 61 - Single-width, single height letters
								"#6",	// 62 - Double-width, single height letters
								"[K",	// 63 - Clear line from cursor right
								"[0K",	// 64 - Clear line from cursor right
								"[1K",	// 65 - Clear line from cursor left
								"[2K",	// 66 - Clear entire line
								"[J",	// 67 - Clear screen from cursor down
								"[0J",	// 68 - Clear screen from cursor down
								"[1J",	// 69 - Clear screen from cursor up
								"[2J",	// 70 - Clear entire screen
								"5n",	// 71 - Device status report
								"0n",	// 72 - Response: terminal is OK
								"3n",	// 73 - Response: terminal is not OK
								"6n",	// 74 - Get cursor position
								"*;*R",	// 75 - Response: cursor is at v,h
								"[c",		// 76 - Identify what terminal type
								"[0c",	// 77 - Identify what terminal type
								"[?1*;0c",	// 78 - Response: terminal type code n
								"c",		// 79 - Reset terminal to initial state
								"#8",	// 80 - Screen alignment display
								"[2;1y",	// 81 - Confidence power up test
								"[2;2y",	// 82 - Confidence loopback test
								"[2;9y",	// 83 - Repeat power up test
								"[2;10y",	// 84 - Repeat loopback test
								"[0q",	// 85 - Turn off all four leds
								"[1q",	// 86 - Turn on LED #1
								"[2q",	// 87 - Turn on LED #2
								"[3q",	// 88 - Turn on LED #3
								"[4q"	// 89 - Turn on LED #4
							};


UInt32 TerminalReadThread::processEscapeCode(ConstASCII8Str escCode,Int32* n1,Int32* n2)
{
	for(UInt32 i=0;i<sizeof(escapeCodes)/sizeof(escapeCodes[0]);i++)
	{
		UInt32	whichN = 0;
		*n1 = *n2 = 0;
		ASCII8*	table = escapeCodes[i];
		const ASCII8*	comp = escCode;
		while(*table && *comp)
		{
			if(*table == '*')	// Wild-card number
			{
				Boolean	gotNumber = false;
				while(*comp >= '0' && *comp <= '9')
				{
					gotNumber = true;
					if(whichN == 0)
					{
						*n1 *= 10;
						*n1 += *comp - '0';
					}
					else
					{
						*n2 *= 10;
						*n2 += *comp - '0';
					}
					comp++;
				}
				if(gotNumber)
				{
					comp--;
					whichN++;
				}
			}
			else if(*table != *comp)
				break;
			table++;
			comp++;
		}
		if(!*table && !*comp)
			return i;
	}
	return -1;
}

TerminalWriteThread::TerminalWriteThread(Terminal* t):
	Thread(10240,1,"Terminal write thread")
{
	terminal = t;
}

void TerminalWriteThread::threadLoop()
{
	ASCII8	theChar;
	for(;;)
	{
		stupidOSUser->console.read(&theChar,1);
		if(theChar == 0x1B)	// Escape
			break;
		modemPort << theChar;
	}
	
	terminal->terminalGoThread->resume();
	
	for(;;)
		;
}

Terminal::Terminal()
{
	quit = false;
	circBuff = new UInt8[TERM_BUFFER_SIZE];
	dataPtr = freePtr = circBuff;
	readThread = new TerminalReadThread(this);
	writeThread = new TerminalWriteThread(this);
}

Terminal::~Terminal()
{
	delete writeThread;
	delete readThread;
	delete [] circBuff;
}

void Terminal::go()
{
	terminalGoThread = CurrThread::thread();
	
	modemPort.createPipe(this);
	
	readThread->resume();
	writeThread->resume();
	
	CurrThread::suspend();
	
	modemPort.disposePipe(this);
}

void Terminal::write(ConstPtr data,UInt32 len)
{
	while(len--)
	{
		*freePtr++ = *data++;
		if(freePtr >= circBuff + TERM_BUFFER_SIZE)
			freePtr = circBuff;
		if(freePtr == dataPtr)
		{
			nkVideo << "circular buffer collision\n";
			dataPtr++;
			if(dataPtr >= circBuff + TERM_BUFFER_SIZE)
				dataPtr = circBuff;
		}
	}
}

void Term()
{
	Terminal	t;
	t.go();
}

void Underline()
{
	stupidOSUser->console.underlineText();
}

void Inverse()
{
	stupidOSUser->console.inverseText();
}

void Bold()
{
	stupidOSUser->console.boldText();
}

void Plain()
{
	stupidOSUser->console.plainText();
}

void Sleep()
{
	Wait_s(4);
}

static void LineCount(ConstASCII8Str args)
{
	if(currDir)
	{
		UInt32					totalLines = 0;
		FileDescriptor				*file;
		if( currDir->fileExists( args ) )
		{
			file = currDir->subFile( args );
			totalLines = LineCountFile( file );
			delete file;
		}
		else
		{
			DirectoryDescriptor*	dir = currDir->subDir(args);
			if(dir)
				totalLines = LineCountDir( dir );
			else
				stupidOSUser->console << "No such directory!" << newLine;
			
			delete dir;
		}
		
		stupidOSUser->console << "Total lines in " << args << ": " << totalLines << newLine;
	}
	else
		stupidOSUser->console << "Select volume first" << newLine;
}

UInt32 LineCountFile(FileDescriptor *file)
{
	stupidOSUser->console << "Scanning " << file->name() << "\n";
	
	FileIStream*		iStream = file->openForRead();
	UInt32			numBytes = iStream->eos();
	UInt32			lineCount = 0;
	
	if(numBytes)
	{
		ASCII8Str			buffer = new ASCII8[numBytes];
		Boolean			lastCharNL = false;
		iStream->read(buffer,numBytes);
		for(UInt32 i=0;i<numBytes;i++)
		{
			lastCharNL = false;
			if(buffer[i] == '\r')	// Count marked lines
			{
				lineCount++;
				lastCharNL = true;
			}
		}
		if( !lastCharNL )
			lineCount++;		// Count last line if it has no ending return character
		delete [] buffer;
	}
	else
		stupidOSUser->console << "File " << file->name() << " has no data fork!" << newLine;
	
	delete iStream;	
	delete file;
	
	return lineCount;
}

UInt32 LineCountDir(DirectoryDescriptor *dir)
{
	stupidOSUser->console << "Scanning /" << dir->name() << "\n";
	
	UInt32				lineCount = 0;
	
	FileIterator*			fileIterator = dir->newFileIterator();
	for(;*fileIterator;fileIterator->advance())
		lineCount += LineCountFile( (FileDescriptor*)*fileIterator );
	delete fileIterator;
	
	DirectoryIterator*		directoryIterator = dir->newDirectoryIterator();
	for(;*directoryIterator;directoryIterator->advance())
		lineCount += LineCountDir( (DirectoryDescriptor*)*directoryIterator );
	delete directoryIterator;
	
	return lineCount;
}

void L2CRRead()
{
	stupidOSUser->console << "L2CR: " << hexMsg << _getL2CR() << decMsg << "\n";
}

extern SWIM3Device* swim3Device;
void FDEject()
{
	IOCommand* cmd = swim3Device->eject();
	CurrThread::blockForIO(cmd);
	delete cmd;
}

void FDRead()
{
	UInt32	block0[128];
	IOCommand* cmd = swim3Device->readSectorsAsync((Int8*)block0,2,1);
	CurrThread::blockForIO(cmd);
	
	cout << hexMsg;
	for(UInt32 i=0;i<16;i++)
	{
		for(UInt32 j=0;j<8;j++)
			cout << block0[i*8 + j] << " ";
		cout << "\n";
	}
	cout << decMsg;
	delete cmd;
}
