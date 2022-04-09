/*
	ADBKeyboardMaps.h
	Copyright © 1998 by Patrick Varilly

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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	???
	
	Version History
	============
	Patrick Varilly		-	Tuesday, 27 Jan 98	-	Original creation of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
/*
	ADBKeyboardMaps.h
	
	Default keyboard maps for ADB keyboards
	
	Copyright © 1998 by The Pandora Team. All rights reserved worldwide.
	Permission to use and modify this file is given solely to the Pandora Team until further notice
*/
#ifndef __ADB__KEYBOARD__MAPS__
#define __ADB__KEYBOARD__MAPS__

/* These are just *defaults*, and are hard-coded into the kernel. Although they shouldn't be removed (not even in
	shipping Pandora), they should be replaced (via software) as the OS (not the kernel) boots, and reads the
	user's keyboard drivers from the hard disk or CD */

typedef UInt8 ADBKeyboardMap[128][2];

enum {
	kKeyboardMapUndefined = 0x00 };

#define	KEY_F1	kKeyboardMapUndefined
#define	KEY_F2	kKeyboardMapUndefined
#define	KEY_F3	kKeyboardMapUndefined
#define	KEY_F4	kKeyboardMapUndefined
#define	KEY_F5	kKeyboardMapUndefined
#define	KEY_F6	kKeyboardMapUndefined
#define	KEY_F7	kKeyboardMapUndefined
#define	KEY_F8	kKeyboardMapUndefined
#define	KEY_F9	kKeyboardMapUndefined
#define	KEY_F10	kKeyboardMapUndefined
#define	KEY_F11	kKeyboardMapUndefined
#define	KEY_F12	kKeyboardMapUndefined
#define	KEY_F1S	kKeyboardMapUndefined
#define	KEY_F2S	kKeyboardMapUndefined
#define	KEY_F3S	kKeyboardMapUndefined
#define	KEY_F4S	kKeyboardMapUndefined
#define	KEY_F5S	kKeyboardMapUndefined
#define	KEY_F6S	kKeyboardMapUndefined
#define	KEY_F7S	kKeyboardMapUndefined
#define	KEY_F8S	kKeyboardMapUndefined
#define	KEY_F9S	kKeyboardMapUndefined
#define	KEY_F10S	kKeyboardMapUndefined
#define	KEY_F11S	kKeyboardMapUndefined
#define	KEY_F12S	kKeyboardMapUndefined

#pragma mark ADBKeyboardMapUS
ADBKeyboardMap ADBKeyboardMapUS = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'a',       'A' },
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'z',       'Z' },
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'q',       'Q' },
	{	/*   0x0D, */       'w',       'W' },
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'y',       'Y' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '!' },
	{	/*   0x13, */       '2',       '@' },
	{	/*   0x14, */       '3',       '#' },
	{	/*   0x15, */       '4',       '$' },
	{	/*   0x16, */       '6',       '^' },
	{	/*   0x17, */       '5',       '%' },
	{	/*   0x18, */       '=',       '+' },
	{	/*   0x19, */       '9',       '(' },
	{	/*   0x1A, */       '7',       '&' },
	{	/*   0x1B, */       '-',       '_' },
	{	/*   0x1C, */       '8',       '*' },
	{	/*   0x1D, */       '0',       ')' },
	{	/*   0x1E, */       ']',       '}' },
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */       '[',       '{' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Returns
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */      '\'',       '"' },
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       ';',       ':' },
	{	/*   0x2A, */      '\\',       '|' },
	{	/*   0x2B, */       ',',       '<' },
	{	/*   0x2C, */       '/',       '?' },
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       'm',       'M' },
	{	/*   0x2F, */       '.',       '>' },
	{	/*   0x30, */      0x09,      0x09 },	// Tab
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '`',       '~' },
	{	/*   0x33, */      0x08,      0x08 }, // Delete
	{	/*   0x34, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Enter beside spacebar
	{	/*   0x35, */      0x1B,      0x1B },	// Escape
	{	/*   0x36, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x37, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x38, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x39, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3B, */       0x1C/*'h'*/,      kKeyboardMapUndefined },	// Left arrow
	{	/*   0x3C, */       0x1D/*'l'*/,      kKeyboardMapUndefined },	// Right arrow
	{	/*   0x3D, */       0x1F/*'j'*/,      kKeyboardMapUndefined },	// Down arrow
	{	/*   0x3E, */       0x1E/*'k'*/,      kKeyboardMapUndefined },	// Up arrow
	{	/*   0x3F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x40, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x47, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x48, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x49, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Enter on keypad
	{	/*   0x4D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x50, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x60, */      KEY_F5,	KEY_F5S },
	{	/*   0x61, */      KEY_F6,	KEY_F6S },
	{	/*   0x62, */      KEY_F7,	KEY_F7S },
	{	/*   0x63, */      KEY_F3,	KEY_F3S },
	{	/*   0x64, */      KEY_F8,	KEY_F8S },
	{	/*   0x65, */      KEY_F9,	KEY_F9S },
	{	/*   0x66, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x67, */      KEY_F11,	KEY_F11S },
	{	/*   0x68, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x69, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6D, */      KEY_F10,	KEY_F10S },
	{	/*   0x6E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6F, */      KEY_F12,	KEY_F12S },
	{	/*   0x70, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x71, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x72, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x73, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x74, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x75, */      0x7F,		0x7F },		// <-- Forward Delete
	{	/*   0x76, */      KEY_F4,	KEY_F4S },
	{	/*   0x77, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x78, */      KEY_F2,	KEY_F2S },
	{	/*   0x79, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7A, */      KEY_F1,	KEY_F1S },
	{	/*   0x7B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined }
};

#pragma mark ADBKeyboardMapFrench
ADBKeyboardMap ADBKeyboardMapFrench = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'q',       'Q' },//
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'w',       'W' },//
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'a',       'A' },//
	{	/*   0x0D, */       'z',       'Z' },//
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'y',       'Y' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '&' },//
	{	/*   0x13, */       '2',       '{' },//
	{	/*   0x14, */       '3',       '"' },//
	{	/*   0x15, */       '4',       '\'' },//
	{	/*   0x16, */       '6',       '6' },//
	{	/*   0x17, */       '5',       '(' },//
	{	/*   0x18, */       '-',       '_' },//
	{	/*   0x19, */       '9',       'c' },//
	{	/*   0x1A, */       '7',       '}' },//
	{	/*   0x1B, */       ')',       ')' },//
	{	/*   0x1C, */       '8',       '!' },//
	{	/*   0x1D, */       '0',       '@' },//
	{	/*   0x1E, */       '$',       '*' },//
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */       '^',       '^' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Return
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */	    'u',       '%' },//
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       'm',       'M' },//
	{	/*   0x2A, */      	'`',       '#' },//
	{	/*   0x2B, */       ';',       '.' },//
	{	/*   0x2C, */       '=',       '+' },//
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       ',',       '?' },//
	{	/*   0x2F, */       ':',       '/' },//
	{	/*   0x30, */      0x09,      0x09 },	// Tab
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '<',       '>' },
	{	/*   0x33, */      0x7F,      0x7F }, // Delete
	{	/*   0x34, */      KEYBOARD_RETURN,	KEYBOARD_RETURN },	// Enter beside spacebar
	{	/*   0x35, */      0x1B,      0x1B },	// Escape
	{	/*   0x36, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x37, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x38, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x39, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3B, */       0x1C,      kKeyboardMapUndefined },
	{	/*   0x3C, */       0x1D,      kKeyboardMapUndefined },
	{	/*   0x3D, */       0x1F,      kKeyboardMapUndefined },
	{	/*   0x3E, */       0x1E,      kKeyboardMapUndefined },
	{	/*   0x3F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x40, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x47, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x48, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x49, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Enter on keypad
	{	/*   0x4D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x50, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x60, */      KEY_F5,	KEY_F5S },
	{	/*   0x61, */      KEY_F6,	KEY_F6S },
	{	/*   0x62, */      KEY_F7,	KEY_F7S },
	{	/*   0x63, */      KEY_F3,	KEY_F3S },
	{	/*   0x64, */      KEY_F8,	KEY_F8S },
	{	/*   0x65, */      KEY_F9,	KEY_F9S },
	{	/*   0x66, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x67, */      KEY_F11,	KEY_F11S },
	{	/*   0x68, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x69, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6D, */      KEY_F10,	KEY_F10S },
	{	/*   0x6E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6F, */      KEY_F12,	KEY_F12S },
	{	/*   0x70, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x71, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x72, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x73, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x74, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x75, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x76, */      KEY_F4,	KEY_F4S },
	{	/*   0x77, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x78, */      KEY_F2,	KEY_F2S },
	{	/*   0x79, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7A, */      KEY_F1,	KEY_F1S },
	{	/*   0x7B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined }
};

#pragma mark ADBKeyboardMapGerman
ADBKeyboardMap ADBKeyboardMapGerman = {
		/* Scan code      Normal     Shifted */
	{	/*   0x00, */       'a',       'A' },
	{	/*   0x01, */       's',       'S' },
	{	/*   0x02, */       'd',       'D' },
	{	/*   0x03, */       'f',       'F' },
	{	/*   0x04, */       'h',       'H' },
	{	/*   0x05, */       'g',       'G' },
	{	/*   0x06, */       'y',       'Y' },
	{	/*   0x07, */       'x',       'X' },
	{	/*   0x08, */       'c',       'C' },
	{	/*   0x09, */       'v',       'V' },
	{	/*   0x0A, */       '^',       '~' },
	{	/*   0x0B, */       'b',       'B' },
	{	/*   0x0C, */       'q',       'Q' },
	{	/*   0x0D, */       'w',       'W' },
	{	/*   0x0E, */       'e',       'E' },
	{	/*   0x0F, */       'r',       'R' },
	{	/*   0x10, */       'z',       'Z' },
	{	/*   0x11, */       't',       'T' },
	{	/*   0x12, */       '1',       '!' },
	{	/*   0x13, */       '2',       '"' },
	{	/*   0x14, */       '3',       '@' },
	{	/*   0x15, */       '4',       '$' },
	{	/*   0x16, */       '6',       '&' },
	{	/*   0x17, */       '5',       '%' },
	{	/*   0x18, */       '~',       '`' },
	{	/*   0x19, */       '9',       ')' },
	{	/*   0x1A, */       '7',       '/' },
	{	/*   0x1B, */       '^',       '?' },
	{	/*   0x1C, */       '8',       '(' },
	{	/*   0x1D, */       '0',       '=' },
	{	/*   0x1E, */       '+',       '*' },
	{	/*   0x1F, */       'o',       'O' },
	{	/*   0x20, */       'u',       'U' },
	{	/*   0x21, */      '\\',       '|' },
	{	/*   0x22, */       'i',       'I' },
	{	/*   0x23, */       'p',       'P' },
	{	/*   0x24, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },
	{	/*   0x25, */       'l',       'L' },
	{	/*   0x26, */       'j',       'J' },
	{	/*   0x27, */       ']',       '}' },
	{	/*   0x28, */       'k',       'K' },
	{	/*   0x29, */       '[',       '{' },
	{	/*   0x2A, */       '#',      '\'' },
	{	/*   0x2B, */       ',',       ';' },
	{	/*   0x2C, */       '-',       '_' },
	{	/*   0x2D, */       'n',       'N' },
	{	/*   0x2E, */       'm',       'M' },
	{	/*   0x2F, */       '.',       ':' },
	{	/*   0x30, */      0x09,      0x09 },	// Tab
	{	/*   0x31, */       ' ',       ' ' },
	{	/*   0x32, */       '<',       '>' },
	{	/*   0x33, */      0x7F,      0x7F }, // Backspace
	{	/*   0x34, */      KEYBOARD_RETURN,	KEYBOARD_RETURN },	// Enter beside spacebar
	{	/*   0x35, */      0x1B,      0x1B },	// Escape
	{	/*   0x36, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x37, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x38, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x39, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x3B, */       0x1C,      kKeyboardMapUndefined },
	{	/*   0x3C, */       0x1D,      kKeyboardMapUndefined },
	{	/*   0x3D, */       0x1F,      kKeyboardMapUndefined },
	{	/*   0x3E, */       0x1E,      kKeyboardMapUndefined },
	{	/*   0x3F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x40, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x41, */       '.',       '.' },
	{	/*   0x42, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x43, */       '*',       '*' },
	{	/*   0x44, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x45, */       '+',       '+' },
	{	/*   0x46, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x47, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x48, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x49, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4B, */       '/',       '/' },
	{	/*   0x4C, */      KEYBOARD_RETURN,      KEYBOARD_RETURN },	// Enter key on keypad
	{	/*   0x4D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x4E, */       '-',       '-' },
	{	/*   0x4F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x50, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x51, */       '=',       '=' },
	{	/*   0x52, */       '0',       '0' },
	{	/*   0x53, */       '1',       '1' },
	{	/*   0x54, */       '2',       '2' },
	{	/*   0x55, */       '3',       '3' },
	{	/*   0x56, */       '4',       '4' },
	{	/*   0x57, */       '5',       '5' },
	{	/*   0x58, */       '6',       '6' },
	{	/*   0x59, */       '7',       '7' },
	{	/*   0x5A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5B, */       '8',       '8' },
	{	/*   0x5C, */       '9',       '9' },
	{	/*   0x5D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x5F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x60, */      KEY_F5,	KEY_F5S },
	{	/*   0x61, */      KEY_F6,	KEY_F6S },
	{	/*   0x62, */      KEY_F7,	KEY_F7S },
	{	/*   0x63, */      KEY_F3,	KEY_F3S },
	{	/*   0x64, */      KEY_F8,	KEY_F8S },
	{	/*   0x65, */      KEY_F9,	KEY_F9S },
	{	/*   0x66, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x67, */      KEY_F11,	KEY_F11S },
	{	/*   0x68, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x69, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6A, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6D, */      KEY_F10,	KEY_F10S },
	{	/*   0x6E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x6F, */      KEY_F12,	KEY_F12S },
	{	/*   0x70, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x71, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x72, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x73, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x74, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x75, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x76, */      KEY_F4,	KEY_F4S },
	{	/*   0x77, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x78, */      KEY_F2,	KEY_F2S },
	{	/*   0x79, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7A, */      KEY_F1,	KEY_F1S },
	{	/*   0x7B, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7C, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7D, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7E, */      kKeyboardMapUndefined,      kKeyboardMapUndefined },
	{	/*   0x7F, */      kKeyboardMapUndefined,      kKeyboardMapUndefined }
};

/* Add your keyboard maps here */
typedef struct
{
	ADBKeyboardMap		*keyMap;
	ASCII8Str				mapName;
} ADBKeyboardMapTableEntry;

ADBKeyboardMapTableEntry keyMapTable[] = {
	{ &ADBKeyboardMapUS, "US" },
	{ &ADBKeyboardMapFrench, "French" },
	{ &ADBKeyboardMapGerman, "German" },
	{ nil, nil }				// Add entries *before* this
};

#endif /* !__ADB__KEYBOARD__MAPS__ */