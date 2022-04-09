/*
	Compiler.h
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
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __COMPILER__
#define __COMPILER__

#include "Config.h"
#include "Kernel Types.h"

// All compiler-specific stuff goes in this header

// Stuff for Metrowerks Codewarrior
#ifdef __MWERKS__

#define	InitCPPGlobals		__sinit		// InitCPPGlobals is the function you need to call to construct all global C++ objects
#define	__inline__		inline		// __inline__ is used wherever there is a function that should be inlined.  Note:  __inline__ does not guarantee that the function WILL be inlined
#define	__asm__			asm			// __asm__ is used wherever there is a function that should be interpreted as assembly
#define	BIG_ENDIAN		1			// BIG_ENDIAN should be set to 1 to make a big-endian kernel.  Currently, little-endian kernels are not supported

#if __MWERKS__ > 0x1800		// CW Pro 1 corresponds to 0x1800.  If CW Pro 2 doesn't support new[] or delete[] then we should bump this up
#define	ARRAY_NEW_DELETE_SUPPORTED	1
#else
#define	ARRAY_NEW_DELETE_SUPPORTED	0
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	void	__sinit(void);
#ifdef __cplusplus
}
#endif

// For reading registers
inline asm Int8		ReadReg8(register Reg8* reg)			{lbz r3,0(r3); extsb r3,r3; eieio; blr;}
inline asm UInt8	ReadUReg8(register UReg8* reg)		{lbz r3,0(r3); eieio; blr;}
inline asm Int16	ReadReg16LE(register Reg16LE* reg)	{lhbrx r3,r0,r3; extsh r3,r3; eieio; blr;}
inline asm Int16	ReadReg16BE(register Reg16BE* reg)	{lha r3,0(r3); eieio; blr;}
inline asm UInt16	ReadUReg16LE(register UReg16LE* reg)	{lhbrx r3,r0,r3; eieio; blr;}
inline asm UInt16	ReadUReg16BE(register UReg16BE* reg)	{lhz r3,0(r3); eieio; blr;}
inline asm Int32	ReadReg32LE(register Reg32LE* reg)	{lwbrx r3,r0,r3; eieio; blr;}
inline asm Int32	ReadReg32BE(register Reg32BE* reg)	{lwz r3,0(r3); eieio; blr;}
inline asm UInt32	ReadUReg32LE(register UReg32LE* reg)	{lwbrx r3,r0,r3; eieio; blr;}
inline asm UInt32	ReadUReg32BE(register UReg32BE* reg)	{lwz r3,0(r3); eieio; blr;}

// For writing registers
inline asm void		WriteReg8(register Int8 val,register Reg8* reg)			{stb r3,0(r4); eieio; blr;}
inline asm void		WriteUReg8(register UInt8 val,register UReg8* reg)			{stb r3,0(r4); eieio; blr;}
inline asm void		WriteReg16LE(register Int16 val,register Reg16LE* reg)		{sthbrx r3,r0,r4; eieio; blr;}
inline asm void		WriteReg16BE(register Int16 val,register Reg16BE* reg)		{sth r3,0(r4); eieio; blr;}
inline asm void		WriteUReg16LE(register UInt16 val,register UReg16LE* reg)	{sthbrx r3,r0,r4; eieio; blr;}
inline asm void		WriteUReg16BE(register UInt16 val,register UReg16BE* reg)	{sth r3,0(r4); eieio; blr;}
inline asm void		WriteReg32LE(register Int32 val,register Reg32LE* reg)		{stwbrx r3,r0,r4; eieio; blr;}
inline asm void		WriteReg32BE(register Int32 val,register Reg32BE* reg)		{stw r3,0(r4); eieio; blr;}
inline asm void		WriteUReg32LE(register UInt32 val,register UReg32LE* reg)	{stwbrx r3,r0,r4; eieio; blr;}
inline asm void		WriteUReg32BE(register UInt32 val,register UReg32BE* reg)	{stw r3,0(r4); eieio; blr;}

// For reading data
inline asm Int16	ReadInt16LE(register Int16* addr)		{lhbrx r3,r0,r3; extsh r3,r3; blr;}
inline asm UInt16	ReadUInt16LE(register UInt16* addr)	{lhbrx r3,r0,r3; blr;}
inline asm Int32	ReadInt32LE(register Int32* addr)		{lwbrx r3,r0,r3; blr;}
inline asm UInt32	ReadUInt32LE(register UInt32* addr)	{lwbrx r3,r0,r3; blr;}

// For writing data
inline asm void		WriteInt16LE(register Int16 val,register Int16* addr)		{sthbrx r3,r0,r4; blr;}
inline asm void		WriteUInt16LE(register UInt16 val,register UInt16* addr)	{sthbrx r3,r0,r4; blr;}
inline asm void		WriteInt32LE(register Int32 val,register Int32* addr)		{stwbrx r3,r0,r4; blr;}
inline asm void		WriteUInt32LE(register UInt32 val,register UInt32* addr)	{stwbrx r3,r0,r4; blr;}

// This is solely for PEFBinaryFormat.h, which is a MacOS Universal Header file
#ifndef	PRAGMA_ALIGN_SUPPORTED
#define	PRAGMA_ALIGN_SUPPORTED	1
#endif
#ifndef	PRAGMA_IMPORT_SUPPORTED
#define	PRAGMA_IMPORT_SUPPORTED	1
#endif

#else

#error	Need some #defines to make this compiler work!

#endif /* __MWERKS__ */

#endif /* __COMPILER__ */
