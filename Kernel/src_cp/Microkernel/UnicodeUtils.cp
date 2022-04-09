/*
	UnicodeUtils.cp
	Copyright � 1999 by Patrick Varilly

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
	Patrick Varilly		-	Tuesday, 30 March 99	-	Creation of file
*/

#include "UnicodeUtils.h"

UInt16			LCTable[] = {
	// High-byte indices ( == 0xFFFF if all values are equal for the whole range)
	/* 0 */	0x0001,	0x0002,	0x0003,	0x0004,	0x0005,	0x0006,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0x0007,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0x0008,	0x0009,
	/* 2 */	0xFFFF,	0x000A,	0xFFFF,	0xFFFF,	0x000B,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0x000C,

	// Table 1 (for high byte 0x00)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0x0061,	0x0062,	0x0063,	0x0064,	0x0065,	0x0066,	0x0067,
			0x0068,	0x0069,	0x006A,	0x006B,	0x006C,	0x006D,	0x006E,	0x006F,
	/* 5 */	0x0070,	0x0071,	0x0072,	0x0073,	0x0074,	0x0075,	0x0076,	0x0077,
			0x0078,	0x0079,	0x007A,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0x00E0,	0x00E1,	0x00E2,	0x00E3,	0x00E4,	0x00E5,	0x00E6,	0x00E7,
			0x00E8,	0x00E9,	0x00EA,	0x00EB,	0x00EC,	0x00ED,	0x00EE,	0x00EF,
	/* D */	0x00F0,	0x00F1,	0x00F2,	0x00F3,	0x00F4,	0x00F5,	0x00F6,	0xFFFF,
			0x00F8,	0x00F9,	0x00FA,	0x00FB,	0x00FC,	0x00FD,	0x00FE,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 2 (for high byte 0x01)
	
	/* 0 */	0x0101,	0xFFFF,	0x0103,	0xFFFF,	0x0105,	0xFFFF,	0x0107,	0xFFFF,
			0x0109,	0xFFFF,	0x010B,	0xFFFF,	0x010D,	0xFFFF,	0x010F,	0xFFFF,
	/* 1 */	0x0111,	0xFFFF,	0x0113,	0xFFFF,	0x0115,	0xFFFF,	0x0117,	0xFFFF,
			0x0119,	0xFFFF,	0x011B,	0xFFFF,	0x011D,	0xFFFF,	0x011F,	0xFFFF,
	/* 2 */	0x0121,	0xFFFF,	0x0123,	0xFFFF,	0x0125,	0xFFFF,	0x0127,	0xFFFF,
			0x0129,	0xFFFF,	0x012B,	0xFFFF,	0x012D,	0xFFFF,	0x012F,	0xFFFF,
	/* 3 */	0x0069,	0xFFFF,	0x0133,	0xFFFF,	0x0135,	0xFFFF,	0x0137,	0xFFFF,
			0xFFFF,	0x013A,	0xFFFF,	0x013C,	0xFFFF,	0x013E,	0xFFFF,	0x0140,
	/* 4 */	0xFFFF,	0x0142,	0xFFFF,	0x0144,	0xFFFF,	0x0146,	0xFFFF,	0x0148,
			0xFFFF,	0xFFFF,	0x014B,	0xFFFF,	0x014D,	0xFFFF,	0x014F,	0xFFFF,
	/* 5 */	0x0151,	0xFFFF,	0x0153,	0xFFFF,	0x0155,	0xFFFF,	0x0157,	0xFFFF,
			0x0159,	0xFFFF,	0x015B,	0xFFFF,	0x015D,	0xFFFF,	0x015F,	0xFFFF,
	/* 6 */	0x0161,	0xFFFF,	0x0163,	0xFFFF,	0x0165,	0xFFFF,	0x0167,	0xFFFF,
			0x0169,	0xFFFF,	0x016B,	0xFFFF,	0x016D,	0xFFFF,	0x016F,	0xFFFF,
	/* 7 */	0x0171,	0xFFFF,	0x0173,	0xFFFF,	0x0175,	0xFFFF,	0x0177,	0xFFFF,
			0x00FF,	0x017A,	0xFFFF,	0x017C,	0xFFFF,	0x017E,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0x0253,	0x0183,	0xFFFF,	0x0185,	0xFFFF,	0x0254,	0x0188,
			0xFFFF,	0x0256,	0x0257,	0x018C,	0xFFFF,	0xFFFF,	0x0258,	0x0259,
	/* 9 */	0x025B,	0x0192,	0xFFFF,	0x0260,	0x0263,	0xFFFF,	0x0269,	0x0268,
			0x0199,	0xFFFF,	0xFFFF,	0xFFFF,	0x026F,	0x0272,	0xFFFF,	0x0275,
	/* A */	0x01A1,	0xFFFF,	0x01A3,	0xFFFF,	0x01A5,	0xFFFF,	0xFFFF,	0x01A8,
			0xFFFF,	0x0283,	0xFFFF,	0xFFFF,	0x01AD,	0xFFFF,	0x0288,	0x01B0,
	/* B */	0xFFFF,	0x028A,	0x028B,	0x01B4,	0xFFFF,	0x01B6,	0xFFFF,	0x0292,
			0x01B9,	0xFFFF,	0xFFFF,	0xFFFF,	0x01BD,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0x01C6,	0x01C6,	0xFFFF,	0x01C9,
			0x01C9,	0xFFFF,	0x01CC,	0x01CC,	0xFFFF,	0x01CE,	0xFFFF,	0x01D0,
	/* D */	0xFFFF,	0x01D2,	0xFFFF,	0x01D4,	0xFFFF,	0x01D6,	0xFFFF,	0x01D8,
			0xFFFF,	0x01DA,	0xFFFF,	0x01DC,	0xFFFF,	0xFFFF,	0x01DF,	0xFFFF,
	/* E */	0x01E1,	0xFFFF,	0x01E3,	0xFFFF,	0x01E5,	0xFFFF,	0x01E7,	0xFFFF,
			0x01E9,	0xFFFF,	0x01EB,	0xFFFF,	0x01ED,	0xFFFF,	0x01EF,	0xFFFF,
	/* F */	0xFFFF,	0x01F3,	0x01F3,	0xFFFF,	0x01F5,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0x01FB,	0xFFFF,	0x01FD,	0xFFFF,	0x01FF,	0xFFFF,

	// Table 3 (for high byte 0x02)
	
	/* 0 */	0x0201,	0xFFFF,	0x0203,	0xFFFF,	0x0205,	0xFFFF,	0x0207,	0xFFFF,
			0x0209,	0xFFFF,	0x020B,	0xFFFF,	0x020D,	0xFFFF,	0x020F,	0xFFFF,
	/* 1 */	0x0211,	0xFFFF,	0x0213,	0xFFFF,	0x0215,	0xFFFF,	0x0217,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 4 (for high byte 0x03)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0x03CA,	0xFFFF,
			0x03AD,	0x03AE,	0x03AF,	0xFFFF,	0x03CC,	0xFFFF,	0x03CD,	0x03CE,
	/* 9 */	0xFFFF,	0x03B1,	0x03B2,	0x03B3,	0x03B4,	0x03B5,	0x03B6,	0x03B7,
			0x03B8,	0x03B9,	0x03BA,	0x03BB,	0x03BC,	0x03BD,	0x03BE,	0x03BF,
	/* A */	0x03C0,	0x03C1,	0xFFFF,	0x03C3,	0x03C4,	0x03C5,	0x03C6,	0x03C7,
			0x03C8,	0x03C9,	0x03CA,	0x03CB,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0x03E3,	0xFFFF,	0x03E5,	0xFFFF,	0x03E7,	0xFFFF,
			0x03E9,	0xFFFF,	0x03EB,	0xFFFF,	0x03ED,	0xFFFF,	0x03EF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 5 (for high byte 0x04)
	
	/* 0 */	0xFFFF,	0x0451,	0x0452,	0x0453,	0x0454,	0x0455,	0x0456,	0x0457,
			0x0458,	0x0459,	0x045A,	0x045B,	0x045C,	0xFFFF,	0x045E,	0x045F,
	/* 1 */	0x0430,	0x0431,	0x0432,	0x0433,	0x0434,	0x0435,	0x0436,	0x0437,
			0x0438,	0x0439,	0x043A,	0x043B,	0x043C,	0x043D,	0x043E,	0x043F,
	/* 2 */	0x0440,	0x0441,	0x0442,	0x0443,	0x0444,	0x0445,	0x0446,	0x0447,
			0x0448,	0x0449,	0x044A,	0x044B,	0x044C,	0x044D,	0x044E,	0x044F,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0x0461,	0xFFFF,	0x0463,	0xFFFF,	0x0465,	0xFFFF,	0x0467,	0xFFFF,
			0x0469,	0xFFFF,	0x046B,	0xFFFF,	0x046D,	0xFFFF,	0x046F,	0xFFFF,
	/* 7 */	0x0471,	0xFFFF,	0x0473,	0xFFFF,	0x0475,	0xFFFF,	0x0477,	0xFFFF,
			0x0479,	0xFFFF,	0x047B,	0xFFFF,	0x047D,	0xFFFF,	0x047F,	0xFFFF,
	/* 8 */	0x0481,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0x0491,	0xFFFF,	0x0493,	0xFFFF,	0x0495,	0xFFFF,	0x0497,	0xFFFF,
			0x0499,	0xFFFF,	0x049B,	0xFFFF,	0x049D,	0xFFFF,	0x049F,	0xFFFF,
	/* A */	0x04A1,	0xFFFF,	0x04A3,	0xFFFF,	0x04A5,	0xFFFF,	0x04A7,	0xFFFF,
			0x04A9,	0xFFFF,	0x04AB,	0xFFFF,	0x04AD,	0xFFFF,	0x04AF,	0xFFFF,
	/* B */	0x04B1,	0xFFFF,	0x04B3,	0xFFFF,	0x04B5,	0xFFFF,	0x04B7,	0xFFFF,
			0x04B9,	0xFFFF,	0x04BB,	0xFFFF,	0x04BD,	0xFFFF,	0x04BF,	0xFFFF,
	/* C */	0xFFFF,	0x04C2,	0xFFFF,	0x04C4,	0xFFFF,	0xFFFF,	0xFFFF,	0x04C8,
			0xFFFF,	0xFFFF,	0xFFFF,	0x04CC,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0x04D1,	0xFFFF,	0x04D3,	0xFFFF,	0x04D5,	0xFFFF,	0x04D7,	0xFFFF,
			0x04D9,	0xFFFF,	0x04DB,	0xFFFF,	0x04DD,	0xFFFF,	0x04DF,	0xFFFF,
	/* E */	0x04E1,	0xFFFF,	0x04E3,	0xFFFF,	0x04E5,	0xFFFF,	0x04E7,	0xFFFF,
			0x04E9,	0xFFFF,	0x04EB,	0xFFFF,	0xFFFF,	0xFFFF,	0x04EF,	0xFFFF,
	/* F */	0x04F1,	0xFFFF,	0x04F3,	0xFFFF,	0x04F5,	0xFFFF,	0xFFFF,	0xFFFF,
			0x01F9,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 6 (for high byte 0xX5)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0x0561,	0x0562,	0x0563,	0x0564,	0x0565,	0x0566,	0x0567,
			0x0568,	0x0569,	0x056A,	0x056B,	0x056C,	0x056D,	0x056E,	0x056F,
	/* 4 */	0x0570,	0x0571,	0x0572,	0x0573,	0x0574,	0x0575,	0x0576,	0x0577,
			0x0578,	0x0579,	0x057A,	0x057B,	0x057C,	0x057D,	0x057E,	0x057F,
	/* 5 */	0x0580,	0x0581,	0x0582,	0x0583,	0x0584,	0x0585,	0x0586,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 7 (for high byte 0x10)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0x01D0,	0x01D1,	0x01D2,	0x01D3,	0x01D4,	0x01D5,	0x01D6,	0x01D7,
			0x01D8,	0x01D9,	0x01DA,	0x01DB,	0x01DC,	0x01DD,	0x01DE,	0x01DF,
	/* B */	0x01E0,	0x01E1,	0x01E2,	0x01E3,	0x01E4,	0x01E5,	0x01E6,	0x01E7,
			0x01E8,	0x01E9,	0x01EA,	0x01EB,	0x01EC,	0x01ED,	0x01EE,	0x01EF,
	/* C */	0x01F0,	0x01F1,	0x01F2,	0x01F3,	0x01F4,	0x01F5,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 8 (for high byte 0x1E)
	
	/* 0 */	0x1E01,	0xFFFF,	0x1E03,	0xFFFF,	0x1E05,	0xFFFF,	0x1E07,	0xFFFF,
			0x1E09,	0xFFFF,	0x1E0B,	0xFFFF,	0x1E0D,	0xFFFF,	0x1E0F,	0xFFFF,
	/* 1 */	0x1E11,	0xFFFF,	0x1E13,	0xFFFF,	0x1E15,	0xFFFF,	0x1E17,	0xFFFF,
			0x1E19,	0xFFFF,	0x1E1B,	0xFFFF,	0x1E1D,	0xFFFF,	0x1E1F,	0xFFFF,
	/* 2 */	0x1E21,	0xFFFF,	0x1E23,	0xFFFF,	0x1E25,	0xFFFF,	0x1E27,	0xFFFF,
			0x1E29,	0xFFFF,	0x1E2B,	0xFFFF,	0x1E2D,	0xFFFF,	0x1E2F,	0xFFFF,
	/* 3 */	0x1E31,	0xFFFF,	0x1E33,	0xFFFF,	0x1E35,	0xFFFF,	0x1E37,	0xFFFF,
			0x1E39,	0xFFFF,	0x1E3B,	0xFFFF,	0x1E3D,	0xFFFF,	0x1E3F,	0xFFFF,
	/* 4 */	0x1E41,	0xFFFF,	0x1E43,	0xFFFF,	0x1E45,	0xFFFF,	0x1E47,	0xFFFF,
			0x1E49,	0xFFFF,	0x1E4B,	0xFFFF,	0x1E4D,	0xFFFF,	0x1E4F,	0xFFFF,
	/* 5 */	0x1E51,	0xFFFF,	0x1E53,	0xFFFF,	0x1E55,	0xFFFF,	0x1E57,	0xFFFF,
			0x1E59,	0xFFFF,	0x1E5B,	0xFFFF,	0x1E5D,	0xFFFF,	0x1E5F,	0xFFFF,
	/* 6 */	0x1E61,	0xFFFF,	0x1E63,	0xFFFF,	0x1E65,	0xFFFF,	0x1E67,	0xFFFF,
			0x1E69,	0xFFFF,	0x1E6B,	0xFFFF,	0x1E6D,	0xFFFF,	0x1E6F,	0xFFFF,
	/* 7 */	0x1E71,	0xFFFF,	0x1E73,	0xFFFF,	0x1E75,	0xFFFF,	0x1E77,	0xFFFF,
			0x1E79,	0xFFFF,	0x1E7B,	0xFFFF,	0x1E7D,	0xFFFF,	0x1E7F,	0xFFFF,
	/* 8 */	0x1E81,	0xFFFF,	0x1E83,	0xFFFF,	0x1E85,	0xFFFF,	0x1E87,	0xFFFF,
			0x1E89,	0xFFFF,	0x1E8B,	0xFFFF,	0x1E8D,	0xFFFF,	0x1E8F,	0xFFFF,
	/* 9 */	0x1E91,	0xFFFF,	0x1E93,	0xFFFF,	0x1E95,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0x1EA1,	0xFFFF,	0x1EA3,	0xFFFF,	0x1EA5,	0xFFFF,	0x1EA7,	0xFFFF,
			0x1EA9,	0xFFFF,	0x1EAB,	0xFFFF,	0x1EAD,	0xFFFF,	0x1EAF,	0xFFFF,
	/* B */	0x1EB1,	0xFFFF,	0x1EB3,	0xFFFF,	0x1EB5,	0xFFFF,	0x1EB7,	0xFFFF,
			0x1EB9,	0xFFFF,	0x1EBB,	0xFFFF,	0x1EBD,	0xFFFF,	0x1EBF,	0xFFFF,
	/* C */	0x1EC1,	0xFFFF,	0x1EC3,	0xFFFF,	0x1EC5,	0xFFFF,	0x1EC7,	0xFFFF,
			0x1EC9,	0xFFFF,	0x1ECB,	0xFFFF,	0x1ECD,	0xFFFF,	0x1ECF,	0xFFFF,
	/* D */	0x1ED1,	0xFFFF,	0x1ED3,	0xFFFF,	0x1ED5,	0xFFFF,	0x1ED7,	0xFFFF,
			0x1ED9,	0xFFFF,	0x1EDB,	0xFFFF,	0x1EDD,	0xFFFF,	0x1EDF,	0xFFFF,
	/* E */	0x1EE1,	0xFFFF,	0x1EE3,	0xFFFF,	0x1EE5,	0xFFFF,	0x1EE7,	0xFFFF,
			0x1EE9,	0xFFFF,	0x1EEB,	0xFFFF,	0x1EED,	0xFFFF,	0x1EEF,	0xFFFF,
	/* F */	0x1EF1,	0xFFFF,	0x1EF3,	0xFFFF,	0x1EF5,	0xFFFF,	0x1EF7,	0xFFFF,
			0x1EF9,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 9 (for high byte 0x1F)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F00,	0x1F01,	0x1F02,	0x1F03,	0x1F04,	0x1F05,	0x1F06,	0x1F07,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F10,	0x1F11,	0x1F12,	0x1F13,	0x1F14,	0x1F15,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F20,	0x1F21,	0x1F22,	0x1F23,	0x1F24,	0x1F25,	0x1F26,	0x1F27,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F30,	0x1F31,	0x1F32,	0x1F33,	0x1F34,	0x1F35,	0x1F36,	0x1F37,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F40,	0x1F41,	0x1F42,	0x1F43,	0x1F44,	0x1F45,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F50,	0x1F51,	0x1F52,	0x1F53,	0x1F54,	0x1F55,	0x1F56,	0x1F57,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F60,	0x1F61,	0x1F62,	0x1F63,	0x1F64,	0x1F65,	0x1F66,	0x1F67,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F80,	0x1F81,	0x1F82,	0x1F83,	0x1F84,	0x1F85,	0x1F86,	0x1F87,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F90,	0x1F91,	0x1F92,	0x1F93,	0x1F94,	0x1F95,	0x1F96,	0x1F97,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1FA0,	0x1FA1,	0x1FA2,	0x1FA3,	0x1FA4,	0x1FA5,	0x1FA6,	0x1FA7,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1FB0,	0x1FB1,	0x1F70,	0x1F71,	0x1FB3,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F72,	0x1F73,	0x1F74,	0x1F75,	0x1FC3,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1FD0,	0x1FD1,	0x1F76,	0x1F77,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1FE0,	0x1FE1,	0x1F7A,	0x1F7B,	0x1FE5,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0x1F78,	0x1F79,	0x1F7C,	0x1F7D,	0x1FF3,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 10 (for high byte 0x21)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0x2170,	0x2171,	0x2172,	0x2173,	0x2174,	0x2175,	0x2176,	0x2177,
			0x2178,	0x2179,	0x217A,	0x217B,	0x217C,	0x217D,	0x217E,	0x217F,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 11 (for high byte 0x24)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 3 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0x24D0,	0x24D1,
			0x24D2,	0x24D3,	0x24D4,	0x24D5,	0x24D6,	0x24D7,	0x24D8,	0x24D9,
	/* C */	0x24DA,	0x24DB,	0x24DC,	0x24DD,	0x24DE,	0x24DF,	0x24E0,	0x24E1,
			0x24E2,	0x24E3,	0x24E4,	0x24E5,	0x24E6,	0x24E7,	0x24E8,	0x24E9,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	
	// Table 12 (for high byte 0xFF)
	
	/* 0 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 1 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 2 */	0xFFFF,	0xFF41,	0xFF42,	0xFF43,	0xFF44,	0xFF45,	0xFF46,	0xFF47,
			0xFF48,	0xFF49,	0xFF4A,	0xFF4B,	0xFF4C,	0xFF4D,	0xFF4E,	0xFF4F,
	/* 3 */	0xFF50,	0xFF51,	0xFF52,	0xFF53,	0xFF54,	0xFF55,	0xFF56,	0xFF57,
			0xFF58,	0xFF59,	0xFF5A,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 4 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 5 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 6 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 7 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 8 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* 9 */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* A */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* B */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* C */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* D */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* E */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
	/* F */	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,
			0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF,	0xFFFF
};
	
UniChar UnicodeLower( UniChar c )
{
	UInt8			cUpper, cLower;
	cUpper = (c >> 8) & 0xFF;
	cLower = c & 0xFF;
	
	UInt16			group;
	group = LCTable[cUpper];
	if( group == 0xFFFF )
		return c;
	
	UInt16			index;
	UniChar			lower;
	index = group << 8;
	lower = LCTable[index + cLower];
	if( lower == 0xFFFF )
		return c;
	else
		return lower;
}

void UnicodeToASCII( UniStr unicode, UInt32 length, ASCII8Str ascii )
{
	UInt32			i;
	for( i = 0; i < length; i++ )
		ascii[i] = unicode[i] & 0xFF;
}

void ASCIIToUnicode( ConstASCII8Str ascii, UInt32 length, UniStr unicode )
{
	UInt32			i;
	for( i = 0; i < length; i++ )
		unicode[i] = ascii[i];
}