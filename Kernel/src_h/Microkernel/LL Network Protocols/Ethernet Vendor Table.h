/*
	Ethernet Vendor Table.h
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
	Version History
	============
	Terry Greeniaus	-	Sunday, 1 November 1998	-	Original creation of file
*/
#ifndef __ETHERNET_VENDOR_TABLE__
#define __ETHERNET_VENDOR_TABLE__

typedef struct EthernetVendor
{
	UInt32			vendorID;		// The 24-bit vendor ID
	ConstASCII8Str		name;		// The name of the vendor
	ConstASCII8Str		info;			// Other info (may be nil)
}EthernetVendor;

extern EthernetVendor	ethernetVendorTable[];

void	SearchEthernetTable(UInt32 vendorID,ConstASCII8Str* name,ConstASCII8Str* info);

#endif /* __ETHERNET_VENDOR_TABLE__ */