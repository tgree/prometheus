#ifndef __NK_SERIAL_STREAM__
#define __NK_SERIAL_STREAM__

#include "NKStream.h"
#include "ESCC.h"

typedef void (*NKSerialStreamMoveToProcPtr)(struct NKSerialStream&,UInt32 x,UInt32 y);
typedef void (*NKSerialStreamMovedProcPtr)(struct NKSerialStream&,UInt32 x,UInt32 y);

struct NKSerialStream	:	public NKStream
{
	esccDevice*					sccRegs;
	UInt32						x;
	UInt32						y;
	NKSerialStreamMoveToProcPtr		moveTo;
	NKSerialStreamMovedProcPtr		moved;
};

struct NKSerialStreamTextBox	:	public NKStream
{
	NKSerialStream	serial;
	ASCII8			c[40][100];
	UInt32			linesUsed;
	UInt32			x1;
	UInt32			y1;
	UInt32			x2;
	UInt32			y2;
	UInt32			x;
	UInt32			y;
};

void NKInitSerialStream(NKSerialStream& s,esccDevice* sccRegs,UInt32 baudRate);
void NKInitSerialStreamTextBox(NKSerialStreamTextBox& b,NKSerialStream& s,UInt32 x1,UInt32 y1,UInt32 x2,UInt32 y2);

#endif /* __NK_SERIAL_STREAM__ */