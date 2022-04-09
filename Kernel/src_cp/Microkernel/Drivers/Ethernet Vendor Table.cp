/*
	Ethernet Vendor Table.cp
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
#include "Ethernet Vendor Table.h"

EthernetVendor	ethernetVendorTable[] = {	{0x0000000C,"Cisco",nil},
								{0x0000000E,"Fujitsu",nil},
								{0x0000000F,"NeXT",nil},
								{0x00000010,"Sytek",nil},
								{0x0000001D,"Cabletron",nil},
								{0x00000020,"DIAB","Data Intdustrier AB"},
								{0x00000022,"Visual Technology",nil},
								{0x0000002A,"TRW",nil},
								{0x00000032,"GPT Limited","Reassigned from GEC Computers Ltd."},
								{0x0000005A,"S & Koch",nil},
								{0x0000005E,"IANA",nil},
								{0x00000065,"Network General",nil},
								{0x0000006B,"MIPS",nil},
								{0x00000077,"MIPS",nil},
								{0x0000007A,"Ardent",nil},
								{0x00000089,"Cayman Systems","Gatorbox"},
								{0x00000093,"Proteon",nil},
								{0x0000009F,"Ameristar Technology",nil},
								{0x000000A2,"Wellfleet",nil},
								{0x000000A3,"Network Application Technology",nil},
								{0x000000A6,"Network General","Internal assignment, not for products"},
								{0x000000A7,"NCD","X-terminals"},
								{0x000000A9,"Network Systems",nil},
								{0x000000AA,"Xerox","Xerox Machines"},
								{0x000000B3,"CIMLinc",nil},
								{0x000000B7,"Dove","Fastnet"},
								{0x000000BC,"Allen-Bradley",nil},
								{0x000000C0,"Western Digital",nil},
								{0x000000C5,"Farallong phone net card",nil},
								{0x000000C6,"HP Intelligent Networks Operation","Formerly Eon Systems"},
								{0x000000C8,"Altos",nil},
								{0x000000C9,"Emulex","Terminal Servers"},
								{0x000000D7,"Dartmouth College","NED Router"},
								{0x000000D8,"3Com? Novell?","PS/2"},
								{0x000000DD,"Gould",nil},
								{0x000000DE,"Unigraph",nil},
								{0x000000E2,"Acer Counterpoint",nil},
								{0x000000EF,"Alantec",nil},
								{0x000000FD,"High Level Hardvare","Orion, UK"},
								{0x00000102,"BBN","BBN internal usage (not registered)"},
								{0x00001700,"Kabel",nil},
								{0x000020AF,"3COM???",nil},
								{0x0000802B,"IMAC",nil},
								{0x0000802D,"Xylogics, Inc.","Annex terminal servers"},
								{0x00008064,"Wyse Technology / Link Technologies",nil},
								{0x0000808C,"Frontier Software Development",nil},
								{0x000080C2,"IEEE 802.1 Committee",nil},
								{0x000080D3,"Shiva",nil},
								{0x0000AA00,"Intel",nil},
								{0x0000DD00,"Ungermann-Bass",nil},
								{0x0000DD01,"Ungermann-Bass",nil},
								{0x00020406,"BBN","BBN internal usage (not registered)"},
								{0x00020701,"Racal Interlan",nil},
								{0x00026086,"Satelcom MegaPac","UK"},
								{0x0002608C,"3Com","IBM PC; Imagen; Valid; Cisco"},
								{0x0002CF1F,"CMC","Masscomp; Sililcon Graphics; Prime EXL"},
								{0x00080002,"3Com","Formerly Bridge"},
								{0x00080003,"ACC","Advanced Computer Communications"},
								{0x00080005,"Symbolics","Symbolics LISP machines"},
								{0x00080008,"BBN",nil},
								{0x00080009,"Hewlett-Packard",nil},
								{0x0008000A,"Nestar Systems",nil},
								{0x0008000B,"Unisys",nil},
								{0x00080011,"Tektronix, Inc.",nil},
								{0x00080014,"Excelan","BBN Butterfly, Masscomp, Silicon Graphics"},
								{0x00080017,"NSC",nil},
								{0x0008001A,"Data General",nil},
								{0x0008001B,"Data General",nil},
								{0x0008001E,"Apollo",nil},
								{0x00080020,"Sun","Sun machines"},
								{0x00080022,"NBI",nil},
								{0x00080025,"CDC",nil},
								{0x00080026,"Norsk Data (Nord)",nil},
								{0x00080027,"PCS Computer Systems GmbH",nil},
								{0x00080028,"TI","Explorer"},
								{0x0008002B,"DEC",nil},
								{0x0008002E,"Metaphor",nil},
								{0x0008002F,"Prime Computer","Prime 50-Series LHC300"},
								{0x00080036,"Intergraph","CAE stations"},
								{0x00080037,"Fujitsu-Xerox",nil},
								{0x00080038,"Bull",nil},
								{0x00080039,"Spider Systems",nil},
								{0x00080041,"DCA Digital Comm. Assoc.",nil},
								{0x00080045,"???? (maybe Xylogics, but they claim no to know this number)",nil},
								{0x00080046,"Sony",nil},
								{0x00080047,"Sequent",nil},
								{0x00080049,"Univation",nil},
								{0x0008004C,"Encore",nil},
								{0x0008004E,"BICC",nil},
								{0x00080056,"Stanford University",nil},
								{0x00080058,"???","DECsystem-20"},
								{0x0008005A,"IBM",nil},
								{0x00080067,"Comdesign",nil},
								{0x00080068,"Ridge",nil},
								{0x00080069,"Silicon Graphics",nil},
								{0x0008006E,"Concurrent","Masscomp"},
								{0x00080075,"DDE","Danish Data Elektronik A/S"},
								{0x0008007C,"Vitalink","TransLAN III"},
								{0x00080080,"XIOS",nil},
								{0x00080086,"Imagen/QMS",nil},
								{0x00080087,"Xyplex","Terminal servers"},
								{0x00080089,"Kinetics","AppleTalk-Ethernet interface"},
								{0x0008008B,"Pyramid",nil},
								{0x0008008D,"XyVision","XyVision machines"},
								{0x00080090,"Retix Inc.","Bridges"},
								{0x00484453,"HDS ????","???"},
								{0x00800010,"AT&T",nil},
								{0x00AA0000,"DEC","obsolete"},
								{0x00AA0001,"DEC","obsolete"},
								{0x00AA0002,"DEC","obsolete"},
								{0x00AA0003,"DEC","obsolete"},
								{0x00AA0004,"DEC","obsolete"}
							};

void SearchEthernetTable(UInt32 vendorID,ConstASCII8Str* name,ConstASCII8Str* info)
{
	// This should really be implemented as a binary search, but it isn't, so there.
	for(UInt32 i=0;i<sizeof(ethernetVendorTable)/sizeof(ethernetVendorTable[0]);i++)
	{
		if(ethernetVendorTable[i].vendorID == vendorID)
		{
			*name = ethernetVendorTable[i].name;
			*info = ethernetVendorTable[i].info;
			return;
		}
	}
	
	*name = "Unknown";
	*info = nil;
}
