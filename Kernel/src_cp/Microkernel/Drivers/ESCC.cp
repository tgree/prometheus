/*
	ESCC.cp
	Copyright © 1998 by Terry Greeniaus, based heavily on work by Alessandro Forin

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
	Version History
	============
	Terry Greeniaus	-	Wednesday, 4 November 98	-	Original creation of file
*/
#include "ESCC.h"
#include "External Interrupt.h"
#include "Chip Debugger.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "Streams.h"
#include "NKThreads.h"

static RegisterDescriptor	esccRegisterDescriptor[] =	{	{"reg select",1,REG_WRITE_ONLY,0},
												{"read reg",1,REG_READ_ONLY | REG_SIDE_EFFECTS,0},
												{"write reg",1,REG_WRITE_ONLY,0},
												{"data",1,REG_SIDE_EFFECTS,16},
												LAST_REGISTER
											};

struct ESCCChannel	:	public IOCommandFullDuplexDriver,
					public IOStream
{
	SpinLock				deviceLock;
	class ESCCIOCommand*	readCommand;
	class ESCCIOCommand*	writeCommand;
	esccChannel*			channelRegs;
	esccDevice*			deviceRegs;
	UInt32				channel;
	class ESCCDriver*		myDriver;
	
	ESCCChannel(MachineDevice<esccDevice>* device,MachineDevice<esccChannel>* channel,ConstASCII8Str name,ESCCDriver* driver);
	virtual ~ESCCChannel();
	
	// Stuff for Driver
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// Stuff for IOCommandDriver
	virtual	void	startAsyncIORead(IOCommand* cmd);
	virtual	void	startAsyncIOWrite(IOCommand* cmd);
	
	// Stuff for IOStream
	virtual	void	read(Ptr data,UInt32 len);
	virtual	void	write(ConstPtr data,UInt32 len);
	
	// Other stuff
			void		writeChannelRegister(UInt8 channel,UInt8 n,UInt8 v);
			UInt8	readChannelRegister(UInt8 channel,UInt8 n);
			void		writeRegister(UInt8 n,UInt8 v);
			UInt8	readRegister(UInt8 n);
			void		handleInterrupt();
};

struct ESCCDriver	:	public InterruptHandler
{
	ESCCChannel*	channelA;
	ESCCChannel*	channelB;
	
	ESCCDriver(MachineDevice<esccDevice>* device,MachineDevice<esccChannel>* deviceChannelA,MachineDevice<esccChannel>* deviceChannelB);
	
	virtual	void	handleInterrupt();
};

enum
{
	ioRead	=	0,
	ioWrite	=	1
};

struct ESCCIOCommand	:	public IOCommand
{
	UInt32	operation;
	Ptr		data;
	UInt32	len;
	UInt32	result;
	
	ESCCIOCommand(UInt32 operation,Ptr data,UInt32 len);
	
	virtual	UInt32	ioError();
};

// This is a table of all the _integral_ baud rates - that is to say all baud rates that are exact divisors of the
// baud rate clock.  Simply search the table for the one closest to your desired baud rate, and subtract two
// from the other value in the pair.  That will give you your baud rate constant.  Of course you can always
// use the standard baud rate formula to get your time constant two.  However, suppose you want a baud
// rate of 45000.  Well, needless to say, if you apply the formula you will get a baud rate of 38400.  That's
// because the port doesn't support a baud rate anywhere near 45000 (38400 is the closest).  The standard
// formula is:
//
//	tc = (freq)/(2*baudRate) - 2
//
// Freq on PowerMacs turns out to be 3686400 divided by your clock multiplier (thus with the x16 clock
// it is 230400, with the x32 clock it is 115200 and with the x64 clock it is 57600).  So your real tc is:
//
//	tc = (3686400/m)/(2*baudRate) - 2
//
// Now, under MkLinux and LinuxPPC, they use a slightly modified formula that takes into account that you
// can in fact get the formula to round itself off by adding half the divisor.  i.e. to round off in a division
// correctly using the integer registers:
//
//	q = (a/b),		but q rounded correct = (a + (b/2))/b
//
// Thus to make this work:
//
//	q = tc
//	a = (3686400/m)
//	b = (2*baudRate)
//
//	q = (a + (b/2))/b - 2		--->	tc = (3686400/m + (2*baudRate/2))/(2*baudRate) - 2
//						---> tc = (3686400/m + baudRate)/(2*baudRate) - 2
//
// Thus, the closest you can get to 45000 with the x16 clock is:
//
//	tc	= (3686400/16 + 45000))/(2*45000) - 2
//		= (230400 + 45000)/(90000) - 2
//		= 275400/90000 - 2
//		= 1.06
//		= 1
//
// Now, if you look on the table at the entry for 38400, you will see that it pairs up with the number
// 3.  Remember, when calculating a tc with the table, you need subtract two from the paired entry.  Thus
// 3-2 == 1, which is the number we calculated above.  So it turns out that instead of calculating a baud
// rate time constant of 45000, we actually calculated the one for 38400.
static UInt16	baudRateTable[][2] =			{	{2,		57600},
										{3,		38400},
										{4,		28800},
										{5,		23040},
										{6,		19200},
										{8,		14400},
										{9,		12800},
										{10,		11520},
										{12,		9600},
										{15,		7680},
										{16,		7200},
										{18,		6400},
										{20,		5760},
										{24,		4800},
										{25,		4608},
										{30,		3840},
										{32,		3600},
										{36,		3200},
										{40,		2880},
										{45,		2560},
										{48,		2400},
										{50,		2304},
										{60,		1920},
										{64,		1800},
										{72,		1600},
										{75,		1536},
										{80,		1460},
										{90,		1280},
										{96,		1200},
										{100,	1152},
										{120,	960},
										{128,	900},
										{144,	800},
										{150,	768},
										{160,	720},
										{180,	640},
										{192,	600},
										{200,	576},
										{225,	512},
										{240,	480},
										{256,	450},
										{288,	400},
										{300,	384},
										{320,	360},
										{0,		0}
									};

IOStreamWrapper	modemPort("Modem port");
IOStreamWrapper	printerPort("Printer port");

void InitESCC(void)
{
	if(machine.sccInUseForDebugger)
		return;
	
	if(machine.esccDevice.physicalAddr)
	{
		ESCCDriver* driver = new ESCCDriver(&machine.esccDevice,&machine.esccAChannel,&machine.esccBChannel);
		modemPort.setStream(driver->channelA);
		printerPort.setStream(driver->channelB);
		machine.driverList.enqueue(driver->channelA);
		machine.driverList.enqueue(driver->channelB);
		new Chip("SCC A (Modem)",esccRegisterDescriptor,machine.esccAChannel.logicalAddr);
		new Chip("SCC B (Printer)",esccRegisterDescriptor,machine.esccBChannel.logicalAddr);
	}
}

UInt16 BaudRateToTimeConstant(UInt16 baudRate)
{
	UInt32	index = 0;
	if(baudRate >= 360)
		index = 1;
	
	for(UInt32 i=0;baudRateTable[i][0];i++)
	{
		if(baudRateTable[i][index] == baudRate)
			return baudRateTable[i][!index] - 2;
	}
	
	return -1;
}

ESCCChannel::ESCCChannel(MachineDevice<esccDevice>* device,MachineDevice<esccChannel>* _channel,ConstASCII8Str name,ESCCDriver* driver):
	IOCommandFullDuplexDriver(name),
	Stream(name)
{
	readCommand = writeCommand = nil;
	if(!device->logicalAddr)
		device->logicalAddr = (esccDevice*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	deviceRegs = device->logicalAddr;
	channel = _channel->otherInfo;
	nkVideo << "New SCC device at " << (UInt32)deviceRegs << "(Channel " << (UInt32)channel << ")\n";
	switch(channel)
	{
		case channelA:
			channelRegs = &deviceRegs->channelA;
			_channel->logicalAddr = channelRegs;
		break;
		case channelB:
			channelRegs = &deviceRegs->channelB;
			_channel->logicalAddr = channelRegs;
		break;
	}
	nkVideo << "New SCC channel at " << (UInt32)channelRegs << ", with interrupt " << (UInt32)_channel->interrupts[0] << "\n";
	myDriver = driver;
}

ESCCChannel::~ESCCChannel()
{
}

void ESCCChannel::initialize()
{
	// Reset the chip
	switch(channel)
	{
		case channelA:
			writeRegister(9,0x80);	// Reset channel A
		break;
		case channelB:
			writeRegister(9,0x40);	// Reset channel B
		break;
	}
	
	// Set up for x16 clock, 1 stop bit and no parity
	writeRegister(4,0x44);
	
	// Select 8 bits/character for receiver
	writeRegister(3,0xC0);
	
	// Select 8 bits/character, DTR, RTS
	writeRegister(5,0xE2);
	
	// Disable interrupts
	writeRegister(1,0x00);
	writeRegister(15,0x00);
	
	// Set up time constant for baudrate generator
	UInt16	tc = BaudRateToTimeConstant(9600);
	writeRegister(13,(tc >> 8));
	writeRegister(12,tc);
	
	// Enable baudrate generator
	writeRegister(14,0x01);
	
	// Set clock to baudrate generator
	writeRegister(11,0x50);
	
	// Enable transmitter
	writeRegister(5,0xEA);
	
	// Enable receiver, using 8 bits/character
	writeRegister(3,0xC1);
}

void ESCCChannel::start()
{
	// Enable interrupt on all received characters
	//writeRegister(0,0x20);
	
	// Enable external interrupt and interrupt on all transmitted/received characters
	writeRegister(1,0x13);
	
	// Master interrupt enable, no interrupt vector
	writeRegister(9,0x0A);
	
	myDriver->enable();
}

void ESCCChannel::stop()
{
	myDriver->disable();
	
	// Disable master interrupt
	writeRegister(9,0);
	
	// Disable external interrupt and interrupt on all transmitted/received characters
	writeRegister(1,0);
	
	// Disable interrupts
	writeRegister(0,0x00);
}

void ESCCChannel::handleInterrupt()
{
	UInt8	rr3 = readChannelRegister(channelA,3);
	UInt8	intPending = ( (channel == channelA) ? ((rr3 >> 3) & 0x07) : (rr3 & 0x07));

	while(intPending)
	{
		// Receive char available - this must execute FAST, or else we can drop characters from the
		// tiny SCC FIFO.
		if(intPending & 0x04)
		{
			// See if a character is here
			while(readRegister(0) & 0x01)
			{
				// Eat the character
				UInt8	theChar = ReadUReg8(&channelRegs->data);
				
				// Pipe the data
				pipeData((Ptr)&theChar,1);
				
				// Check for errors
				UInt8	rr1 = readRegister(1);
				if(rr1 & 0x70)
				{
					nkVideo << "    Clearing error conditions (" << ((UInt32)rr1 & 0x00000070) << ")\n";
					writeRegister(0,0x30);	// Clear the errors
					if(readCommand)
					{
						readCommand->result = -1;
						readCommand->doneIO();
					}
				}
				
				if(readCommand)
				{
					{
						ProcessWindow	window(readCommand->process);
						*(UInt8*)readCommand->data++ = theChar;
					}
					if(!--readCommand->len)
					{
						if(readCommand->result == 1)
						{
							readCommand->result = 0;
							readCommand->doneIO();
						}
						readCommand = nil;
						startAsyncIORead(dequeueRead());
					}
				}
			}
		}
		
		// External/Status change?
		if(intPending & 0x01)
		{
			nkVideo << "    External/Status change\n";
			
			if(readRegister(0) & 0x40)	// Tx Underrun
				writeRegister(0,0xC0);	// Reset underrun
			
			writeRegister(0,0x10);	// Reset external interrupt
			writeRegister(0,0x30);	// Reset any errors
			writeRegister(0,0x38);	// Reset highest IUS
		}
		
		// Transmit buffer empty?
		if(intPending & 0x02)
		{
			// If we have a pending write put it out
			if(writeCommand)
			{
				if(writeCommand->len)
				{
					// Wait for an empty spot
					while(!(readRegister(0) & 0x04))
						;
					
					ProcessWindow	window(writeCommand->process);
					WriteUReg8(*(UInt8*)writeCommand->data++,&channelRegs->data);
					writeCommand->len--;
				}
				if(!writeCommand->len)
				{
					if(writeCommand->result == 1)
					{
						writeCommand->result = 0;
						writeCommand->doneIO();
					}
					writeCommand = nil;
					
					IOCommand*	cmd = dequeueWrite();
					if(cmd)
					{
						ProcessWindow	window(cmd->process);
						startAsyncIOWrite(cmd);
					}
				}
			}
			
			if(!writeCommand)
			{
				// Reset Tx Int Pending
				writeRegister(0,0x28);
			}
		}
		
		rr3 = readChannelRegister(channelA,3);
		intPending = ( (channel == channelA) ? ((rr3 >> 3) & 0x07) : (rr3 & 0x07));
	}
}

void ESCCChannel::startAsyncIORead(IOCommand* cmd)
{
	if(cmd)
		readCommand = static_cast<ESCCIOCommand*>(cmd);	// Wait for Rx interrupts to come along now
}

void ESCCChannel::startAsyncIOWrite(IOCommand* cmd)
{
	if(cmd)
	{
		NKCriticalSection	critical;
		ESCCIOCommand*	theCmd = static_cast<ESCCIOCommand*>(cmd);
		writeCommand = theCmd;
		
		// Wait for a free spot in the buffer
		while(!(readRegister(0) & 0x04))
			;
			
		// Write out the first character - subsequent characters will be interrupt driven
		UInt8	theChar = *writeCommand->data++;
		writeCommand->len--;
		WriteUReg8(theChar,&channelRegs->data);
	}
}

void ESCCChannel::read(Ptr data,UInt32 len)
{
	ESCCIOCommand*	cmd = new ESCCIOCommand(ioRead,data,len);
	enqueueRead(cmd);
	CurrThread::blockForIO(cmd);
	delete cmd;
}

void ESCCChannel::write(ConstPtr data,UInt32 len)
{
	ESCCIOCommand*	cmd = new ESCCIOCommand(ioWrite,(Ptr)data,len);
	enqueueWrite(cmd);
	CurrThread::blockForIO(cmd);
	delete cmd;
}

void ESCCChannel::writeChannelRegister(UInt8 ch,UInt8 n,UInt8 v)
{
	CriticalSection		critical(deviceLock);
	
	switch(ch)
	{
		case channelA:
			WriteUReg8(n,&deviceRegs->channelA.command);
			WriteUReg8(v,&deviceRegs->channelA.command);
		break;
		case channelB:
			WriteUReg8(n,&deviceRegs->channelB.command);
			WriteUReg8(v,&deviceRegs->channelB.command);
		break;
	}
}

UInt8 ESCCChannel::readChannelRegister(UInt8 ch,UInt8 n)
{
	CriticalSection		critical(deviceLock);
	
	switch(ch)
	{
		case channelA:
			WriteUReg8(n,&deviceRegs->channelA.command);
			return ReadUReg8(&deviceRegs->channelA.command);
		break;
		case channelB:
			WriteUReg8(n,&deviceRegs->channelB.command);
			return ReadUReg8(&deviceRegs->channelB.command);
		break;
	}
	return 0;
}

void ESCCChannel::writeRegister(UInt8 n,UInt8 v)
{
	writeChannelRegister(channel,n,v);
}

UInt8 ESCCChannel::readRegister(UInt8 n)
{
	return readChannelRegister(channel,n);
}

ESCCIOCommand::ESCCIOCommand(UInt32 _op,Ptr _data,UInt32 _len)
{
	operation = _op;
	data = _data;
	len = _len;
	result = 1;
}

UInt32 ESCCIOCommand::ioError()
{
	return result;
}

ESCCDriver::ESCCDriver(MachineDevice<esccDevice>* device,MachineDevice<esccChannel>* deviceChannelA,MachineDevice<esccChannel>* deviceChannelB):
	InterruptHandler(deviceChannelA->interrupts[0],(deviceChannelA->interrupts[0] != deviceChannelB->interrupts[0] ? deviceChannelB->interrupts[0] : PMAC_DEV_NO_INT))
{
	channelA = new ESCCChannel(device,deviceChannelA,"ESCC Channel A (Modem)",this);
	channelB = new ESCCChannel(device,deviceChannelB,"ESCC Channel B (Printer)",this);
}

void ESCCDriver::handleInterrupt()
{
	channelA->handleInterrupt();
	channelB->handleInterrupt();
}