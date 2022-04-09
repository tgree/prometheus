/*
	debugger_parser.h
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
	Other sources			Project				Author			Notes
	===========			======				=====			====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#define	tC_IL	258
#define	tC_IP	259
#define	tC_ST	260
#define	tC_HELP	261
#define	tC_RS	262
#define	tC_SB	263
#define	tC_SH	264
#define	tC_SW	265
#define	tR_PC	266
#define	tR_LR	267
#define	tR_SP	268
#define	tR_RTOC	269
#define	tSHIFTLEFT	270
#define	tSHIFTRIGHT	271
#define	tREGISTER	272
#define	tCONSTANT	273
#define	tSEGREGISTER 274
#define	tC_TL		275
#define	tC_DM		276

#define ntSTATEMENT	0
#define ntREGISTER	1
#define ntEXPR			2
#define ntTERM			3
#define ntFACTOR		4
#define ntVALUE			5

extern Int32 yylval;
