/*
	BootInfo.h
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
	Terry Greeniaus	-	Monday, 12 Oct 98	-	Original creation of file
*/
#ifndef __BOOT_INFO__
#define __BOOT_INFO__

#ifdef __KILL_BASIC_TYPES__
#undef char
#undef short
#undef long
#endif

typedef struct BootInfo
{
	void*					physAddr;
	unsigned long				pefLen;
	void*					mainTV;
	char*					screenAddr;
	short					pixRes;
	short					rowBytes;
	short					width;
	short					height;
	long						gestaltMachType;
	long						physMemSize;
	long						busSpeed;
	void*					pefKernelCodeSection;
	void*					pefKernelDataSection;
	struct PEFLoaderInfoHeader*	pefKernelLoaderSection;
	unsigned long				kernelDirID;
	unsigned long				kernelVolumeChecksum;
	unsigned long				openFirmwareTreeLen;
	unsigned long				numProcessors;
}BootInfo;

#endif /* __BOOT_INFO__ */