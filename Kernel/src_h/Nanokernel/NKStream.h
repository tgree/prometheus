#ifndef __NK_STREAM__
#define __NK_STREAM__

typedef void (*NKStreamWriteProcPtr)(struct NKStream& s,ASCII8 c);
typedef void (*NKStreamMessageProcPtr)(struct NKStream& s,const class StreamMessage& msg);
typedef ASCII8 (*NKStreamReadProcPtr)(struct NKStream& s);
typedef Boolean (*NKCharAvailableProcPtr)(struct NKStream& s);

// Have to make a stream in this manner, because at nanokernel time virtual function tables have not
// been set up correctly.  (???)
struct NKStream
{
	NKStreamWriteProcPtr	write;
	NKStreamMessageProcPtr	msg;
	NKStreamReadProcPtr	read;
	NKCharAvailableProcPtr	charAvailable;
	UInt32				baseMode;
};

enum
{
	baseModeDec	=	0,
	baseModeHex	=	1
};

NKStream& operator<<(NKStream& s,ConstASCII8Str str);
NKStream& operator<<(NKStream& s,UInt32 n);
NKStream& operator<<(NKStream& video,const class StreamMessage& message);
NKStream& operator>>(NKStream& s,ASCII8Str str);
NKStream& operator>>(NKStream& s,UInt32& n);

#endif /* __NK_STREAM__ */