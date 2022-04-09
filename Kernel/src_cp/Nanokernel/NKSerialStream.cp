#include "NKSerialStream.h"
#include "NKVirtualMemory.h"
#include "ANSI.h"

static void	WriteESCCChannelRegister(esccDevice* dev,UInt8 ch,UInt8 n,UInt8 v);
static UInt8	ReadESCCChannelRegister(esccDevice* dev,UInt8 ch,UInt8 n);

static void	NKSerialStreamWrite(NKStream& s,ASCII8 c);
static void	NKSerialStreamMessage(NKStream& s,const StreamMessage& m);
static ASCII8	NKSerialStreamRead(NKStream& s);
static Boolean	NKSerialStreamCharAvailable(NKStream& s);
static void	NKSerialStreamMoveTo(NKSerialStream& s,UInt32 x,UInt32 y);
static void	NKSerialStreamMoved(NKSerialStream& s,UInt32 x,UInt32 y);

void NKInitSerialStream(NKSerialStream& s,esccDevice* sccRegs,UInt32 baudRate)
{
	// Map in the registers
	s.sccRegs = (esccDevice*)NKIOMap(sccRegs,sizeof(esccDevice),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// Reset channel A
	WriteESCCChannelRegister(s.sccRegs,channelA,9,0x80);
	
	// Set up for x16 clock, 1 stop bit and no parity
	WriteESCCChannelRegister(s.sccRegs,channelA,4,0x44);
	
	// Select 8 bits/character for receiver
	WriteESCCChannelRegister(s.sccRegs,channelA,3,0xC0);
	
	// Select 8 bits/character, DTR, RTS
	WriteESCCChannelRegister(s.sccRegs,channelA,5,0xE2);
	
	// Disable interrupts
	WriteESCCChannelRegister(s.sccRegs,channelA,1,0x00);
	WriteESCCChannelRegister(s.sccRegs,channelA,15,0x00);
	
	// Set up time constant for baudrate generator
	UInt16	tc = BaudRateToTimeConstant(baudRate);
	WriteESCCChannelRegister(s.sccRegs,channelA,13,(tc >> 8));
	WriteESCCChannelRegister(s.sccRegs,channelA,12,tc);
	
	// Enable baudrate generator
	WriteESCCChannelRegister(s.sccRegs,channelA,14,0x01);
	
	// Set clock to baudrate generator
	WriteESCCChannelRegister(s.sccRegs,channelA,11,0x50);
	
	// Enable transmitter
	WriteESCCChannelRegister(s.sccRegs,channelA,5,0xEA);
	
	// Enable receiver, using 8 bits/character
	WriteESCCChannelRegister(s.sccRegs,channelA,3,0xC1);
	
	s.read = NKSerialStreamRead;
	s.msg = NKSerialStreamMessage;
	s.write = NKSerialStreamWrite;
	s.charAvailable = NKSerialStreamCharAvailable;
	s.moveTo = NKSerialStreamMoveTo;
	s.moved = NKSerialStreamMoved;
}

void NKSerialStreamWrite(NKStream& s,ASCII8 c)
{
	NKSerialStream&	_s = static_cast<NKSerialStream&>(s);
	
	// Wait for a free spot in the buffer
	while(!(ReadESCCChannelRegister(_s.sccRegs,channelA,0) & 0x04))
		;
	
	// Write out the character
	WriteUReg8(c,&_s.sccRegs->channelA.data);
	
	if(c == '\n')
	{
		while(!(ReadESCCChannelRegister(_s.sccRegs,channelA,0) & 0x04))
			;
		WriteUReg8('\r',&_s.sccRegs->channelA.data);
		_s.moved(_s,1,_s.y+1);
	}
	else if(c == '\r')
	{
		while(!(ReadESCCChannelRegister(_s.sccRegs,channelA,0) & 0x04))
			;
		WriteUReg8('\n',&_s.sccRegs->channelA.data);
		_s.moved(_s,1,_s.y+1);
	}
	else
		_s.moved(_s,_s.x+1,_s.y);
}

void NKSerialStreamMessage(NKStream&,const StreamMessage&)
{
}

ASCII8 NKSerialStreamRead(NKStream& s)
{
	NKSerialStream&	_s = static_cast<NKSerialStream&>(s);
	
	// Wait for a character to arrive
	while(!(ReadESCCChannelRegister(_s.sccRegs,channelA,0) & 0x01))
		;
	
	// Eat the character
	ASCII8	inputChar = ReadUReg8(&_s.sccRegs->channelA.data);
	
	// Check for errors
	if(ReadESCCChannelRegister(_s.sccRegs,channelA,1) & 0x70)
		WriteESCCChannelRegister(_s.sccRegs,channelA,0,0x30);	// Clear errors
	
	return inputChar;
}

Boolean NKSerialStreamCharAvailable(NKStream& s)
{
	NKSerialStream&	_s = static_cast<NKSerialStream&>(s);
	
	if(ReadESCCChannelRegister(_s.sccRegs,channelA,0) & 0x01)
		return true;
	
	return false;
}

void NKSerialStreamMoveTo(NKSerialStream& s,UInt32 x,UInt32 y)
{
	if(s.x != x || s.y != y)
	{
		ASCII8	numStr[20];
		s << "\033[";
		unum2str(y,numStr);
		s << numStr << ";";
		unum2str(x,numStr);
		s << numStr << "H";
		s.x = x;
		s.y = y;
	}
}

void NKSerialStreamMoved(NKSerialStream& s,UInt32 x,UInt32 y)
{
	s.x = x;
	s.y = y;
}

void WriteESCCChannelRegister(esccDevice* dev,UInt8 ch,UInt8 n,UInt8 v)
{
	switch(ch)
	{
		case channelA:
			WriteUReg8(n,&dev->channelA.command);
			WriteUReg8(v,&dev->channelA.command);
		break;
		case channelB:
			WriteUReg8(n,&dev->channelB.command);
			WriteUReg8(v,&dev->channelB.command);
		break;
	}
}

UInt8 ReadESCCChannelRegister(esccDevice* dev,UInt8 ch,UInt8 n)
{
	switch(ch)
	{
		case channelA:
			WriteUReg8(n,&dev->channelA.command);
			return ReadUReg8(&dev->channelA.command);
		break;
		case channelB:
			WriteUReg8(n,&dev->channelB.command);
			return ReadUReg8(&dev->channelB.command);
		break;
	}
	return 0;
}

static void	NKSerialStreamTextBoxWrite(NKStream& s,ASCII8 c);
static void	NKSerialStreamTextBoxNewLine(NKSerialStreamTextBox& s);

void NKInitSerialStreamTextBox(NKSerialStreamTextBox& b,NKSerialStream& s,UInt32 x1,UInt32 y1,UInt32 x2,UInt32 y2)
{
	b.serial = s;
	for(UInt32 i=0;i<40;i++)
		for(UInt32 j=0;j<100;j++)
			b.c[i][j] = ' ';
	b.linesUsed = 0;
	b.x1 = x1;
	b.y1 = y1;
	b.x2 = x2;
	b.y2 = y2;
	b.x = b.x1;
	b.y = b.y1;
	
	b.write = NKSerialStreamTextBoxWrite;
	b.msg = nil;
	b.read = nil;
}

void NKSerialStreamTextBoxWrite(NKStream& _s,ASCII8 c)
{
	NKSerialStreamTextBox&	s = static_cast<NKSerialStreamTextBox&>(_s);
	
	if(isprint(c))
	{
		s.serial.moveTo(s.serial,s.x,s.y);
		s.serial.write(s.serial,c);
		s.c[s.y][s.x] = c;
		s.x++;
		if(s.x >= s.x2)
			NKSerialStreamTextBoxNewLine(s);
	}
	else if(c == '\r' || c == '\n')
		NKSerialStreamTextBoxNewLine(s);
	else
		s.serial.write(s.serial,c);
}

void NKSerialStreamTextBoxNewLine(NKSerialStreamTextBox& s)
{
	s.x = s.x1;
	if(s.y == s.y2-1)
	{
		for(UInt32 i=s.y1;i<s.y2-1;i++)
			for(UInt32 j=s.x1;j<s.x2;j++)
				s.c[i][j] = s.c[i+1][j];
		for(UInt32 i=s.x1;i<s.x2;i++)
			s.c[s.y2-1][i] = ' ';
		for(UInt32 i=s.y1;i<s.y2;i++)
		{
			s.serial.moveTo(s.serial,s.x1,i);
			for(UInt32 j=s.x1;j<s.x2;j++)
				s.serial.write(s.serial,s.c[i][j]);
		}
	}
	else
		s.y++;
}
