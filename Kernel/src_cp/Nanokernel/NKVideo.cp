/*
	NKVideo.cp
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
#include "NKVideo.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKFonts.h"
#include "Memory Utils.h"
#include "Kernel Types.h"
#include "ADBMouse.h"
#include "Streams.h"

NKVideo		nkVideo;
static ASCII8	NKVideoNum2HexTab[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static Boolean	firstNKVideo = true;

static void NKVideoWrite(NKStream& s,ASCII8 c);
static void NKVideoMessage(NKStream& s,const StreamMessage& m);
static void NKWhiteOutScreen(void);
static void NKVideoNewLine(NKVideo& _video);

void NKInitVideo(NKVideo* _video,const Rect* _bounds,Boolean _scrolls,NKFont* _font)
{
	// Before making screen memory cache inhibited, we have to be sure that there are no screen cache lines in any cache.  MacOS marks screen memory as
	// cacheable and write-through.  This means that when you do a read from screen memory, it will stay in the cache for (presumably) faster access later.
	// When doing a write, it will go to the cache and also go directly to memory, since the write-through bit is set.
	//
	// The current desire to mark screen memory as cache inhibited is that I feel that there is likely more important stuff that should be residing in the cache,
	// and the video memory is not going to be read from very often at all.  Since we are marking this range as cache inhibited, we need to be sure to flush this
	// area of memory from the cache first.
	
	if(firstNKVideo)
	{
		// Map video memory via BAT3 - we haven't yet set up the paged memory system, so we use the BATs for now.
		machine.videoParams.logicalAddr = machine.videoParams.physicalAddr;	// Map it 1-1 for now.  [We actually could care less about the logical address as passed in from MacOS].
		NKBATMap(3,(void*)machine.videoParams.logicalAddr,(void*)machine.videoParams.physicalAddr,machine.videoParams.rowBytes*machine.videoParams.height,
			/*WIMG_CACHE_INHIBITED*//*WIMG_WRITE_THRU*/WIMG_CACHE_INHIBITED | WIMG_COHERENT | WIMG_GUARDED,PP_READ_WRITE);
		
		// Flush video memory from the cache
		UInt32 videoBufferLen = machine.videoParams.rowBytes*machine.videoParams.height;
		videoBufferLen += ((UInt32)machine.videoParams.logicalAddr & 0x00000FFF);
		void* videoBufferAddr = (void*)((UInt32)machine.videoParams.logicalAddr & 0xFFFFF000);
		NKFlushCaches(videoBufferAddr,videoBufferLen);
		
		// White out the entire screen, erasing anything that was left over on it from MacOS.
		NKWhiteOutScreen();
		
		firstNKVideo = false;
	}
	
	// Set up some info for this screen.
	_video->bounds = *_bounds;
	
	// Black out the nanokernel video console.  This is usually set up to be the left half of the screen.
	MouseShield	mouseShield;
	_video->bounds.x1++;
	_video->bounds.x2--;
	_video->bounds.y1++;
	_video->bounds.y2--;
	NKBlackOutRect(&_video->bounds);
	_video->bounds.x1++;
	_video->bounds.x2--;
	_video->bounds.y1++;
	_video->bounds.y2--;
	
	_video->cursX = _video->bounds.x1;
	_video->cursY = _video->bounds.y1;
	_video->scrolls = _scrolls;
	_video->font = _font;
	
	_video->foreColor = white8bit;
	_video->backColor = black8bit;
	
	_video->read = nil;
	_video->msg = NKVideoMessage;
	_video->write = NKVideoWrite;
}

void NKRemapVideo(void)
{
	// Remap video in IO memory using the page tables.  This is called after the page tables have been set up, but before the BATs have been disabled.
	machine.videoParams.logicalAddr =
		(Int8*)NKIOMap((void*)machine.videoParams.physicalAddr,machine.videoParams.rowBytes*machine.videoParams.height,
			/*WIMG_WRITE_THRU*/WIMG_CACHE_INHIBITED | WIMG_COHERENT | WIMG_GUARDED,PP_READ_WRITE);
}

void NKVideoWrite(NKStream& s,ASCII8 c)
{
	NKVideo&	_video = static_cast<NKVideo&>(s);
	if(c == '\n' || c == '\r')
		NKVideoNewLine(_video);
	else if(c == '\t')
	{
		_video << " ";
		while(_video.cursX % 4)
			_video << " ";
	}
	else
	{
		_video.font->drawChar(c,machine.videoParams.logicalAddr + _video.cursX + _video.cursY*machine.videoParams.rowBytes,machine.videoParams.rowBytes,_video.foreColor,_video.backColor,8);
		_video.cursX += _video.font->width();
		if(_video.cursX + _video.font->width() > _video.bounds.x2)
			NKVideoNewLine(_video);
	}
}

void NKVideoMessage(NKStream& s,const StreamMessage& message)
{
	NKVideo&	_video = static_cast<NKVideo&>(s);
	switch(message.type)
	{
		case colorMessage:
			_video.foreColor = message.data;
		break;
	}
}

static void NKWhiteOutScreen(void)
{
	UInt32*	zapper;
	UInt32	rowLenWords = machine.videoParams.width*machine.videoParams.pixSize/(4*8);
	for(UInt32 y=0;y<machine.videoParams.height;y++)
	{
		zapper = (UInt32*)(machine.videoParams.logicalAddr + machine.videoParams.rowBytes*y);
		for(UInt32 x=0;x<rowLenWords;x++)
			*zapper++ = 0;
	}
}

void NKBlackOutRect(Rect* _rect, UInt32 color)
{
	UInt32 leftByte = _rect->x1*machine.videoParams.pixSize/8;
	UInt32 rightByte = _rect->x2*machine.videoParams.pixSize/8;
	
	UInt32 leftPixels = (4 - (leftByte % 4)) % 4;
	UInt32 wordPixels = (rightByte - leftByte - leftPixels) / 4;
	UInt32 rightPixels = (rightByte - leftByte - leftPixels - 4*wordPixels);
	
	MouseShield	mouseShield;
	for(Int32 y = _rect->y1;y<_rect->y2;y++)
	{
		UInt8* x = (UInt8*)(machine.videoParams.logicalAddr + leftByte + y*machine.videoParams.rowBytes);
		
		for(Int32 i = 0;i < leftPixels;i++)
			*x++ = color;
		
		for(Int32 i = 0;i < wordPixels;i++)
			*((UInt32*)x)++ = color;
		
		for(Int32 i = 0;i < rightPixels;i++)
			*x++ = color;
	}
}

void NKInvertRect(Rect* _rect)
{
	UInt32 leftByte = _rect->x1*machine.videoParams.pixSize/8;
	UInt32 rightByte = _rect->x2*machine.videoParams.pixSize/8;
	
	UInt32 leftPixels = (4 - (leftByte % 4)) % 4;
	UInt32 wordPixels = (rightByte - leftByte - leftPixels) / 4;
	UInt32 rightPixels = (rightByte - leftByte - leftPixels - 4*wordPixels);
	
	MouseShield	mouseShield;
	for(Int32 y = _rect->y1;y<_rect->y2;y++)
	{
		UInt8* x = (UInt8*)(machine.videoParams.logicalAddr + leftByte + y*machine.videoParams.rowBytes);
		
		for(Int32 i = 0;i < leftPixels;i++)
		{
			*x = ~*x;
			x++;
		}
		
		for(Int32 i = 0;i < wordPixels;i++)
		{
			*(UInt32*)x = ~*(UInt32*)x;
			x += 4;
		}
		
		for(Int32 i = 0;i < rightPixels;i++)
		{
			*x = ~*x;
			x++;
		}
	}
}

static void NKVideoNewLine(NKVideo& _video)
{
	_video.cursX = _video.bounds.x1;
	_video.cursY += _video.font->height();
	if(_video.cursY + _video.font->height() > _video.bounds.y2)
	{
		if(_video.scrolls)
		{
			NKScrollVideoVertical(&_video.bounds,-_video.font->height());
			Rect	blackRect = _video.bounds;
			blackRect.y1 = _video.bounds.y1 + ((_video.bounds.y2 - _video.bounds.y1)/_video.font->height() - 1) * _video.font->height();
			NKBlackOutRect(&blackRect);
			_video.cursY -= _video.font->height();
		}
		else
		{
			_video.cursY = _video.bounds.y1;
			NKBlackOutRect(&_video.bounds);
		}
	}
}

// This has been assembly optimized below
#define NK_SCROLL_ASM			1
#if !NK_SCROLL_ASM
void NKScrollVideoVertical(Rect* _rect,Int32 dy)
{
	// (Pat:) This actually doesn't work if you give it a positive dy, since it always copies from top to bottom.  Thus, the first line scrolled
	// overwrites the line dy pixels below, which should have been copied before but wasn't.  In essence, right now a positive dy yields
	// a strange duplicating effect, where the first dy lines of the rectangle are replicated throughout the rectangle, quite an opposite
	// effect from what is wanted.  You really need two rather different copy loops to scroll up and down, since to scroll down, you have
	// to copy lines from the bottom up.
	
	// A negative dy means scroll the stuff in the rect UP, a positive dy means scroll the stuff down
	UInt32 leftByte = _rect->x1*machine.videoParams.pixSize/8;
	UInt32 rightByte = _rect->x2*machine.videoParams.pixSize/8;
	UInt32 topRow = (dy < 0) ? _rect->y1 : _rect->y1 - dy;
	UInt32 bottomRow = (dy < 0) ? _rect->y2 + dy : _rect->y2;
	
	UInt32 leftCopy = (4 - (leftByte % 4)) % 4;						// Number of bytes to copy before we align to a word boundary
	UInt32 wordCopy = (rightByte - leftByte - leftCopy) / 4;			// Number of words we can copy
	wordCopy -= wordCopy % 4;										// Round down to a multiple of 4 words (16 bytes)
	UInt32 rightCopy = (rightByte - leftByte - leftCopy - 4*wordCopy);	// Number of bytes to copy at the end of the row
	
	MouseShield	shield;
	for(Int32 y = topRow;y <= bottomRow;y++)
	{
		UInt8* dst = (UInt8*)(machine.videoParams.logicalAddr + leftByte + y*machine.videoParams.rowBytes);
		UInt8* src = (UInt8*)(machine.videoParams.logicalAddr + leftByte + (y - dy)*machine.videoParams.rowBytes);
		
		// Copy bytes until we are word aligned
		for(Int32 i=0;i<leftCopy;i++)
			*dst++ = *src++;
		
		MoveMem_x16(src,dst,wordCopy*4);
		dst += wordCopy*4;
		src += wordCopy*4;
		
		// Copy the remaining bytes at the end of the line
		for(Int32 i=0;i<rightCopy;i++)
			*dst++ = *src++;
	}
}
#else
static void _NKScrollVideoVertical(Rect* _rect, Int32 dy, UInt32 pixSize, UInt32 rowBytes, Int8* addr);

void NKScrollVideoVertical(Rect* _rect,Int32 dy)
{
//	if( dy == 0 )
//		return;
//	else if( dy > 0 )
//		Panic( "Assembly NKScrollVideoVertical doesn't handle positive dy!!!" );
	MouseShield	shield;
	_NKScrollVideoVertical(_rect,dy,machine.videoParams.pixSize,machine.videoParams.rowBytes,machine.videoParams.logicalAddr);
}

#define USE_CTR			1		// 1= use mtctr...bdnz for loops, otherwise uses sub. ... bne (latter may actually be faster in some CPUs)
#if USE_CTR
__asm__ void _NKScrollVideoVertical(Rect* _rect, Int32 dy, UInt32 pixSize, UInt32 rowBytes, Int8* addr)
{
	// r3 = *rectangle
	// r4 = signed displacement
	// r5 = size in bits of each pixel
	// r6 = rowBytes
	// r7 = base address of screen
	
	// Save non-volatile regs
	stmw			r27,-20(sp);
	
	// Convert pixSize to bytes (divide by 8 = shift 3 bits to the right)
	srwi				(r5,r5,3);
	
	// Put start src (addr of pixel (_rect->x1,_rect->y1-dy)) into r30
	// Changes:	r8 = _rect->x1*pixSize
	// 			r9 = baseAddr + _rect->x1*pixSize
	//			r10 = _rect->y1
	//			r11 = (_rect->y1-dy)*rowBytes
	lwz				r8,Rect.x1(r3);
	mullw			r8,r8,r5;
	add				r9,r8,r7;
	lwz				r10,Rect.y1(r3);
	sub				r11,r10,r4;
	mullw			r11,r11,r6;
	add				r30,r9,r11;
	
	// Free volatiles: r0,r7,r11,r12
	
	// Put start dst (addr of pixel (_rect->x1,_rect->y1)) into r31
	mullw			r12,r10,r6;
	add				r31,r9,r12;
	
	// Free volatiles: r0,r7,r11,r12
	
	// Put offset necessary from one row to the next into r9
	lwz				r9,Rect.x2(r3);
	mullw			r9,r9,r5;		// r9 = _rect->x2*pixSize
	sub				r8,r9,r8;		// r8 now contains the number of bytes in one row of the rectangle
	sub				r9,r6,r8;		// r9 = rowBytes - bytes in one rectangle row, i.e. displacement from the end of one row to the start
								// of the next
	
	// Free volatiles: r0,r5,r6,r7,r10,r11,r12
	
	// Put number of rows to copy into r10
	lwz				r0,Rect.y2(r3);
	sub				r10,r0,r10;
	addi				r10,r10,1;
	add.				r10,r10,r4;	// Remember, dy is negative
	ble				@exit;		// Return if we have nothing to do
	
	// Free volatiles: r0,r3,r4,r5,r6,r7,r11,r12
	
	// Calculate number of bytes to word alignment of addresses (both addresses should be aligned to the same boundary) into r27
	andi.				r27,r30,0x0007;
	beq				@noLeft;			// 0 if word-aligned already
	li				r0,8;
	sub				r27,r0,r27;
@noLeft:
	sub				r29,r8,r27;		// Adjust total bytes remaining to copy
	
	// We copy as many sixteen-byte runs as possible.  But, should there be any trailing bytes, those must be copied to.  Sooo.... r28 = trailing right bytes
	andi.				r28,r29,0x000F;
	
	// Calculate the number of 16-byte runs we can copy, into r29
	srwi				(r29,r29,4);
	
	// Check on important registers up until now:
	//	r8 = number of bytes in one row
	//	r9 = offset in bytes from the end of one row to the start of the next
	//	r10 = number of rows to copy
	//	r27 = number of bytes to copy at start until addresses are word-aligned
	//	r28 = number of bytes to copy after as many 16-byte runs have been copied as possible
	//	r29 = number of 16-byte runs to copy
	//	r30 = address to start copying from
	//	r31 = address to start writing to
	// Free volatiles: r0,r3,r4,r5,r6,r7,r11,r12
	
	// Pre-decrement
	subi				r30,r30,1;
	subi				r31,r31,1;
	
@mainLoop:
		// Copy left bytes
		mtctr		r27;
		beq			@startRuns;	// Got this from MoveMem_x16, is it that mtctr updates the condition register?
	@leftLoop:
		lbzu			r0,1(r30);
		stbu			r0,1(r31);
		bdnz+		@leftLoop;
		
	@startRuns:
		// Main copy loop
		mtctr		r29;
		beq-			@startRight;
		
		//Adjust pre-decrement to be -8
		subi			r30,r30,7;
		subi			r31,r31,7;
	@copyLoop:
		/*lwzu			r0,4(r30);
		lwzu			r3,4(r30);
		lwzu			r4,4(r30);
		lwzu			r5,4(r30);
		stwu			r0,4(r31);
		stwu			r3,4(r31);
		stwu			r4,4(r31);
		stwu			r5,4(r31);*/
		lfd			fp0,8(r30);
		lfdu			fp1,16(r30);
		stfd			fp0,8(r31);
		stfdu			fp1,16(r31);
		bdnz+		@copyLoop;
		// Adjust pre-decrement to be -1
		addi			r30,r30,7;
		addi			r31,r31,7;
		
	@startRight:
		// Copy right bytes?
		mtctr		r28;
		beq-			@endRow;
	@rightLoop:
		lbzu			r0,1(r30);
		stbu			r0,1(r31);
		bdnz+		@rightLoop;
		
	@endRow:
		// Adjust for next row
		add			r30,r30,r9;
		add			r31,r31,r9;
		
		// Another row?
		subi			r10,r10,1;
		cmpwi		r10,0;
		bne+			@mainLoop;
	
	// Restore non-volatile regs and leave
@exit:
	lmw				r27,-20(sp);
	blr;
}
#else
__asm__ void _NKScrollVideoVertical(Rect* _rect, Int32 dy, UInt32 pixSize, UInt32 rowBytes, Int8* addr)
{
	// r3 = *rectangle
	// r4 = signed displacement
	// r5 = size in bits of each pixel
	// r6 = rowBytes
	// r7 = base address of screen
	
	// Save non-volatile regs
	stmw			r27,-20(sp);
	
	// Convert pixSize to bytes (divide by 8 = shift 3 bits to the right)
	srwi				(r5,r5,3);
	
	// Put start src (addr of pixel (_rect->x1,_rect->y1-dy)) into r30
	// Changes:	r8 = _rect->x1*pixSize
	// 			r9 = baseAddr + _rect->x1*pixSize
	//			r10 = _rect->y1
	//			r11 = (_rect->y1-dy)*rowBytes
	lwz				r8,Rect.x1(r3);
	mullw			r8,r8,r5;
	add				r9,r8,r7;
	lwz				r10,Rect.y1(r3);
	sub				r11,r10,r4;
	mullw			r11,r11,r6;
	add				r30,r9,r11;
	
	// Free volatiles: r0,r7,r11,r12
	
	// Put start dst (addr of pixel (_rect->x1,_rect->y1)) into r31
	mullw			r12,r10,r6;
	add				r31,r9,r12;
	
	// Free volatiles: r0,r7,r11,r12
	
	// Put offset necessary from one row to the next into r9
	lwz				r9,Rect.x2(r3);
	mullw			r9,r9,r5;		// r9 = _rect->x2*pixSize
	sub				r8,r9,r8;		// r8 now contains the number of bytes in one row of the rectangle
	sub				r9,r6,r8;		// r9 = rowBytes - bytes in one rectangle row, i.e. displacement from the end of one row to the start
								// of the next
	
	// Free volatiles: r0,r5,r6,r7,r10,r11,r12
	
	// Put number of rows to copy into r10
	lwz				r0,Rect.y2(r3);
	sub				r10,r0,r10;
	addi				r10,r10,1;
	add.				r10,r10,r4;	// Remember, dy is negative
	ble				@exit;		// Return if we have nothing to do
	
	// Free volatiles: r0,r3,r4,r5,r6,r7,r11,r12
	
	// Calculate number of bytes to word alignment of addresses (both addresses should be aligned to the same boundary) into r27
	andi.				r27,r30,0x0007;
	beq				@noLeft;			// 0 if word-aligned already
	li				r0,8;
	sub				r27,r0,r27;
@noLeft:
	sub				r29,r8,r27;		// Adjust total bytes remaining to copy
	
	// We copy as many sixteen-byte runs as possible.  But, should there be any trailing bytes, those must be copied to.  Sooo.... r28 = trailing right bytes
	andi.				r28,r29,0x000F;
	
	// Calculate the number of 16-byte runs we can copy, into r29
	srwi				(r29,r29,4);
	
	// Check on important registers up until now:
	//	r8 = number of bytes in one row
	//	r9 = offset in bytes from the end of one row to the start of the next
	//	r10 = number of rows to copy
	//	r27 = number of bytes to copy at start until addresses are word-aligned
	//	r28 = number of bytes to copy after as many 16-byte runs have been copied as possible
	//	r29 = number of 16-byte runs to copy
	//	r30 = address to start copying from
	//	r31 = address to start writing to
	// Free volatiles: r0,r3,r4,r5,r6,r7,r11,r12
	
	// Pre-decrement
	subi				r30,r30,1;
	subi				r31,r31,1;
	li				r11,1;
	
	// r12 replaces the ctr
	// r11 is fixed to one so that I can do sub.  instead of subi/cmpwi 0
	
@mainLoop:
		// Copy left bytes
		mr.			r12,r27;
		beq-			@startRuns;
	@leftLoop:
		sub.			r12,r12,r11;
		lbzu			r0,1(r30);
		stbu			r0,1(r31);
		bne+			@leftLoop;
		
	@startRuns:
		// Main copy loop
		mr.			r12,r29;
		beq-			@startRight;
		
		//Adjust pre-decrement to be -8
		subi			r30,r30,7;
		subi			r31,r31,7;
	@copyLoop:
		sub.			r12,r12,r11;
		lfd			fp0,8(r30);
		lfdu			fp1,16(r30);
		stfd			fp0,8(r31);
		stfdu			fp1,16(r31);
		bne+			@copyLoop;
		
		// Adjust pre-decrement to be -1
		addi			r30,r30,7;
		addi			r31,r31,7;
		
	@startRight:
		// Copy right bytes?
		mr.			r12,r28;
		beq-			@endRow;
	@rightLoop:
		sub.			r12,r12,r11;
		lbzu			r0,1(r30);
		stbu			r0,1(r31);
		bne+			@rightLoop;
		
	@endRow:
		// Adjust for next row
		add			r30,r30,r9;
		add			r31,r31,r9;
		
		// Another row?
		sub.			r10,r10,r11;
		bne+			@mainLoop;
	
	// Restore non-volatile regs and leave
@exit:
	lmw				r27,-20(sp);
	blr;
}
#endif /* USE_CTR */
#endif /* NK_SCROLL_ASM */