/*
	Video Driver.cp
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	video_control.cp		Mach DR2.1 update 6		Michael Burg	Used part of control_init() routine in
														PCIVideo::PCIVideo().
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "Kernel Types.h"
#include "Assembly.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "Video Driver.h"

struct DummyVideo	:	public VideoDriver
{
	DummyVideo();
};

struct PCIVideo	:	public VideoDriver
{
	Reg8*	control_clut;
	Reg8*	control_cursor_data;
	Reg8*	control_multiport;
	Reg8*	control_clut_data;
	
	PCIVideo();
};

VideoDriver*			video;

void InitVideo(void)
{
	// Allocate a video driver for this machine
	switch(machine.machineClass)
	{
		case classPCI:
			video = new PCIVideo;
		break;
		default:
			video = new DummyVideo;
		break;
	}
}

DummyVideo::DummyVideo():
	VideoDriver("Dummy Video Driver")
{
	videoParams = machine.videoParams;
}

PCIVideo::PCIVideo():
	VideoDriver("PCI Control Video Driver")
{
	videoParams = machine.videoParams;
	
	// Map some video controllers
	control_clut = (Reg8*)NKIOMap((void*)0xF301B000,4096,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	control_cursor_data = control_clut + 0x00000010;
	control_multiport = control_clut + 0x00000020;
	control_clut_data = control_clut + 0x00000030;
	
	// Disable the hardware cursor
	_eieio();
	*control_clut = 0x20;
	_eieio();
	UInt8 val = *control_multiport;
	_eieio();
	*control_multiport = (val & 0xFD);
	_eieio();
}
