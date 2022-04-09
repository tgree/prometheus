/*
	Streams.h
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
#ifndef __STREAMS__
#define __STREAMS__

#include "Kernel Types.h"
#include "NKInterruptSafeList.h"

struct StreamMessage
{
	UInt32	type;
	UInt32	data;
};

enum
{
	// Types for StreamMessage
	nullMessage	=	0,	// No message
	colorMessage	=	1,	// Change color
	hexMessage	=	2,	// Set to hex mode
	decMessage	=	3,	// Set to decimal mode
	lineMessage	=	4	// An appropriate newline character(s) for this stream
};

class Stream
{
	ConstASCII8Str		_name;
protected:
	Stream();	// Never call this explicitly
	Stream(ConstASCII8Str name);
	virtual ~Stream();
public:
	ConstASCII8Str		name();
};

struct PipeDestination	:	public KernelObject
{
	class Process*		process;
	class OStream*	stream;
};

class IStream	:	virtual	public	Stream
{
	//NKInterruptSafeList<PipeDestination>		pipeList;
	PipeDestination*		pipeDest;
	
	// Note: reading of ints (Int) is not allowed.
protected:
	IStream();
	virtual ~IStream();
	
	// Overload these to do something unusual - the defaults are pretty standard stuff
	virtual	void			read_str(ASCII8Str str);	// Reads until it hits a '\0'
	virtual	void			read_int8(Int8& n);
	virtual	void			read_uint8(UInt8& n);
	virtual	void			read_int16(Int16& n);
	virtual	void			read_uint16(UInt16& n);
	virtual	void			read_int32(Int32& n);
	virtual	void			read_uint32(UInt32& n);
	virtual	void			read_int64(Int64& n);
	virtual	void			read_uint64(UInt64& n);
	
	// Call this routine when the stream receives data for piping
			void			pipeData(Ptr data,UInt32 len);
public:
			IStream&		operator>>(ASCII8Str str)	{read_str(str); return *this;}
			IStream&		operator>>(Int8& n)			{read_int8(n); return *this;}
			IStream&		operator>>(UInt8& n)		{read_uint8(n); return *this;}
			IStream&		operator>>(Int16& n)		{read_int16(n); return *this;}
			IStream&		operator>>(UInt16& n)		{read_uint16(n); return *this;}
			IStream&		operator>>(Int32& n)		{read_int32(n); return *this;}
			IStream&		operator>>(UInt32& n)		{read_uint32(n); return *this;}
			IStream&		operator>>(Int64& n)		{read_int64(n); return *this;}
			IStream&		operator>>(UInt64& n)		{read_uint64(n); return *this;}
	
	virtual	void			read(Ptr data,UInt32 len)	=	0;
	
	virtual	void			createPipe(class OStream* dest);	// This should only be overloaded in very special circumstances (IStreamWrapper)
	virtual	void			disposePipe(class OStream* dest);	// This should only be overloaded in very special circumstances (IStreamWrapper)
	
	friend class IStreamWrapper;
};

class OStream	:	virtual	public	Stream
{
	// Note: writing of ints is allowed, but only do it implicitly please.  An int will
	// take up a 4 byte value, regardless of the size of an int on your compiler, so
	// be sure to read it back with an Int32 if its going into storage.
	//
	//		cout << "One = " << 1 << "\n";
	//
protected:
	// Overload these to do something unusual - the defaults are pretty standard stuff
	virtual	void			write_str(ConstASCII8Str str);	// Writes the '\0'
	virtual	void			write_int8(const Int8& n);
	virtual	void			write_uint8(const UInt8& n);
	virtual	void			write_int16(const Int16& n);
	virtual	void			write_uint16(const UInt16& n);
	virtual	void			write_int32(const Int32& n);
	virtual	void			write_uint32(const UInt32& n);
	virtual	void			write_int64(const Int64& n);
	virtual	void			write_uint64(const UInt64& n);
	virtual	void			handle_message(const StreamMessage& message);
public:
			OStream&		operator<<(ConstASCII8Str str)		{write_str(str); return *this;}
			OStream&		operator<<(const Int& n)				{Int32 temp = n; write_int32(temp); return *this;}
			OStream&		operator<<(const Int8& n)				{write_int8(n); return *this;}
			OStream&		operator<<(const UInt8& n)			{write_uint8(n); return *this;}
			OStream&		operator<<(const Int16& n)			{write_int16(n); return *this;}
			OStream&		operator<<(const UInt16& n)			{write_uint16(n); return *this;}
			OStream&		operator<<(const Int32& n)			{write_int32(n); return *this;}
			OStream&		operator<<(const UInt32& n)			{write_uint32(n); return *this;}
			OStream&		operator<<(const Int64& n)			{write_int64(n); return *this;}
			OStream&		operator<<(const UInt64& n)			{write_uint64(n); return *this;}
			OStream&		operator<<(const StreamMessage& m)	{handle_message(m); return *this;}
	virtual	void			write(ConstPtr data,UInt32 len) = 0;
	friend class OStreamWrapper;
};

class IOStream	:	virtual	public	IStream,
				virtual	public	OStream
{
protected:
	IOStream();
};

class SeekableStream	:	virtual	public	Stream
{
protected:
	SeekableStream();
public:
	virtual	UInt64	getPos()				=	0;	// Returns the position in the stream
	virtual	void		setPos(UInt64 newPos)	=	0;	// Sets a new position in the stream
	virtual	UInt64	eos()					=	0;	// Returns the length of the stream (seekable streams generally have a well-defined length)
			void		advance(Int64 disp);				// Advances the stream pointer (- goes backwards)
};

class SeekableIStream	:	virtual	public	SeekableStream,
						virtual	public	IStream
{
protected:
	SeekableIStream();
};

class SeekableOStream	:	virtual	public	SeekableStream,
						virtual	public	OStream
{
protected:
	SeekableOStream();
};

class SeekableIOStream	:			public	IOStream,
						virtual	public	SeekableIStream,
						virtual	public	SeekableOStream
{
protected:
	SeekableIOStream();
};

class ASCIIIStream	:	virtual	public	IStream
{
	virtual	void			read_str(ASCII8Str str);	// Reads until it hits a !isprint
	virtual	void			read_int8(Int8& n);		// Reads an ASCII string into a value.  $, 0x specify hex, 0b specify binary, anything else is decimal.
	virtual	void			read_uint8(UInt8& n);	// ditto
	virtual	void			read_int16(Int16& n);	// ditto
	virtual	void			read_uint16(UInt16& n);	// etc.
	virtual	void			read_int32(Int32& n);
	virtual	void			read_uint32(UInt32& n);
	virtual	void			read_int64(Int64& n);
	virtual	void			read_uint64(UInt64& n);
};

class ASCIIOStream	:	virtual	public	OStream
{
	UInt8				base;				// The base used for number output (currently only 10 and 16 are supported)
	
	virtual	void			write_str(ConstASCII8Str str);	// Doesn't write the '\0'
	virtual	void			write_int8(const Int8& n);		// Writes a value as an ASCII string.
	virtual	void			write_uint8(const UInt8& n);		// ditto
	virtual	void			write_int16(const Int16& n);		// ditto
	virtual	void			write_uint16(const UInt16& n);	// etc.
	virtual	void			write_int32(const Int32& n);
	virtual	void			write_uint32(const UInt32& n);
	virtual	void			write_int64(const Int64& n);
	virtual	void			write_uint64(const UInt64& n);
	
protected:
	ASCIIOStream();
	
	virtual	void			handle_message(const StreamMessage& m);	// Handles hexMsg and decMsg messages
};

class ASCIIIOStream	:	public ASCIIIStream,
					public ASCIIOStream,
					virtual public IOStream
{
protected:
	ASCIIIOStream();
};

class OStreamWrapper	:	virtual	public	OStream
{
	OStream*			ostream;
protected:
	OStreamWrapper()											{ostream = nil;}
	
	virtual	void			write_str(ConstASCII8Str str)				{if(ostream) ostream->write_str(str);}
	virtual	void			write_int8(const Int8& n)					{if(ostream) ostream->write_int8(n);}
	virtual	void			write_uint8(const UInt8& n)				{if(ostream) ostream->write_uint8(n);}
	virtual	void			write_int16(const Int16& n)				{if(ostream) ostream->write_int16(n);}
	virtual	void			write_uint16(const UInt16& n)				{if(ostream) ostream->write_uint16(n);}
	virtual	void			write_int32(const Int32& n)				{if(ostream) ostream->write_int32(n);}
	virtual	void			write_uint32(const UInt32& n)				{if(ostream) ostream->write_uint32(n);}
	virtual	void			write_int64(const Int64& n)				{if(ostream) ostream->write_int64(n);}
	virtual	void			write_uint64(const UInt64& n)				{if(ostream) ostream->write_uint64(n);}
	virtual	void			handle_message(const StreamMessage& m)	{if(ostream) ostream->handle_message(m);}
public:
	OStreamWrapper(ConstASCII8Str name):Stream(name)		{}
	
	virtual	void			write(ConstPtr data,UInt32 len)	{if(ostream) ostream->write(data,len);}
			void			setOStream(OStream* s)			{ostream = s;}
};

class IStreamWrapper	:	virtual	public	IStream
{
	IStream*			istream;
protected:
	IStreamWrapper()									{istream = nil;}
	
	virtual	void			read_str(ASCII8Str str)			{if(istream) istream->read_str(str);}
	virtual	void			read_int8(Int8& n)				{if(istream) istream->read_int8(n);}
	virtual	void			read_uint8(UInt8& n)			{if(istream) istream->read_uint8(n);}
	virtual	void			read_int16(Int16& n)			{if(istream) istream->read_int16(n);}
	virtual	void			read_uint16(UInt16& n)			{if(istream) istream->read_uint16(n);}
	virtual	void			read_int32(Int32& n)			{if(istream) istream->read_int32(n);}
	virtual	void			read_uint32(UInt32& n)			{if(istream) istream->read_uint32(n);}
	virtual	void			read_int64(Int64& n)			{if(istream) istream->read_int64(n);}
	virtual	void			read_uint64(UInt64& n)			{if(istream) istream->read_uint64(n);}
public:
	IStreamWrapper(ConstASCII8Str name):Stream(name)		{}
	
	virtual	void			read(Ptr data,UInt32 len)			{if(istream) istream->read(data,len);}
	virtual	void			createPipe(OStream* dest)		{if(istream) istream->createPipe(dest);}
	virtual	void			disposePipe(OStream* dest)		{if(istream) istream->disposePipe(dest);}
			void			setIStream(IStream* s)			{istream = s;}
};

struct IOStreamWrapper	:			public	IOStream,
						virtual	public	IStreamWrapper,
						virtual	public	OStreamWrapper
{
public:
	IOStreamWrapper(ConstASCII8Str name):Stream(name)		{}
	
			void			setStream(IOStream* s)			{setOStream(s); setIStream(s);}
};

// cout/cin
extern OStreamWrapper 	cout;
extern IStreamWrapper	cin;

// StreamMessages
extern StreamMessage	redMsg;
extern StreamMessage	greenMsg;
extern StreamMessage	whiteMsg;
extern StreamMessage	decMsg;
extern StreamMessage	hexMsg;
extern StreamMessage	newLine;

#endif /*__STREAMS__*/