/*
	PEF.cp
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
	PEFBinaryFormat.h		MacOS Universal Headers	Apple			PEFComputeHashWord() function based on this source.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "NKDebuggerNub.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "File Systems.h"
#include "ANSI.h"
#include "PEF.h"
#include "Kernel Console.h"
#include "Macros.h"

typedef Int16 (*PEFInitPtr)(void);		// Must return 0 == noErr to load correctly
typedef Int16 (*PEFMainPtr)(UInt32);		// Can return an error code on completion.  0 == noErr
typedef void (*PEFTerminatePtr)(void);

static	UInt32 PEFComputeHashWord(ConstASCII8Str name);

static	Int8	PEFLookupTable[] = {PEFMaskedBasicOpcodes};					// PEFMakesBasicOpcodes defined in PEFBinaryFormat.h

PEFFragment::PEFFragment(PEFLoaderInfoHeader* _loaderSection,Int8* _codeSection,Int8* _dataSection):
	CodeFragment("Kernel",nil)
{
	// This should only be called by the kernel init stuff
	fragName = "Kernel";
	
	sectionStart[CODE_SECTION] = _codeSection;
	sectionStart[DATA_SECTION] = _dataSection;
	
	loaderHeader = _loaderSection;
	importedLibraryTable = (PEFImportedLibrary*)((Int32)loaderHeader + sizeof(PEFLoaderInfoHeader));
	importedSymbolTable = (PEFImportedSymbol*)((Int32)importedLibraryTable + sizeof(PEFImportedLibrary)*loaderHeader->importedLibraryCount);
	relocationHeadersTable = (PEFLoaderRelocationHeader*)((Int32)importedSymbolTable + sizeof(PEFImportedSymbol)*loaderHeader->totalImportedSymbolCount);
	relocations = (void*)((Int32)loaderHeader + loaderHeader->relocInstrOffset);
	loaderStringTable = (Int8*)((Int32)loaderHeader + loaderHeader->loaderStringsOffset);
	exportHashTable = (PEFExportedSymbolHashSlot*)((Int32)loaderHeader + loaderHeader->exportHashOffset);
	exportKeyTable = (PEFExportedSymbolKey*)((Int32)exportHashTable + sizeof(PEFExportedSymbolHashSlot)*(1 << loaderHeader->exportHashTablePower));
	exportSymbolTable = (PEFExportedSymbol*)((Int32)exportKeyTable + sizeof(PEFExportedSymbolKey)*loaderHeader->exportedSymbolCount);
}

PEFFragment::PEFFragment(FileIStream& s):
	CodeFragment(s.name(),s.file())
{
	loaderHeader = nil;
	importedLibraryTable = nil;
	importedSymbolTable = nil;
	relocationHeadersTable = nil;
	relocations = nil;
	loaderStringTable = nil;
	exportHashTable = nil;
	exportKeyTable = nil;
	exportSymbolTable = nil;
	
	fragName = new ASCII8[strlen(s.name())+1];
	fragName[0] = 0;
	strcat(fragName,s.name());
	
	nkVideo << "New PEF Fragment: " << fragName << "\n";
	
	// Read the PEF header and perform sanity checks on it
	PEFContainerHeader	containerHeader;
	s.read((Int8*)&containerHeader,sizeof(PEFContainerHeader));
	Assert(containerHeader.tag1 == 'Joy!');
	Assert(containerHeader.tag2 == 'peff');
	Assert(containerHeader.architecture == 'pwpc');
	Assert(containerHeader.formatVersion == 1);
	Assert(containerHeader.sectionCount == 3);		// Assumes code section, data section and loader section
	Assert(containerHeader.instSectionCount == 2);	// Assumes only code section and data section are instantiated
	Assert(containerHeader.reservedA == 0);
	
	// Read the section header table
	PEFSectionHeader* sectionHeaderTable;
	sectionHeaderTable = new PEFSectionHeader[containerHeader.sectionCount];
	s.read((Int8*)sectionHeaderTable,sizeof(PEFSectionHeader)*containerHeader.sectionCount);
	for(Int32 i=0;i<containerHeader.sectionCount;i++)
	{
		Assert(sectionHeaderTable[i].defaultAddress == 0);
		switch(i)
		{
			case CODE_SECTION:		Assert(sectionHeaderTable[i].sectionKind == kPEFCodeSection);		break;
			case DATA_SECTION:	Assert(sectionHeaderTable[i].sectionKind == kPEFUnpackedDataSection);	break;	// Don't support compressed data yet
			case LOADER_SECTION:	Assert(sectionHeaderTable[i].sectionKind == kPEFLoaderSection);		break;
		}
	}
	
	// Read all of the section contents
	sectionStart[CODE_SECTION] = sectionStart[DATA_SECTION] = nil;
	for(UInt32 i=0;i<containerHeader.sectionCount;i++)
	{
		UInt32 currSection;
		switch(sectionHeaderTable[i].sectionKind)
		{
			case kPEFCodeSection:		currSection = CODE_SECTION;		break;
			case kPEFUnpackedDataSection:	currSection = DATA_SECTION;		break;
			case kPEFLoaderSection:		currSection = LOADER_SECTION;	break;
		}
		
		s.setPos(sectionHeaderTable[i].containerOffset);
		switch(sectionHeaderTable[i].sectionKind)
		{
			case kPEFCodeSection:
			case kPEFUnpackedDataSection:
				sectionStart[currSection] = new Int8[sectionHeaderTable[i].totalLength];
				s.read(sectionStart[currSection],sectionHeaderTable[i].containerLength);
			break;
			case kPEFLoaderSection:
				// This is the loader header.  Read it in
				loaderHeader = new PEFLoaderInfoHeader;
				s.read((Int8*)loaderHeader,sizeof(PEFLoaderInfoHeader));
				
				// Read the loader imported library table
				importedLibraryTable = new PEFImportedLibrary[loaderHeader->importedLibraryCount];
				s.read((Int8*)importedLibraryTable,sizeof(PEFImportedLibrary)*loaderHeader->importedLibraryCount);
				
				// Read the loader imported symbol table
				importedSymbolTable = new PEFImportedSymbol[loaderHeader->totalImportedSymbolCount];
				s.read((Int8*)importedSymbolTable,sizeof(PEFImportedSymbol)*loaderHeader->totalImportedSymbolCount);
				
				// Read the loader relocation header table
				relocationHeadersTable = new PEFLoaderRelocationHeader[loaderHeader->relocSectionCount];
				s.read((Int8*)relocationHeadersTable,sizeof(PEFLoaderRelocationHeader)*loaderHeader->relocSectionCount);
				
				// Read the loader relocation instructions
				relocations = new UInt16[(loaderHeader->loaderStringsOffset - loaderHeader->relocInstrOffset)/2];
				s.read((Int8*)relocations,loaderHeader->loaderStringsOffset - loaderHeader->relocInstrOffset);
				
				// Read the loader string table
				loaderStringTable = new Int8[loaderHeader->exportHashOffset - loaderHeader->loaderStringsOffset];
				s.read(loaderStringTable,loaderHeader->exportHashOffset - loaderHeader->loaderStringsOffset);
				
				// Read the export hash table
				exportHashTable = new PEFExportedSymbolHashSlot[1L << loaderHeader->exportHashTablePower];
				s.read((Int8*)exportHashTable,sizeof(PEFExportedSymbolHashSlot)*(1L << loaderHeader->exportHashTablePower));
				
				// Read the export key table
				if(loaderHeader->exportedSymbolCount)
				{
					exportKeyTable = new PEFExportedSymbolKey[loaderHeader->exportedSymbolCount];
					s.read((Int8*)exportKeyTable,sizeof(PEFExportedSymbolKey)*loaderHeader->exportedSymbolCount);
				}
				
				// Read the export symbol table
				if(loaderHeader->exportedSymbolCount)
				{
					exportSymbolTable = new PEFExportedSymbol[loaderHeader->exportedSymbolCount];
					s.read((Int8*)exportSymbolTable,sizeof(PEFExportedSymbol)*loaderHeader->exportedSymbolCount);
				}
			break;
			default:
				Panic("Shouldn't have gotten here in PEFFragment::PEFFragment()!\n");
			break;
		}
	}
	
	prepare();
	
	NKFlushCaches(sectionStart[CODE_SECTION],sectionHeaderTable[CODE_SECTION].containerLength);
	
	delete [] sectionHeaderTable;
	
	nkVideo << "\tCode section: " << (UInt32)sectionStart[CODE_SECTION] << "\n";
	nkVideo << "\tData section: " << (UInt32)sectionStart[DATA_SECTION] << "\n";
}

PEFFragment::~PEFFragment()
{
}

void PEFFragment::prepare()
{
	// Load imported libraries
	for(Int32 i=0;i<loaderHeader->importedLibraryCount;i++)
	{
		Boolean		isWeakLink = ((importedLibraryTable[i].options & kPEFWeakImportLibMask) != 0);
		ASCII8Str	libName = loaderStringTable + importedLibraryTable[i].nameOffset;
		importLibrary(libName,isWeakLink);
	}
	
	// Perform relocations
	for(Int32 i=0;i<loaderHeader->relocSectionCount;i++)
	{
		resetVM(i);
		performRelocations(relocationHeadersTable[i].relocCount);
	}
	
	// Call the init routine
	if(loaderHeader->initSection != -1)
	{
		PEFInitPtr init = (PEFInitPtr)(sectionStart[loaderHeader->initSection] + loaderHeader->initOffset);
		Int16 initVal = (*init)();
		Assert(initVal == 0);	// Return of a non-zero number indicates Init failure
	}
}

void PEFFragment::main(UInt32 arg1)
{
	if(loaderHeader->mainSection != -1)
	{
		PEFMainPtr mainAddr = (PEFMainPtr)(sectionStart[loaderHeader->mainSection] + loaderHeader->mainOffset);
		(*mainAddr)(arg1);
	}
}

void PEFFragment::resetVM(UInt32 relocHeaderIndex)
{
	Assert(relocHeaderIndex < loaderHeader->relocSectionCount);
	Assert(relocationHeadersTable[relocHeaderIndex].sectionIndex != LOADER_SECTION);
	
	inst = (UInt16*)((UInt32)relocations + relocationHeadersTable[relocHeaderIndex].firstRelocOffset);
	relocAddress = (UInt32*)sectionStart[relocationHeadersTable[relocHeaderIndex].sectionIndex];
	relocSectionStart = (Int8*)relocAddress;
	importIndex = 0;
	sectionC = (UInt32)sectionStart[CODE_SECTION];
	sectionD = (UInt32)sectionStart[DATA_SECTION];
}

void PEFFragment::performRelocations(UInt32 instCount)
{
	while(instCount > 0)
	{
		switch(PEFLookupTable[ (*inst & 0xFE00) >> 9])
		{
			case kPEFRelocBySectDWithSkip:	RelocBySectDWithSkip();	instCount--;	inst++;	break;
			case kPEFRelocBySectC:
			case kPEFRelocBySectD:
			case kPEFRelocTVector12:
			case kPEFRelocTVector8:
			case kPEFRelocVTable8:
			case kPEFRelocImportRun:			RelocRun();			instCount--;	inst++;	break;
			case kPEFRelocSmByImport:
			case kPEFRelocSmSetSectC:
			case kPEFRelocSmSetSectD:
			case kPEFRelocSmBySection:		RelocSmIndex();		instCount--;	inst++;	break;
			case kPEFRelocIncrPosition:		RelocIncrPosition();		instCount--;	inst++;	break;
			case kPEFRelocSmRepeat:			RelocSmRepeat();		instCount--;	inst++;	break;
			case kPEFRelocSetPosition:		RelocSetPosition();		instCount -= 2;	inst += 2;	break;
			case kPEFRelocLgByImport:		RelocLgByImport();		instCount -= 2;	inst += 2;	break;
			case kPEFRelocLgRepeat:			RelocLgRepeat();		instCount -= 2;	inst += 2;	break;
			case kPEFRelocLgSetOrBySection:	RelocLgSetOrBySection();	instCount -= 2;	inst += 2;	break;
			default:	Panic("Illegal PEF relocation instruction!\n");							return;
		}
	}
}

void PEFFragment::RelocBySectDWithSkip(void)
{
	register UInt16 count = (*inst & 0x003F);
	
	relocAddress += ( (*inst >> 6) & 0x00FF);
	for(UInt16 i=0;i<count;i++)
		*relocAddress++ += sectionD;
}

void PEFFragment::RelocRun(void)
{
	register UInt16 count = (*inst & 0x01FF);
	register UInt8 subOpcode = ( (*inst >> 9) & 0x0F);
	
	for(UInt16 i=0;i<=count;i++)
	{
		switch(subOpcode)
		{
			case 0:	*relocAddress++ += sectionC;											break;
			case 1:	*relocAddress++ += sectionD;											break;
			case 2:	*relocAddress++ += sectionC; *relocAddress++ += sectionD; relocAddress++;	break;
			case 3:	*relocAddress++ += sectionC; *relocAddress++ += sectionD;					break;
			case 4:	*relocAddress++ += sectionD; relocAddress++;							break;
			case 5:	*relocAddress++ += (UInt32)getImportedSymAddr(importIndex++);		break;
			default:	Panic("Illegal sub-opcode for a RelocRun PEF instruction!\n");					break;
		}
	}
}

void PEFFragment::RelocSmIndex(void)
{
	register UInt8 subOpcode = ( (*inst >> 9) & 0x0F );
	register UInt32 index = (*inst & 0x0001FF);
	
	switch(subOpcode)
	{
		case 0:	*relocAddress++ += (UInt32)getImportedSymAddr(index); importIndex = index+1;			break;
		case 1:	sectionC = (UInt32)sectionStart[index];											break;
		case 2:	sectionD = (UInt32)sectionStart[index];											break;
		case 3:	*relocAddress++ += (UInt32)sectionStart[index];									break;
		default:	Panic("Illegal sub-opcode for a RelocSmIndex PEF instruction!\n");							break;
	}
}

void PEFFragment::RelocIncrPosition(void)
{
	relocAddress = (UInt32*)((UInt32)relocAddress + (*inst & 0x0FFF) + 1);
}

void PEFFragment::RelocSmRepeat(void)
{
	register UInt32	blockLen = ((*inst >> 8) & 0x000F) + 1;
	register UInt32	count = (*inst & 0x00FF);
	
	inst -= blockLen;
	
	for(UInt32 i=0;i<=count;i++)
		performRelocations(blockLen);
}

void PEFFragment::RelocSetPosition(void)
{
	relocAddress = (UInt32*)(relocSectionStart + ( ((inst[0] & 0x000003FF) << 16) | (inst[1] & 0x0000FFFF) ) );
}

void PEFFragment::RelocLgByImport(void)
{
	importIndex = ((inst[0] & 0x000003FF) << 16) | (inst[1] & 0x0000FFFF);
	*relocAddress++ += (UInt32)getImportedSymAddr(importIndex++);
}

void PEFFragment::RelocLgRepeat(void)
{
	register UInt32	blockLen = ((*inst >> 8) & 0x000F) + 1;
	register UInt32	count = ((inst[0] & 0x0000003F) << 16) | (inst[1] & 0x0000FFFF);
	
	inst -= blockLen;
	
	for(UInt32 i=0;i<count;i++)
		performRelocations(blockLen);
}

void PEFFragment::RelocLgSetOrBySection(void)
{
	register UInt8 subOpcode = ( (*inst >> 6) & 0x0F );
	register UInt32 index = ((inst[0] & 0x0000003F) << 16) | (inst[1] & 0x0000FFFF);
	
	switch(subOpcode)
	{
		case 0:	*relocAddress++ += (UInt32)getImportedSymAddr(index); importIndex = index+1;				break;
		case 1:	sectionC = (UInt32)sectionStart[index];												break;
		case 2:	sectionD = (UInt32)sectionStart[index];												break;
		default:	Panic("Illegal sub-opcode for a RelocLgSetOrBySection PEF instruction!\n");						break;
	}
}

void PEFFragment::importLibrary(ConstASCII8Str name,Boolean isWeakLink)
{
	CodeFragment*	frag = getFragment(name);
	if(!frag)
	{
		if(fileDesc)
		{
			DirectoryDescriptor*	dd = fileDesc->parent();
			FileDescriptor*			fd = dd->subFile(name);
			if(fd)
			{
				FileIStream*	iStream = fd->openForRead();
				PEFFragment*	tempFrag;
				tempFrag = new PEFFragment(*iStream);
				delete iStream;
				delete fd;
			}
			else if(!isWeakLink)
				Panic("Couldn't import a PEF library!\n");
			
			delete dd;
		}
		else
			Panic("Couldn't find an imported PEF library!\n");
	}
}

Int8* PEFFragment::getImportedSymAddr(UInt32 index)
{
	ASCII8Str	symName = loaderStringTable + (importedSymbolTable[index].classAndName & 0x00FFFFFF);
	Int8*	addr = nil;
	
	for(Int32 i=0;i<loaderHeader->importedLibraryCount;i++)
	{
		ASCII8Str		fragName = loaderStringTable + importedLibraryTable[i].nameOffset;
		CodeFragment*	frag = getFragment(fragName);
		FatalAssert(frag != nil);
		if((addr = (Int8*)frag->getSymAddr(symName)) != nil)
			return addr;
	}
	
	nkVideo << "Symbol name: " << symName << "\n";
	Panic("Failed to find an imported symbol!\n");
}

void* PEFFragment::getSymAddr(ConstASCII8Str symName)
{
	// Return a pointer to some symbol that we export
	UInt32 hashWord = PEFComputeHashWord(symName);
	UInt32 index = PEFHashTableIndex(hashWord,loaderHeader->exportHashTablePower);
	UInt32 chainCount = ((exportHashTable[index].countAndStart >> 18) & 0x00003FFF);
	UInt32 firstIndex = (exportHashTable[index].countAndStart & 0x0003FFFF);
	
	for(UInt32 i=0;i<chainCount;i++)
	{
		if(exportKeyTable[firstIndex+i].u.fullHashWord == hashWord)
		{
			if(!strncmp(loaderStringTable + (exportSymbolTable[firstIndex+i].classAndName & 0x00FFFFFF),symName,(hashWord >> 16) & 0x0000FFFF))
			{
				switch(exportSymbolTable[firstIndex+i].sectionIndex)
				{
					case -2:	// symbolValue contains an absolute address
						return (void*)exportSymbolTable[firstIndex+i].symbolValue;
					case -3:	// symbolValue contains an imported symbol index, meaning this is a re-exported symbol
						return getImportedSymAddr(exportSymbolTable[firstIndex+i].symbolValue);
					case CODE_SECTION:
					case DATA_SECTION:
						return (void*)((Int32)sectionStart[exportSymbolTable[firstIndex+i].sectionIndex] + exportSymbolTable[firstIndex+i].symbolValue);
					case LOADER_SECTION:
						return (void*)((Int32)loaderHeader + exportSymbolTable[firstIndex+i].symbolValue);
					default:
						Panic("Don't know how to locate a PEF symbol!\n");
				}
			}
		}
	}
	
	return nil;
}

// The following routine is from Apple Computer:
static UInt32 PEFComputeHashWord(ConstASCII8Str name)
{																								
	const ASCII8*	p = name;
	Int32		hashValue = 0;
	UInt32		length = 0;
	UInt32		result;
	ASCII8		currChar;
	
	#define PseudoRotate(x)  (((x) << 1) - ((x) >> 16))
	
	for(;;)
	{
		currChar = *p++;
		if(!currChar)
			break;															
		length++;
		hashValue = PseudoRotate(hashValue)^currChar;
	}
	
	result = (length << kPEFHashLengthShift) |	((UInt16) ((hashValue^(hashValue >> 16)) & kPEFHashValueMask));
	
	return result;
}
