/*
	LL Network Protocol.h
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
	Terry Greeniaus	-	Thursday, 21 Oct. 98	-	Original creation of file
*/
#ifndef __LL_NETWORK_PROTOCOL__
#define __LL_NETWORK_PROTOCOL__

enum
{
	// Defines different types of media
	mediaEthernet	=	1,	// Ethernet port
	mediaSerial	=	2	// Serial port
};

enum
{
	// Defines different types of physical addresses
	addressEthernet	=	1	// An Ethernet address
};

enum
{
	// Different types of low-level network protocols
	
	// **** Ethernet protocols ****
	protocolEthernet2	=	1,	// Ethernet-2 protocol
	protocolIEEE8023	=	2	// IEEE 802.3 protocol
};

class LLNetworkAddress
{
	UInt32	addressType;
protected:
	LLNetworkAddress(UInt32 addressType);
public:
			UInt64	type();			// Returns addressType
	virtual	UInt64	address() = 0;		// Returns this address as a UInt64
};

class LLNetworkProtocol
{
	UInt32					protocolType;
	class LLNetworkProtocol*		next;
};

class LLPacketNetworkProtocol
{
	UInt32					protocolType;
	class LLNetworkProtocol*		next;
protected:
	LLNetworkProtocol(protocolType);
	
	// For receiving data from the stream.
	virtual	UInt32	identifyBytesRequired()	=	0;	// Return the number of bytes required to identify if a particular input source
												// (packet, stream, etc.) has data in your protocol format.
	virtual	Boolean	identifyProtocol(Ptr data)	=	0;	// Return true if the data is in your protocol format.  identifyBytesRequired() bytes will
												// be available in data.
	virtual	Boolean	dmaIn(Ptr* data,UInt32* count)	=	0;	// Set data and count to the address and length of how much data you want to read.
												// If you return true, it will be read in.  If you return false, count bytes will be ignored.
public:
	virtual	void		read(Ptr data,UInt32 len)	=	0;	// Reads len bytes into data (after stripping any headers)
};

#endif