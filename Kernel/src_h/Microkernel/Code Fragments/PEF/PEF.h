/*
	PEF.h
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
#ifndef __PEF__
#define __PEF__

#include "Kernel Types.h"
#include "Code Fragments.h"
#include "PEFBinaryFormat.h"

#define	CODE_SECTION		0				// The PEF section number of the code for this PEF fragment (These are not defined by PEF.  If your compiler outputs them differently change these.)
#define	DATA_SECTION		1				// The PEF section number of the data for this PEF fragment
#define	LOADER_SECTION	2				// The PEF section number of the loader info for this PEF fragment

class PEFFragment	:	public CodeFragment
{
	Int8*					sectionStart[2];
	
	// Loader section stuff
	PEFLoaderInfoHeader*		loaderHeader;
	PEFImportedLibrary*		importedLibraryTable;
	PEFImportedSymbol*		importedSymbolTable;
	PEFLoaderRelocationHeader*	relocationHeadersTable;
	void*					relocations;
	Int8*					loaderStringTable;
	PEFExportedSymbolHashSlot*	exportHashTable;
	PEFExportedSymbolKey*		exportKeyTable;
	PEFExportedSymbol*		exportSymbolTable;
	
	// PEF virtual machine registers
	UInt16*					inst;
	UInt32*					relocAddress;
	Int8*					relocSectionStart;
	UInt32					importIndex;
	UInt32					sectionC;
	UInt32					sectionD;
	
	PEFFragment(PEFLoaderInfoHeader* loaderSection,Int8* codeSection,Int8* dataSection);	// Only to be called by the kernel FOR the kernel PEF fragment
	PEFFragment(class FileIStream& s);		// Only to be called by the kernel for a new Process
	
			void		importLibrary(ConstASCII8Str name,Boolean isWeakLink);
			void		prepare();
			void		resetVM(UInt32 relocHeaderIndex);
			void		performRelocations(UInt32 instCount);
			Int8*	getImportedSymAddr(UInt32 importIndex);
			
			// PEF VM instructions
			void		RelocBySectDWithSkip();
			void		RelocRun();
			void		RelocSmIndex();
			void		RelocIncrPosition();
			void		RelocSmRepeat();
			void		RelocSetPosition();
			void		RelocLgByImport();
			void		RelocLgRepeat();
			void		RelocLgSetOrBySection();
public:
	
	virtual ~PEFFragment();
	
	virtual	void		main(UInt32 arg1);
	virtual	void*	getSymAddr(ConstASCII8Str symName);
	
	friend	void			InitCodeFragments();
	friend	UInt32		InitOSs(class User* theUser);
};

#endif /* __PEF__ */