/*
	Streams.cp
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
#include "ANSI.h"
#include "Streams.h"
#include "NKProcesses.h"
#include "Video Driver.h"

StreamMessage		redMsg	=	{colorMessage,red8bit};
StreamMessage		greenMsg	=	{colorMessage,green8bit};
StreamMessage		whiteMsg	=	{colorMessage,white8bit};
StreamMessage		decMsg	=	{decMessage,0};
StreamMessage		hexMsg	=	{hexMessage,0};
StreamMessage		newLine	=	{lineMessage,0};

// ***************** Stream ************************
Stream::Stream()
{
	//nkVideo << "Unnamed stream\n";
	_name = "\0";
}

Stream::Stream(ConstASCII8Str name)
{
	_name = name;
	//nkVideo << "Named stream: \"" << name << "\"\n";
}

Stream::~Stream()
{
}

ConstASCII8Str Stream::name()
{
	return _name;
}

// ***************** IStream ************************
IStream::IStream()
{
	pipeDest = new PipeDestination;
	pipeDest->stream = nil;
	pipeDest->process = nil;
}

IStream::~IStream()
{
	delete pipeDest;
}

void IStream::read_str(ASCII8Str str)
{
	do
	{
		read((Ptr)str,1);
	}while(*str++);
}

void IStream::read_int8(Int8& n)
{
	read((Ptr)&n,sizeof(n));
}


void IStream::read_uint8(UInt8& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_int16(Int16& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_uint16(UInt16& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_int32(Int32& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_uint32(UInt32& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_int64(Int64& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::read_uint64(UInt64& n)
{
	read((Ptr)&n,sizeof(n));
}

void IStream::pipeData(Ptr data,UInt32 len)
{
	/*
	for(UInt32 i = 0;i<pipeList.numElems();i++)
	{
		PipeDestination*	pipeDest = pipeList[i];
		ProcessWindow		window(pipeDest->process);
		pipeDest->stream->write(data,len);
	}
	*/
	if(pipeDest->stream)
	{
		ProcessWindow		window(pipeDest->process);
		pipeDest->stream->write(data,len);
	}
}

void IStream::createPipe(OStream* dest)
{
	/*
	PipeDestination*	pipeDest = new PipeDestination;
	pipeDest->process = CurrProcess::process();
	pipeDest->stream = dest;
	
	pipeList.enqueue(pipeDest);
	*/
	if(!pipeDest->stream)
	{
		pipeDest->process = CurrProcess::process();
		pipeDest->stream = dest;
	}
}

void IStream::disposePipe(OStream* dest)
{
	/*
	for(UInt32 i = 0;i<pipeList.numElems();i++)
	{
		PipeDestination*	pipeDest = pipeList[i];
		if(pipeDest->stream == dest)
		{
			pipeList.dequeue(pipeDest);
			delete pipeDest;
			break;
		}
	}
	*/
	if(pipeDest->stream == dest)
	{
		pipeDest->stream = nil;
		pipeDest->process = nil;
	}
}

// ***************** OStream ************************
void OStream::write_str(ConstASCII8Str str)
{
	write((Ptr)str,strlen(str) + 1);
}

void OStream::write_int8(const Int8& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_uint8(const UInt8& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_int16(const Int16& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_uint16(const UInt16& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_int32(const Int32& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_uint32(const UInt32& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_int64(const Int64& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::write_uint64(const UInt64& n)
{
	write((Ptr)&n,sizeof(n));
}

void OStream::handle_message(const StreamMessage&)
{
}

// ***************** IOStream ************************
IOStream::IOStream()
{
}

// ***************** SeekableStream ************************
SeekableStream::SeekableStream()
{
}

void SeekableStream::advance(Int64 disp)
{
	setPos(getPos() + disp);
}

// ***************** SeekableIStream ************************
SeekableIStream::SeekableIStream()
{
}

// ***************** SeekableOStream ************************
SeekableOStream::SeekableOStream()
{
}

// ***************** SeekableIOStream ************************
SeekableIOStream::SeekableIOStream()
{
}

// ***************** ASCIIIStream ************************
void ASCIIIStream::read_str(ASCII8Str )
{
	Panic("ASCIIIStream::read_str() not implemented!\n");
}

void ASCIIIStream::read_int8(Int8& )
{
	Panic("ASCIIIStream::read_int8() not implemented!\n");
}

void ASCIIIStream::read_uint8(UInt8& )
{
	Panic("ASCIIIStream::read_uint8() not implemented!\n");
}

void ASCIIIStream::read_int16(Int16& )
{
	Panic("ASCIIIStream::read_int16() not implemented!\n");
}

void ASCIIIStream::read_uint16(UInt16& )
{
	Panic("ASCIIIStream::read_uint16() not implemented!\n");
}

void ASCIIIStream::read_int32(Int32& )
{
	Panic("ASCIIIStream::read_int32() not implemented!\n");
}

void ASCIIIStream::read_uint32(UInt32& )
{
	Panic("ASCIIIStream::read_uint32() not implemented!\n");
}

void ASCIIIStream::read_int64(Int64& )
{
	Panic("ASCIIIStream::read_int64() not implemented!\n");
}

void ASCIIIStream::read_uint64(UInt64& )
{
	Panic("ASCIIIStream::read_uint64() not implemented!\n");
}

// ***************** ASCIIOStream ************************
ASCIIOStream::ASCIIOStream()
{
	base = 10;
}

void ASCIIOStream::write_str(ConstASCII8Str str)
{
	write(str,strlen(str));
}

void ASCIIOStream::write_int8(const Int8& n)
{
	ASCII8	str[12];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
			str[7] = 'x';
			write(&str[6],strlen(&str[6]));
		break;
		default:
			num2str(n,str);
			write(str,strlen(str));
		break;
	}
}

void ASCIIOStream::write_uint8(const UInt8& n)
{
	ASCII8	str[11];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
			str[7] = 'x';
			write(&str[6],strlen(&str[6]));
		break;
		default:
			unum2str(n,str);
			write(str,strlen(str));
		break;
	}
}

void ASCIIOStream::write_int16(const Int16& n)
{
	ASCII8	str[12];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
			str[5] = 'x';
			write(&str[4],strlen(&str[4]));
		break;
		default:
			num2str(n,str);
			write(str,strlen(str));
		break;
	}
}

void ASCIIOStream::write_uint16(const UInt16& n)
{
	ASCII8	str[11];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
			str[5] = 'x';
			write(&str[4],strlen(&str[4]));
		break;
		default:
			unum2str(n,str);
			write(str,strlen(str));
		break;
	}
}

void ASCIIOStream::write_int32(const Int32& n)
{
	ASCII8	str[12];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
		break;
		default:
			num2str(n,str);
		break;
	}
	write(str,strlen(str));
}

void ASCIIOStream::write_uint32(const UInt32& n)
{
	ASCII8	str[11];
	
	switch(base)
	{
		case 16:
			num2hex(n,str);
		break;
		default:
			unum2str(n,str);
		break;
	}
	write(str,strlen(str));
}

void ASCIIOStream::write_int64(const Int64& n)
{
	ASCII8	str[21];
	
	switch(base)
	{
		case 16:
			num2hex64(n,str);
		break;
		default:
			num2str64(n,str);
		break;
	}
	write(str,strlen(str));
}

void ASCIIOStream::write_uint64(const UInt64& n)
{
	ASCII8	str[20];
	
	switch(base)
	{
		case 16:
			num2hex64(n,str);
		break;
		default:
			unum2str64(n,str);
		break;
	}
	write(str,strlen(str));
}

void ASCIIOStream::handle_message(const StreamMessage& message)
{
	switch(message.type)
	{
		case hexMessage:
			base = 16;
		break;
		case decMessage:
			base = 10;
		break;
	}
}

// ***************** ASCIIIOStream ************************
ASCIIIOStream::ASCIIIOStream()
{
}
