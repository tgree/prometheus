/*
	main.cp
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
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Wed, 2 Feb 2000	-	Removed a bunch of unused functions (mostly to figure out the boot device).
										-	Changed checksum code to use only static parts of the MDB.
*/
#include "Types.h"
#include "ANSI.h"
#include <SCSI.h>
#include <PEFBinaryFormat.h>
#include <ShutDown.h>
#include <Palettes.h>
#include "PPCEnterSupervisorMode.h"
#include "Assembly.h"
#include "Macros.h"
#include <ATA.h>
#include "OpenFirmware.h"
#include <NameRegistry.h>
#define _STDARG	// So we don't reinclude some ansi crap
//#include "Multiprocessing.h"
#include "BootInfo.h"

// WARNING: The PEF kernel binary MUST be compiled with "Expand Uninitialized Data" checked.  This simple PEF loader does NOT handle pattern-initialized data expansion.
// You WILL crash if you compress the kernel data.  This will be addressed in a future release - for now a working booter is all that we need.

#define	BASE_ADDRESS	0x00200000		// The base physical address at which to load the PEF fragment (2 MB).  This must be 1MB aligned.
#define	KERNEL_MAXLEN	0x00100000		// The kernel and stack image must be shorter than this number, which is used in the BAT mapping.  It must be >= 128K and <= 8MB and a power of 2.
#define	CODE_SECTION		0				// The PEF section number of the code for this PEF fragment (These are not defined by PEF.  If your compiler outputs them differently change these.)
#define	DATA_SECTION		1				// The PEF section number of the data for this PEF fragment
#define	LOADER_SECTION	2				// The PEF section number of the loader info for this PEF fragment

typedef struct RelocBySectDWithSkipInst
{
	unsigned short	zero			:	2;
	unsigned short	skipCount		:	8;
	unsigned short	relocCount	:	6;
}RelocBySectDWithSkipInst;

typedef struct RelocRunInst
{
	unsigned short	two				:	3;
	unsigned short	subOpcode			:	4;
	unsigned short	runLengthMinusOne	:	9;
}RelocRunInst;

typedef struct RelocSmIndexInst
{
	unsigned short	three		:	3;
	unsigned short	subOpcode		:	4;
	unsigned short	index		:	9;
}RelocSmIndexInst;

typedef struct RelocIncrPositionInst
{
	unsigned short	eight			:	4;
	unsigned short	offsetMinusOne	:	12;
}RelocIncrPositionInst;

typedef struct RelocSmRepeatInst
{
	unsigned short	nine					:	4;
	unsigned short	blockCountMinusOne		:	4;
	unsigned short	repeatCountMinusOne	:	8;
}RelocSmRepeatInst;

typedef struct RelocSetPositionInst
{
	unsigned long	forty	:	6;
	unsigned long	offset	:	26;
}RelocSetPositionInst;

typedef struct RelocLgByImportInst
{
	unsigned long	fortyOne	:	6;
	unsigned long	index	:	26;
}RelocLgByImportInst;

typedef struct RelocLgRepeatInst
{
	unsigned long	fortyFour			:	6;
	unsigned long	blockCountMinusOne	:	4;
	unsigned long	repeatCount		:	22;
}RelocLgRepeatInst;

typedef struct RelocLgSetOrBySectionInst
{
	unsigned long	fortyFive	:	6;
	unsigned long	subOpcode	:	4;
	unsigned long	index	:	22;
}RelocLgSetOrBySectionInst;

#define	ROUND_UP_POW2(num,pow)	( (((long)(num)) & ((1 << (pow)) - 1)) ? ( (((long)(num)) & ~((1 << (pow)) - 1)) + (1 << (pow)) ) : ((long)(num)) )

static	void			InitToolbox(void);
static	void			PreparePEFFragment(const unsigned char* pefName,PEFLoaderInfoHeader** loaderSection,char** codeSection,char** dataSection,unsigned long endPadding,char** pefBaseAddr,unsigned long* pefLen);
static	void			Relocate(char* codeSection,char* dataSection,char* loaderSection);
static	unsigned char	ProcessOpcode(unsigned char* currInst,char*& relocAddress,char*& sectionC,char*& sectionD,char** sections);
static	void			MoveKernelToZero(void);
static	void			EndMoveKernelToZero(void);
static	void			CallMoveKernelToZero(register BootInfo* bootInfo,register BootInfo* booterBootInfo,register void* funcAddr,register unsigned long processorID);
static	void			Message(Str255 err);
static	void			WaitMessage(Str255 err);
static	void			UnrecoverableError(Str255 err);
static	pascal	void	ShutDownProc(short);
static	void*		GetPhysicalAddress(void* logicalAddr);
static	void			MakeOpenFirmwareTree();
static	void			SetUpMPStuff();

// These vars get passed on to the nanokernel
BootInfo	bootInfo;

BootInfo*					kernelBootInfo;
static OpenFirmwareTree*	openFirmwareTree;
unsigned long*				blockMoveZeroCode;

void InitToolbox(void)
{
	InitGraf(&qd.thePort);
	InitFonts();
	FlushEvents(everyEvent,false);
	InitWindows();
	TEInit();
	InitDialogs(nil);
	InitCursor();
}

void main(void)
{
	// If option key is held down, bypass the ShutDwnPower() sequence.
	KeyMap	theKeys;
	GetKeys(theKeys);
	char		skipShutdown = ((theKeys[1] & 4) == 4);
	
	// Initialize the toolbox
	InitToolbox();
	
	// Set up our dialog
	DialogPtr theDlog = GetNewDialog(128,nil,(DialogPtr)-1L);
	SelectWindow(theDlog);
	SetPort((GrafPtr)theDlog);
	SetDialogDefaultItem(theDlog,1);
	TextFont(kFontIDGeneva);
	TextSize(9);
	MoveTo(15,135);
	DrawString("\pPrometheus/Pandora Booter");
	DrawDialog(theDlog);
	
	// Get some arguments that are passed on to the booter
	GDHandle		mainDev = GetMainDevice();
	
	bootInfo.pixRes = (**(**mainDev).gdPMap).pixelSize;
	
	// Make the OpenFirmware tree
	MakeOpenFirmwareTree();
	
	// Actually prepare the PEF fragment
	PEFLoaderInfoHeader*	loaderSection;
	char*				codeSection;
	char*				dataSection;
	unsigned long			moveKernelToZeroLen = FUNC_ADDR(EndMoveKernelToZero) - FUNC_ADDR(MoveKernelToZero);
	char*				pefBaseAddr;
	
	PreparePEFFragment("\p::Kernel:Kernel",&loaderSection,&codeSection,&dataSection,moveKernelToZeroLen + bootInfo.openFirmwareTreeLen + sizeof(BootInfo),&pefBaseAddr,&bootInfo.pefLen);
	kernelBootInfo = (BootInfo*)((unsigned long)pefBaseAddr + bootInfo.pefLen - sizeof(BootInfo));
	bootInfo.pefKernelCodeSection = (void*)BASE_ADDRESS;
	bootInfo.pefKernelDataSection = (void*)(dataSection - codeSection + BASE_ADDRESS);
	bootInfo.pefKernelLoaderSection = (PEFLoaderInfoHeader*)((unsigned long)loaderSection - (unsigned long)codeSection + BASE_ADDRESS);
	
	if((bootInfo.pefLen + 0x00010000) & 0xFFFFF000 > KERNEL_MAXLEN)
		UnrecoverableError("\pKernel length > KERNEL_MAXLEN, update #define KERNEL_MAXLEN");
	
	// Set the screen depth to 8 bits
	if(bootInfo.pixRes != 8)
	{
		short		depthID = HasDepth(mainDev,8,1,1);
		if(!depthID)
			UnrecoverableError("\pYour monitor doesn't appear to support 8 bits/pixel (256 colors)!");
		if(SetDepth(mainDev,depthID,1,1))
			UnrecoverableError("\pFailed to set your monitor to 8 bits/pixel (256 colors)!");
		bootInfo.pixRes = 8;
		
		MoveTo(15,135);
		DrawString("\pPrometheus/Pandora Booter");
		DrawDialog(theDlog);
	}
	
	bootInfo.screenAddr = (**(**mainDev).gdPMap).baseAddr;
	bootInfo.rowBytes = (**(**mainDev).gdPMap).rowBytes & 0x3FFF;
	bootInfo.width = (**mainDev).gdRect.right - (**mainDev).gdRect.left;
	bootInfo.height = (**mainDev).gdRect.bottom - (**mainDev).gdRect.top;
	Gestalt(gestaltMachineType,&bootInfo.gestaltMachType);
	if(bootInfo.gestaltMachType == 512)	// Could be a 20th anniv mac
	{
		long temp;
		if(!Gestalt('bxid',&temp))
			if(temp == 0x0000302B)
				bootInfo.gestaltMachType |= 0x80000000;
	}
	Gestalt(gestaltPhysicalRAMSize,&bootInfo.physMemSize);
	Gestalt('bclk',&bootInfo.busSpeed);
	
	// Set up our block move to zero code (we actually move to address BASE_ADDR)
	blockMoveZeroCode = (unsigned long*)(pefBaseAddr + bootInfo.pefLen - moveKernelToZeroLen - bootInfo.openFirmwareTreeLen - sizeof(BootInfo));
	BlockMove((void*)FUNC_ADDR(MoveKernelToZero),blockMoveZeroCode,moveKernelToZeroLen);
	MakeDataExecutable(blockMoveZeroCode,moveKernelToZeroLen);
	
	// Flatten the OpenFirmware tree
	if(openFirmwareTree)
		openFirmwareTree->flattenTree((char*)((unsigned long)pefBaseAddr + bootInfo.pefLen - bootInfo.openFirmwareTreeLen - sizeof(BootInfo)));
	
	bootInfo.physAddr = (void*)pefBaseAddr;
	bootInfo.mainTV = (void*)(dataSection - codeSection + loaderSection->mainOffset + BASE_ADDRESS);
	
	// Get the directory number of the kernel directory
	WDPBRec	wdPbRec;
	Str255	volName;
	for(long i=0;i<sizeof(WDPBRec);i++)
		((char*)&wdPbRec)[i] = 0;
	wdPbRec.ioNamePtr = volName;
	PBHGetVolSync(&wdPbRec);
	bootInfo.kernelDirID = wdPbRec.ioWDDirID;
	
	// Get the driver reference number for the volume we are booting from
	HParamBlockRec	pb;
	for(long i=0;i<sizeof(HVolumeParam);i++)
		((char*)&pb)[i] = 0;
	pb.volumeParam.ioNamePtr = volName;
	pb.volumeParam.ioVRefNum = wdPbRec.ioVRefNum;
	PBHGetVInfoSync(&pb);
	
	UInt16	drvrRefNum = pb.volumeParam.ioVDRefNum;
	UInt16	drvNum = pb.volumeParam.ioVDrvInfo;
	
	// Now, try to read the first block of the device
	Ptr			theBlockPtr = NewPtr(1024);
	Ptr			theAlignedPtr = theBlockPtr;//(Ptr)ROUND_UP(512,(UInt32)theBlockPtr);
	ParamBlockRec	readRec;
	for(long i=0;i<sizeof(readRec);i++)
		((char*)&readRec)[i] = 0;
	readRec.ioParam.ioVRefNum = drvNum;
	readRec.ioParam.ioRefNum = drvrRefNum;
	readRec.ioParam.ioBuffer = theAlignedPtr;
	readRec.ioParam.ioReqCount = 512;
	readRec.ioParam.ioPosMode = fsFromStart;
	readRec.ioParam.ioPosOffset = 1024;
	OSErr		readRecErr = PBReadSync(&readRec);
	if(readRecErr)
		Message("\pWarning: error reading MDB block of boot device.");
	
	// Compute a checksum on the MDB block of the device - we use this to identify the device in the kernel
	// to find the booter folder
	UInt64					checkSum = 0;
	HFSMasterDirectoryBlock*	mdbPtr = (HFSMasterDirectoryBlock*)theAlignedPtr;
	checkSum += mdbPtr->drSigWord;
	checkSum += mdbPtr->drCrDate;
	checkSum += mdbPtr->drNmAlBlks;
	checkSum += mdbPtr->drAlBlkSiz;
	UInt32*					checkSumPtr = (UInt32*)&mdbPtr->drVN[0];
	for(UInt32 i =0;i<7;i++)	// Checksum the name
		checkSum += *checkSumPtr++;
	while(checkSum & 0xFFFFFFFF00000000)
		checkSum = (checkSum >> 32) + (checkSum & 0x00000000FFFFFFFF);
	bootInfo.kernelVolumeChecksum = checkSum;
	
	InitSuperCode();
	
	// Do the MP stuff
	SetUpMPStuff();
	
	BlockMove((Ptr)&bootInfo,(Ptr)kernelBootInfo,sizeof(BootInfo));
	kernelBootInfo = (BootInfo*)((unsigned long)BASE_ADDRESS + ((unsigned long)kernelBootInfo - (unsigned long)pefBaseAddr));
	
	if(skipShutdown)
		ShutDownProc(0);
	
	// Display a pretty message
	Message("\pWaiting for MacOS to shut down.");
	
	// Shut Down MacOS
	ShutDwnInstall(NewShutDwnProc(ShutDownProc),sdRestartOrPower);
	ShutDwnStart();
	for(;;)
		;
}

// These two functions (CallMoveKernelToZero and MoveKernelToZero) are pretty screwed up right now.  MoveKernelToZero is actually supposed to reside at the end of the
// Kernel's reserved mem, so that it doesn't overwrite itself during the copy process (assuming this code COULD be in the first 50K of memory).  But that crashed all the time,
// so right now it lives right here in the app's code section.  We assume that it won't be within the first 50K of memory.
asm void CallMoveKernelToZero(register BootInfo* kernelBootInfo,register BootInfo* booterBootInfo,register void* funcAddr,register unsigned long processorID)
{
	mtctr	r5;
	bctr;
}

asm void MoveKernelToZero(void)
{
	li		r0,0x1070;
	sync;
	mtmsr	r0;
	sync;
	isync;
	
	// Save our arguments for later
	mr		r31,r3;					// kernelBootInfo
	mr		r27,r4;					// booterBootInfo
	mr		r30,r6;					// processorID
	lwz		r29,BootInfo.mainTV(r27);	// mainTV
	lwz		r28,BootInfo.screenAddr(r27);	// screenAddr
	
	// Map the first KERNEL_MAXLEN bytes of memory from BASE_ADDRESS 1-1
	mfpvr(r3);
	rlwinm	r3,r3,16,16,31;
	cmpwi	r3,1;
	beq-		@601Map;
	
@non601Map:
	li		r3,0;
	isync;
	mtdbatu(3,r3);	// Disable dbatu3
	mtibatu(3,r3);	// Disable ibatu3
	sync;
	lis		r3,UPPER_HW(BASE_ADDRESS);
	rlwinm	r3,r3,0,0,14;
	ori		r3,r3,0x0012;		// batu3: BRPN = (BASE_ADDRESS & 0xFFFFC000), WIMG = M, PP = r/w
	mtdbatl(3,r3);
	mtibatl(3,r3);
	lis		r3,UPPER_HW(BASE_ADDRESS);
	rlwinm	r3,r3,0,0,14;
	ori		r3,r3,((KERNEL_MAXLEN - 1) >> 15);		// batl3: BEPI = (BASE_ADDRESS & 0xFFFC000), BL = KERNEL_MAXLEN, Vs = Vp = true
	isync;
	mtdbatu(3,r3);
	mtibatu(3,r3);
	sync;
	b		@moveKernel;
@601Map:
	isync;
	li		r3,0;
	mtibatl(3,r3);				// Disable ibat3
	sync;
	lis		r3,UPPER_HW(BASE_ADDRESS);
	rlwinm	r3,r3,0,0,14;
	ori		r3,r3,0x001E;		// ibatu3: BLPI = (BASE_ADDRESS & 0xFFFFC000), WIM = M, Ks = Ku = 1, PP = r/w
	mtibatu(3,r3);
	lis		r3,UPPER_HW(BASE_ADDRESS);
	rlwinm	r3,r3,0,0,14;
	ori		r3,r3,(0x0040 | ((KERNEL_MAXLEN - 1) >> 17));		// ibatl3: PBN = (BASE_ADDRESS & 0xFFFFC000), V = true, BSM = KERNEL_MAXLEN
	isync;
	mtibatl(3,r3);
	sync;
	b		@moveKernel;
	
@moveKernel:
	// If r30 (processorID) == 0, we should only flush the kernel memory
	cmpwi	r30,0;
	beq		@flushKernel;
	
	// Move the kernel text to address BASE_ADDRESS
	lwz		r4,BootInfo.pefLen(r27);
	srwi(r4,r4,2);
	mtctr	r4;
	lwz		r3,BootInfo.physAddr(r27);
	subi		r3,r3,4;
	lis		r5,UPPER_HW(BASE_ADDRESS-4);
	ori		r5,r5,LOWER_HW(BASE_ADDRESS-4);
	
	// Set up the MSR - leave only relocation and ROM interrupt prefix on, all other interrupts and things go off!  We leave the
	// ROM interrupt prefix on so that MacOS will continue to handle all 603 memory management interrupts until we're ready
	// to do so!
	//
	// *** Another possibility: Turn off data relocation but leave instruction relocation on.  Then we need to physical address
	// of the kernel passed in here, right now we have the logical address.
	li		r6,0x1070;
	sync;
	mtmsr	r6;
	sync;
	
	@copyLoop:
		lwzu		r4,4(r3);	// Read next word
		stwu		r4,4(r5);	// Write next word
		dcbf		r0,r5;
		sync;
		icbi		r0,r5;
		sync;
		bdnz		@copyLoop;
	
	addi	r0,r5,4;	// Save kernelPreStack for kernel
	
	// Make an (approx.) 65K stack for our personal use.  Top of stack contains a 0 back-chain.  Make a frame too.  This stack will be between 60K and 64K depending
	// on the length of the kernel image.
	addi		r5,r5,4;
	mr		r14,r5;
	addis		sp,r5,0x0001;
	rlwinm	sp,sp,0,0,19;
	mr		r7,sp;
	li		r3,0;
	stwu		r3,-64(sp);	// This is a fake frame, with a zero back chain.  It is at end of the current page.
	//stwu		sp,-300(sp);	// This is a usable frame.
	lis		r3,'Pa';
	ori		r3,r3,'nd';
	eieio;
	stw		r3,4(sp);
	
	b		@startKernel;
	
@flushKernel:
	// Flush all kernel memory from the processor
	// Move the kernel text to address BASE_ADDRESS
	lwz		r4,BootInfo.pefLen(r27);
	srwi(r4,r4,2);
	mtctr	r4;
	lis		r5,UPPER_HW(BASE_ADDRESS-4);
	ori		r5,r5,LOWER_HW(BASE_ADDRESS-4);
	
	// Set up the MSR - leave only relocation and ROM interrupt prefix on, all other interrupts and things go off!  We leave the
	// ROM interrupt prefix on so that MacOS will continue to handle all 603 memory management interrupts until we're ready
	// to do so!
	//
	// *** Another possibility: Turn off data relocation but leave instruction relocation on.  Then we need to physical address
	// of the kernel passed in here, right now we have the logical address.
	li		r6,0x1070;
	sync;
	mtmsr	r6;
	sync;
	
	@flushLoop:
		addi		r5,r5,4;
		dcbf		r0,r5;
		sync;
		icbi		r0,r5;
		sync;
		bdnz		@flushLoop;
	
	// Find the stackbase
	addi		r5,r5,4;
	mr		r14,r5;
	addis		sp,r5,0x0001;
	rlwinm	sp,sp,0,0,19;
	subi		sp,sp,64;
	lis		r3,'Pa';
	ori		r3,r3,'nd';
@waitForKernel:
	lwz		r4,4(sp);
	cmpw	r3,r4;
	bne		@waitForKernel;
	
	b	@startKernel;
	
@startKernel:
	// Move the arguments to the right registers
	mr		r3,r31;							// kernelBootInfo
	mr		r4,r30;							// processorID
	
	// Start up the kernel - find the main transition vector, load it's rtoc and branch to it!
	// Note: This is loading the RTOC and main() address from the moved Kernel PEF image, which is
	// guaranteed to be mapped correctly at this point.
	lwz		rtoc,0(r29);
	mtctr	rtoc;
	lwz		rtoc,4(r29);
	bctr;	// Make the kernel RUN!
entry	static EndMoveKernelToZero
}

void PreparePEFFragment(const unsigned char* _pefName,PEFLoaderInfoHeader** _loaderSection,char** _codeSection,char** _dataSection,unsigned long _endPadding,char** _pefBaseAddr,unsigned long* _pefLen)
{
	short	kernelRefNum;
	short	vRefNum;
	long		dirID;
	
	HGetVol(nil,&vRefNum,&dirID);
	if(HOpenDF(vRefNum,dirID,_pefName,fsRdPerm,&kernelRefNum))
		UnrecoverableError("\pFailed to open the PEF file - is it where you expect it to be?");
	
	long				readLen;
	PEFContainerHeader	pefContainerHeader;
	readLen = sizeof(pefContainerHeader);
	FSRead(kernelRefNum,&readLen,&pefContainerHeader);
	if(pefContainerHeader.tag1 != 'Joy!' || pefContainerHeader.tag2 != 'peff' || pefContainerHeader.architecture != 'pwpc' || pefContainerHeader.formatVersion > 1 || pefContainerHeader.reservedA != 0)
		UnrecoverableError("\pThis was not a valid PEF fragment!");
	
	// Figure out the total length of the memory image
	PEFSectionHeader	codeHeader;
	PEFSectionHeader	dataHeader;
	PEFSectionHeader	loaderHeader;
	long	instLen = 0;
	
	// We load code sections first, so count how that lines up
	instLen = ROUND_UP(4096,instLen);	// Start on a page boundary
	if(SetFPos(kernelRefNum,fsFromStart,sizeof(PEFContainerHeader) + CODE_SECTION*sizeof(PEFSectionHeader)))
		UnrecoverableError("\pFailed SetFPos for codeHeader!");
	readLen = sizeof(codeHeader);
	if(FSRead(kernelRefNum,&readLen,&codeHeader))
		UnrecoverableError("\pFailed FSRead for codeHeader!");
	instLen = ROUND_UP_POW2(instLen,codeHeader.alignment);
	instLen += codeHeader.containerLength;
	
	// Data sections second
	instLen = ROUND_UP(4096,instLen);	// Start on a page boundary
	if(SetFPos(kernelRefNum,fsFromStart,sizeof(PEFContainerHeader) + DATA_SECTION*sizeof(PEFSectionHeader)))
		UnrecoverableError("\pFailed SetFPos for dataHeader!");
	readLen = sizeof(dataHeader);
	if(FSRead(kernelRefNum,&readLen,&dataHeader))
		UnrecoverableError("\pFailed FSRead for dataHeader!");
	if(dataHeader.sectionKind == 2)
		UnrecoverableError("\pThe PEF loader does not support pattern-initialized data sections!");
	instLen = ROUND_UP_POW2(instLen,dataHeader.alignment);
	instLen += dataHeader.containerLength;
	
	// And the loader header
	if(SetFPos(kernelRefNum,fsFromStart,sizeof(PEFContainerHeader) + LOADER_SECTION*sizeof(PEFSectionHeader)))
		UnrecoverableError("\pFailed SetFPos for loaderHeader!");
	readLen = sizeof(loaderHeader);
	if(FSRead(kernelRefNum,&readLen,&loaderHeader))
		UnrecoverableError("\pFailed FSRead for loaderHeader!");
	instLen = ROUND_UP_POW2(instLen,loaderHeader.alignment);
	instLen += loaderHeader.containerLength;
	
	// Now our end padding for other peoples' use.
	instLen = ROUND_UP_POW2(instLen,2);
	instLen += _endPadding;
	
	// Read the sections into memory.
	char*				instData = NewPtrClear(instLen + 4095);
	char*				instDataPageStart = (char*)(((long)instData + 4095) & 0xFFFFF000);
	char*				currPtr = instDataPageStart;
	char*				codeSection;
	char*				dataSection;
	PEFLoaderInfoHeader*	loaderSection;
	
	if(!instData)
		UnrecoverableError("\pFailed to allocate memory for PEF image!");
	if(LockMemoryContiguous(instDataPageStart,instLen))
		UnrecoverableError("\pFailed to lock enough contiguous memory for the PEF image!");
	unsigned long			endPaddingPhys = (unsigned long)GetPhysicalAddress((void*)instDataPageStart) + instLen - _endPadding;
	if(	(endPaddingPhys > BASE_ADDRESS && endPaddingPhys < BASE_ADDRESS + KERNEL_MAXLEN) ||
		(endPaddingPhys + _endPadding > BASE_ADDRESS && endPaddingPhys + _endPadding < BASE_ADDRESS + KERNEL_MAXLEN) )
		UnrecoverableError("\pKernel moving code would overwrite itself!");
	
	// Read in the code section first
	currPtr = (char*)ROUND_UP(4096,(long)currPtr);	// Start code on a page boundary
	currPtr = (char*)ROUND_UP_POW2(currPtr,codeHeader.alignment);
	if(SetFPos(kernelRefNum,fsFromStart,codeHeader.containerOffset))
		UnrecoverableError("\pFailed SetFPos for codeSection!");
	readLen = codeHeader.containerLength;
	if(FSRead(kernelRefNum,&readLen,currPtr))
		UnrecoverableError("\pFailed FSRead for codeSection!");
	codeSection = currPtr;
	currPtr += readLen;
	for(long j=0;j<codeHeader.totalLength - codeHeader.unpackedLength;j++)
		*currPtr++ = 0;
	
	// Read in the data section second
	currPtr = (char*)ROUND_UP(4096,(long)currPtr);	// Start data on a page boundary
	currPtr = (char*)ROUND_UP_POW2((long)currPtr,dataHeader.alignment);
	if(SetFPos(kernelRefNum,fsFromStart,dataHeader.containerOffset))
		UnrecoverableError("\pFailed SetFPos for dataSection!");
	readLen = dataHeader.containerLength;
	if(FSRead(kernelRefNum,&readLen,currPtr))
		UnrecoverableError("\pFailed FSRead for dataSection!");
	dataSection = currPtr;
	currPtr += readLen;
	for(long j=0;j<dataHeader.totalLength - dataHeader.unpackedLength;j++)
		*currPtr++ = 0;
	
	// And the loader section
	currPtr = (char*)ROUND_UP_POW2((long)currPtr,loaderHeader.alignment);
	if(SetFPos(kernelRefNum,fsFromStart,loaderHeader.containerOffset))
		UnrecoverableError("\pFailed SetFPos for loaderSection!");
	readLen = loaderHeader.containerLength;
	if(FSRead(kernelRefNum,&readLen,currPtr))
		UnrecoverableError("\pFailed FSRead for loaderSection!");
	loaderSection = (PEFLoaderInfoHeader*)currPtr;
	currPtr += readLen;
	for(long j=0;j<loaderHeader.totalLength - loaderHeader.unpackedLength;j++)
		*currPtr++ = 0;
	
	if(FSClose(kernelRefNum))
		Message("\pError closing PEF file.");
	
	Relocate(codeSection,dataSection,(char*)loaderSection);
	
	MakeDataExecutable(instDataPageStart,instLen);
	
	*_loaderSection = loaderSection;
	*_codeSection = codeSection;
	*_dataSection = dataSection;
	*_pefBaseAddr = instDataPageStart;
	*_pefLen = instLen;
}

void Relocate(char* codeSection,char* dataSection,char* loaderSection)
{
	// Perform PEF relocations on the kernel.
	char*					sections[3];
	PEFLoaderInfoHeader*		loaderHeader = (PEFLoaderInfoHeader*)loaderSection;
	PEFLoaderRelocationHeader*	relocationHeaders = (PEFLoaderRelocationHeader*)((long)loaderSection + sizeof(PEFLoaderInfoHeader));
	unsigned char*				relocations = (unsigned char*)loaderHeader + loaderHeader->relocInstrOffset;
	
	sections[CODE_SECTION] = codeSection;
	sections[DATA_SECTION] = dataSection;
	sections[LOADER_SECTION] = loaderSection;
	
	for(long i=0;i<loaderHeader->relocSectionCount;i++)
	{
		unsigned char*	currInst = relocations + relocationHeaders[i].firstRelocOffset;
		char*		relocAddress = sections[relocationHeaders[i].sectionIndex];
		char*		sectionC = (char*)BASE_ADDRESS;/*codeSection;*/
		char*		sectionD = (char*)(sections[DATA_SECTION] - sections[CODE_SECTION] + BASE_ADDRESS);/*dataSection;*/
		long			instCount = relocationHeaders[i].relocCount*2;
		
		while(instCount)
		{
			if((currInst[0] & 0xF0) == 0x90)	// RelocSmRepeat
			{
				RelocSmRepeatInst*	theInst = (RelocSmRepeatInst*)currInst;
				for(long j=0;j<=theInst->repeatCountMinusOne;j++)
				{
					unsigned long repeatBlockCount = (theInst->repeatCountMinusOne + 1)*2;
					unsigned char*	repeatCurrInst = currInst - repeatBlockCount;
					while(repeatBlockCount)
					{
						unsigned char count = ProcessOpcode(repeatCurrInst,relocAddress,sectionC,sectionD,sections);
						repeatBlockCount -= count;
						repeatCurrInst += count;
					}
				}
				instCount -= 2;
				currInst += 2;
			}
			else if((currInst[0] & 0xFC) == 0xA0)	// RelocSetPosition
			{
				RelocSetPositionInst*	theInst = (RelocSetPositionInst*)currInst;
				relocAddress = sections[relocationHeaders[i].sectionIndex] + theInst->offset;
				instCount -= 4;
				currInst += 4;
			}
			else if((currInst[0] & 0xFC) == 0xB0)	// RelocLgRepeat
			{
				RelocLgRepeatInst*	theInst = (RelocLgRepeatInst*)currInst;
				for(long j=0;j<theInst->repeatCount;j++)
				{
					unsigned char	repeatBlockCount =(theInst->blockCountMinusOne + 1)*2;
					unsigned char*	repeatCurrInst = currInst - repeatBlockCount;
					while(repeatBlockCount)
					{
						unsigned char count = ProcessOpcode(repeatCurrInst,relocAddress,sectionC,sectionD,sections);
						repeatBlockCount -= count;
						repeatCurrInst += count;
					}
				}
				instCount -= 4;
				currInst += 4;
			}
			else
			{
				unsigned char count = ProcessOpcode(currInst,relocAddress,sectionC,sectionD,sections);
				instCount -= count;
				currInst += count;
			}
		}
	}
}

unsigned char ProcessOpcode(unsigned char* currInst,char*& relocAddress,char*& sectionC,char*& sectionD,char** sections)
{
	// Perform some more PEF relocations on the kernel.
	if((currInst[0] & 0xC0) == 0)	// RelocBySectDWithSkip
	{
		RelocBySectDWithSkipInst*	theInst = (RelocBySectDWithSkipInst*)currInst;
		relocAddress += theInst->skipCount*4;
		for(long j=0;j<theInst->relocCount;j++)
			*((unsigned long*)relocAddress)++ += (unsigned long)sectionD;
		return 2;
	}
	else if((currInst[0] & 0xE0) == 0x40)	// RelocRun
	{
		RelocRunInst*	theInst = (RelocRunInst*)currInst;
		for(long j=0;j<=theInst->runLengthMinusOne;j++)
		{
			switch(theInst->subOpcode)
			{
				case 0:	// RelocBySectC
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionC;
				break;
				case 1:	// RelocBySectD
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionD;
				break;
				case 2:	// RelocTVector12
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionC;
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionD;
					((unsigned long*)relocAddress)++;
				break;
				case 3:	// RelocTVector8
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionC;
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionD;
				break;
				case 4:	// RelocVTable8
					*((unsigned long*)relocAddress)++ += (unsigned long)sectionD;
					((unsigned long*)relocAddress)++;
				break;
				case 5:	// RelocImportRun;
					DebugStr("\pRelocImportRun opcode in kernel: Kernel never imports symbols!");
				break;
			}
		}
		return 2;
	}
	else if((currInst[0] & 0xE0) == 0x60)	// RelocSmIndex
	{
		RelocSmIndexInst*	theInst = (RelocSmIndexInst*)currInst;
		switch(theInst->subOpcode)
		{
			case 0:	// RelocSmByImport
				DebugStr("\pRelocSmByImport opcode in kernel: Kernel never imports symbols!");
			break;
			case 1:	// RelocSmSetSectC
				sectionC = (char*)(sections[theInst->index] - sections[CODE_SECTION] + BASE_ADDRESS);
			break;
			case 2:	// RelocSmSetSectD
				sectionD = (char*)(sections[theInst->index] - sections[CODE_SECTION] + BASE_ADDRESS);
			break;
			case 3:	// RelocSmBySection
				*((unsigned long*)relocAddress)++ += (unsigned long)sections[theInst->index] - (unsigned long)sections[CODE_SECTION] + BASE_ADDRESS;
			break;
		}
		return 2;
	}
	else if((currInst[0] & 0xF0) == 0x80)	// RelocIncrPosition
	{
		RelocIncrPositionInst*	theInst = (RelocIncrPositionInst*)currInst;
		relocAddress += theInst->offsetMinusOne + 1;
		return 2;
	}
	else if((currInst[0] & 0xFC) == 0xA4)	// RelocLgByImport
	{
		DebugStr("\pRelocLgByImport opcode in kernel: Kernel never imports symbols!");
		return 4;
	}
	else if((currInst[0] & 0xFC) == 0xB4)	// RelogLgSetOrBySection
	{
		RelocLgSetOrBySectionInst*	theInst = (RelocLgSetOrBySectionInst*)currInst;
		switch(theInst->subOpcode)
		{
			case 0:	*((unsigned long*)relocAddress)++ += (unsigned long)sections[theInst->index] - (unsigned long)sections[CODE_SECTION];	break;
			case 1:	sectionC = (char*)(sections[theInst->index] - sections[CODE_SECTION] + BASE_ADDRESS);						break;
			case 2:	sectionD = (char*)(sections[theInst->index] - sections[CODE_SECTION] + BASE_ADDRESS);						break;
		}
		return 4;
	}
	else	// Unknown opcode
		UnrecoverableError("\pPEF loader encountered an unknown opcode!");
	return 0;
}

void Message(Str255 err)
{
	Rect theRect = {138,11,179,526};
	EraseRect(&theRect);
	MoveTo(15,147);
	DrawString(err);
}

void WaitMessage(Str255 err)
{
	short itemHit;
	GrafPtr currPort;
	
	Message(err);
	GetPort(&currPort);
	SizeWindow(currPort,541,224,true);	// Show the OK button;
	
	ModalDialog(nil,&itemHit);
	
	SizeWindow(currPort,541,193,false);
}

void UnrecoverableError(Str255 err)
{
	Str255 theStr = "\pA fatal error occurred: ";
	Str255 catStr;
	
	for(unsigned long i=1;i<=theStr[0];i++)
		catStr[i] = theStr[i];
	for(unsigned long i=theStr[0]+1;i<=theStr[0]+err[0];i++)
		catStr[i] = err[i-theStr[0]];
	catStr[0] = theStr[0] + err[0];
	
	WaitMessage(catStr);
	ExitToShell();
}

pascal void ShutDownProc(short)
{
	// Hide the cursor
	HideCursor();
	
	// A nice little message
	Message("\pBooting Prometheus");
	
	// Enter supervisor mode
	PPCEnterSupervisorMode();
	
	// Copy the kernel to address BASE_ADDRESS and then branch to it's main() routine.
	CallMoveKernelToZero(kernelBootInfo,&bootInfo,(char*)blockMoveZeroCode,bootInfo.numProcessors);
}

void* GetPhysicalAddress(void* logicalAddr)
{
	LogicalToPhysicalTable	addresses;
	unsigned long			physicalEntryCount = 1;
	
	addresses.logical.address = logicalAddr;
	addresses.logical.count = 4;
	if(GetPhysical(&addresses,&physicalEntryCount))
		return (void*)0xDEADBEEF;
	if(!physicalEntryCount)
		return (void*)0xDEADBEEF;
	
	return addresses.physical[0].address;
}

static const char*			ignoredOpenFirmwareNodes[] =	{ "chosen", "openprom","AAPL,ROM","options","aliases","packages" };
static unsigned long			openFirmwareNodeCount = 1;
static unsigned long			busFrequency;
static void IterateChildren(RegEntryID* currNode,OpenFirmwareNode* parent);
static void FillInNuBusTrees( void );

_OpenFirmwareNode			openFirmware6100Tree[24], openFirmware8100Tree[27];
/*_OpenFirmwareNode	openFirmware6100Tree[] =
									{
										{	// 0 - Top node of the tree
											emptyNodeType,
											-1,
											-1,-1,
											1,
											"device-tree"
										},
										{	// 1 - CPU node
											cpuNodeType,
											0,
											-1,3,
											2,
											"PowerPC,601",
											{
												// _OpenFirmwareCPUNode
												0x00010000,		// 601 PVR
												-1,				// Processor speed filled in below
												-1,				// Bus speed filled in below
												32*1024,			// 32 K cache (unified)
												32*1024			// 32 K cache (unified)
											}
										},
										{	// 2 - 256K L2 cache node
											cacheNodeType,
											1,
											-1,-1,
											-1,
											"l2-cache",
											{},{},
											{
												// _OpenFirmwareCacheNode
												256*1024,		// 256K instruction cache
												256*1024,		// 256K data cache
												8192,			// 8192 instruction cache sets
												8192,			// 8192 data cache sets
												true				// unified
											}
										},
										{	// 3 - memory node
											memoryNodeType,
											0,
											1,4,
											-1,
											"memory",
											{},
											{
												// _OpenFirmwareMemoryNode
												0,				// 0x00000000 base address
												0				// 0 length (for now)
											}
										},
										{	// 4 - AMIC, empty node
											emptyNodeType,
											0,
											3,15,
											5,
											"amic"
										},
										{	// 5 - AWACS sound chip
											deviceNodeType,
											4,
											-1,6,
											-1,
											"awacs",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F14000,-1,-1,-1},		// Do something about DMA addresses?
												{17,8,9,-1}
											}
										},
										{	// 6 - CUDA VIA chip
											deviceNodeType,
											4,
											5,13,
											7,
											"via-cuda",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F00000,-1,-1,-1},
												{18,-1,-1,-1}
											}
										},
										{	// 7 - adb node
											emptyNodeType,
											6,
											-1,10,
											8,
											"adb"
										},
										{	// 8 - keyboard node
											emptyNodeType,
											7,
											-1,9,
											-1,
											"keyboard"
										},
										{	// 9 - mouse node
											emptyNodeType,
											7,
											8,-1,
											-1,
											"mouse"
										},
										{	// 10 - PRAM node
											emptyNodeType,
											6,
											7,11,
											-1,
											"pram"
										},
										{	// 11 - rtc node
											emptyNodeType,
											6,
											10,12,
											-1,
											"rtc"
										},
										{	// 12 - power-mgt node
											emptyNodeType,
											6,
											11,-1,
											-1,
											"power-mgt"
										},
										{	// 13 - via2 chip
											deviceNodeType,
											4,
											6,14,
											-1,
											"via2",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F26000,-1,-1,-1},
												{-1,-1,-1,-1}
											}
										},
										{	// 14 - SWIM3 node
											deviceNodeType,
											4,
											13,-1,
											-1,
											"swim3",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{-1,-1,-1,-1},	// Don't know SWIM3 address on PDM
												{19,1,-1,-1},
											}
										},
										{	// 15 - curio node
											emptyNodeType,
											0,
											4,23,
											16,
											"curio"
										},
										{	// 16 - SCC node
											emptyNodeType,
											15,
											-1,19,
											17,
											"escc"
										},
										{	// 17 - SCC channel A
											deviceNodeType,
											16,
											-1,18,
											-1,
											"ch-a",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F04020,-1,-1,-1},	// Do something about DMA addresses
												{15,4,5,-1}
											}
										},
										{	// 18 - SCC channel B
											deviceNodeType,
											16,
											17,-1,
											-1,
											"ch-b",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F00000,-1,-1,-1},	// Do something about DMA addresses
												{16,6,7,-1}
											}
										},
										{	// 19 - MACE ethernet
											deviceNodeType,
											15,
											16,20,
											-1,
											"mace",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F0A000,0x50F31C20,-1,-1},
												{14,2,3,-1}
											}
										},
										{	// 20 - 53C94 chip
											deviceNodeType,
											14,
											19,-1,
											21,
											"53c94",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F10000,-1,-1,-1},	// Do something about DMA addresses
												{12,0,-1,-1}
											}
										},
										{	// 21 - sd for 53c94
											emptyNodeType,
											20,
											-1,22,
											-1,
											"sd"
										},
										{	// 22 - st for 53c94
											emptyNodeType,
											20,
											21,-1,
											-1,
											"st"
										},
										{	// 23 - BART Nubus controller - don't know anything about this, except that it exists!
											emptyNodeType,
											0,
											15,-1,
											-1,
											"bart"
										}
									};
_OpenFirmwareNode	openFirmware8100Tree[] =	// Same as 6100/7100 except for 53cf94 at end
									{	
										{	// 0 - Top node of the tree
											emptyNodeType,
											-1,
											-1,-1,
											1,
											"device-tree"
										},
										{	// 1 - CPU node
											cpuNodeType,
											0,
											-1,3,
											2,
											"PowerPC,601",
											{
												// _OpenFirmwareCPUNode
												0x00010000,		// 601 PVR
												-1,				// Processor speed filled in below
												-1,				// Bus speed filled in below
												32*1024,			// 32 K cache (unified)
												32*1024			// 32 K cache (unified)
											}
										},
										{	// 2 - 256K L2 cache node
											cacheNodeType,
											1,
											-1,-1,
											-1,
											"l2-cache",
											{},{},
											{
												// _OpenFirmwareCacheNode
												256*1024,		// 256K instruction cache
												256*1024,		// 256K data cache
												8192,			// 8192 instruction cache sets
												8192,			// 8192 data cache sets
												true				// unified
											}
										},
										{	// 3 - memory node
											memoryNodeType,
											0,
											1,4,
											-1,
											"memory",
											{},
											{
												// _OpenFirmwareMemoryNode
												0,				// 0x00000000 base address
												0				// 0 length (for now)
											}
										},
										{	// 4 - AMIC, empty node
											emptyNodeType,
											0,
											3,15,
											5,
											"amic"
										},
										{	// 5 - AWACS sound chip
											deviceNodeType,
											4,
											-1,6,
											-1,
											"awacs",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F14000,-1,-1,-1},		// Do something about DMA addresses?
												{17,8,9,-1}
											}
										},
										{	// 6 - CUDA VIA chip
											deviceNodeType,
											4,
											5,13,
											7,
											"via-cuda",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F00000,-1,-1,-1},
												{18,-1,-1,-1}
											}
										},
										{	// 7 - adb node
											emptyNodeType,
											6,
											-1,10,
											8,
											"adb"
										},
										{	// 8 - keyboard node
											emptyNodeType,
											7,
											-1,9,
											-1,
											"keyboard"
										},
										{	// 9 - mouse node
											emptyNodeType,
											7,
											8,-1,
											-1,
											"mouse"
										},
										{	// 10 - PRAM node
											emptyNodeType,
											6,
											7,11,
											-1,
											"pram"
										},
										{	// 11 - rtc node
											emptyNodeType,
											6,
											10,12,
											-1,
											"rtc"
										},
										{	// 12 - power-mgt node
											emptyNodeType,
											6,
											11,-1,
											-1,
											"power-mgt"
										},
										{	// 13 - via2 chip
											deviceNodeType,
											4,
											6,14,
											-1,
											"via2",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F26000,-1,-1,-1},
												{-1,-1,-1,-1}
											}
										},
										{	// 14 - SWIM3 node
											deviceNodeType,
											4,
											13,-1,
											-1,
											"swim3",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{-1,-1,-1,-1},	// Don't know SWIM3 address on PDM
												{19,1,-1,-1},
											}
										},
										{	// 15 - curio node
											emptyNodeType,
											0,
											4,23,
											16,
											"curio"
										},
										{	// 16 - SCC node
											emptyNodeType,
											15,
											-1,19,
											17,
											"escc"
										},
										{	// 17 - SCC channel A
											deviceNodeType,
											16,
											-1,18,
											-1,
											"ch-a",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F04020,-1,-1,-1},	// Do something about DMA addresses
												{15,4,5,-1}
											}
										},
										{	// 18 - SCC channel B
											deviceNodeType,
											16,
											17,-1,
											-1,
											"ch-b",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F00000,-1,-1,-1},	// Do something about DMA addresses
												{16,6,7,-1}
											}
										},
										{	// 19 - MACE ethernet
											deviceNodeType,
											15,
											16,20,
											-1,
											"mace",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F0A000,0x50F31C20,-1,-1},
												{14,2,3,-1}
											}
										},
										{	// 20 - 53C94 chip
											deviceNodeType,
											14,
											19,-1,
											21,
											"53c94",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F10000,-1,-1,-1},	// Do something about DMA addresses
												{12,0,-1,-1}
											}
										},
										{	// 21 - sd for 53c94
											emptyNodeType,
											20,
											-1,22,
											-1,
											"sd"
										},
										{	// 22 - st for 53c94
											emptyNodeType,
											20,
											21,-1,
											-1,
											"st"
										},
										{	// 23 - BART Nubus controller - don't know anything about this, except that it exists!
											emptyNodeType,
											0,
											15,24,
											-1,
											"bart"
										},
										{	// 24 - 53cf94 fast SCSI controller
											deviceNodeType,
											0,
											23,-1,
											25,
											"53cf94",
											{},{},{},
											{
												// _OpenFirmwareDeviceNode
												{0x50F11000,-1,-1,-1},	// Do something about DMA addresses
												{13,10,-1,-1}
											}
										},
										{	// 25 - sd for 53cf94
											emptyNodeType,
											24,
											-1,26,
											-1,
											"sd"
										},
										{	// 26 - st for 53cf94
											emptyNodeType,
											24,
											25,-1,
											-1,
											"st"
										}
									};*/

void FillInNuBusTrees( void )
{
	// 0 - Top node of the tree
	openFirmware6100Tree[0]._nodeType = emptyNodeType;
	openFirmware6100Tree[0]._parentNode = -1;
	openFirmware6100Tree[0]._prevSibling = -1;
	openFirmware6100Tree[0]._nextSibling = -1;
	openFirmware6100Tree[0]._childNode = 1;
	strcpy( openFirmware6100Tree[0]._name, "device-tree" );
	openFirmware6100Tree[0]._devType[0] = 0;
	
	// 1 - CPU node
	openFirmware6100Tree[1]._nodeType = cpuNodeType;
	openFirmware6100Tree[1]._parentNode = 0;
	openFirmware6100Tree[1]._prevSibling = -1;
	openFirmware6100Tree[1]._nextSibling = 3;
	openFirmware6100Tree[1]._childNode = 2;
	strcpy( openFirmware6100Tree[1]._name, "PowerPC,601" );
	strcpy( openFirmware6100Tree[1]._devType, "cpu" );
	openFirmware6100Tree[1].cpuNode._cpuVersion = 0x00010000;			// 601 PVR
	openFirmware6100Tree[1].cpuNode._clockFrequency = -1;				// Processor speed filled in below
	openFirmware6100Tree[1].cpuNode._busFrequency = -1;					// Bus speed filled in below
	openFirmware6100Tree[1].cpuNode._dCacheSize = 32*1024;				// 32 K cache (unified)
	openFirmware6100Tree[1].cpuNode._iCacheSize = 32*1024;				// 32 K cache (unified)

	// 2 - 256K L2 cache node
	openFirmware6100Tree[2]._nodeType = cacheNodeType;
	openFirmware6100Tree[2]._parentNode = 1;
	openFirmware6100Tree[2]._prevSibling = -1;
	openFirmware6100Tree[2]._nextSibling = -1;
	openFirmware6100Tree[2]._childNode = -1;
	strcpy( openFirmware6100Tree[2]._name, "l2-cache" );
	strcpy( openFirmware6100Tree[2]._devType, "cache" );
	openFirmware6100Tree[2].cacheNode._iCacheSize = 256*1024;		// 256K instruction cache
	openFirmware6100Tree[2].cacheNode._dCacheSize = 256*1024;		// 256K data cache
	openFirmware6100Tree[2].cacheNode._iCacheSets = 8192;			// 8192 instruction cache sets
	openFirmware6100Tree[2].cacheNode._dCacheSets = 8192;			// 8192 data cache sets
	openFirmware6100Tree[2].cacheNode._unified = true;				// unified
	
	// 3 - memory node
	openFirmware6100Tree[3]._nodeType = memoryNodeType;
	openFirmware6100Tree[3]._parentNode = 0;
	openFirmware6100Tree[3]._prevSibling = 1;
	openFirmware6100Tree[3]._nextSibling = 4;
	openFirmware6100Tree[3]._childNode = -1;
	strcpy( openFirmware6100Tree[3]._name, "memory" );
	strcpy( openFirmware6100Tree[3]._devType, "memory" );
	openFirmware6100Tree[3].memoryNode._baseAddr = 0;				// 0x00000000 base address
	openFirmware6100Tree[3].memoryNode._size = 0;					// 0 length (for now)
	
	// 4 - AMIC, empty node
	openFirmware6100Tree[4]._nodeType = emptyNodeType;
	openFirmware6100Tree[4]._parentNode = 0;
	openFirmware6100Tree[4]._prevSibling = 3;
	openFirmware6100Tree[4]._nextSibling = 15;
	openFirmware6100Tree[4]._childNode = 5;
	strcpy( openFirmware6100Tree[4]._name, "amic" );
	openFirmware6100Tree[4]._devType[0] = 0;
	
	// 5 - AWACS sound chip
	openFirmware6100Tree[5]._nodeType = deviceNodeType;
	openFirmware6100Tree[5]._parentNode = 4;
	openFirmware6100Tree[5]._prevSibling = -1;
	openFirmware6100Tree[5]._nextSibling = 6;
	openFirmware6100Tree[5]._childNode = -1;
	strcpy( openFirmware6100Tree[5]._name, "awacs" );
	openFirmware6100Tree[5]._devType[0] = 0;
	openFirmware6100Tree[5].deviceNode._address[0] = 0x50F14000;			// Do something about DMA addresses
	openFirmware6100Tree[5].deviceNode._address[1] = -1;
	openFirmware6100Tree[5].deviceNode._address[2] = -1;
	openFirmware6100Tree[5].deviceNode._address[3] = -1;
	openFirmware6100Tree[5].deviceNode._interrupts[0] = 17;
	openFirmware6100Tree[5].deviceNode._interrupts[1] = 8;
	openFirmware6100Tree[5].deviceNode._interrupts[2] = 9;
	openFirmware6100Tree[5].deviceNode._interrupts[3] = -1;
	
	// 6 - CUDA VIA chip
	openFirmware6100Tree[6]._nodeType = deviceNodeType;
	openFirmware6100Tree[6]._parentNode = 4;
	openFirmware6100Tree[6]._prevSibling = 5;
	openFirmware6100Tree[6]._nextSibling = 13;
	openFirmware6100Tree[6]._childNode = 7;
	strcpy( openFirmware6100Tree[6]._name, "via-cuda" );
	strcpy( openFirmware6100Tree[6]._devType, "via-cuda" );
	openFirmware6100Tree[6].deviceNode._address[0] = 0x50F00000;
	openFirmware6100Tree[6].deviceNode._address[1] = -1;
	openFirmware6100Tree[6].deviceNode._address[2] = -1;
	openFirmware6100Tree[6].deviceNode._address[3] = -1;
	openFirmware6100Tree[6].deviceNode._interrupts[0] = 18;
	openFirmware6100Tree[6].deviceNode._interrupts[1] = -1;
	openFirmware6100Tree[6].deviceNode._interrupts[2] = -1;
	openFirmware6100Tree[6].deviceNode._interrupts[3] = -1;
	
	// 7 - adb node
	openFirmware6100Tree[7]._nodeType = emptyNodeType;
	openFirmware6100Tree[7]._parentNode = 6;
	openFirmware6100Tree[7]._prevSibling = -1;
	openFirmware6100Tree[7]._nextSibling = 10;
	openFirmware6100Tree[7]._childNode = 8;
	strcpy( openFirmware6100Tree[7]._name, "adb" );
	strcpy( openFirmware6100Tree[7]._devType, "adb" );
	
	// 8 - keyboard node
	openFirmware6100Tree[8]._nodeType = emptyNodeType;
	openFirmware6100Tree[8]._parentNode = 7;
	openFirmware6100Tree[8]._prevSibling = -1;
	openFirmware6100Tree[8]._nextSibling = 9;
	openFirmware6100Tree[8]._childNode = -1;
	strcpy( openFirmware6100Tree[8]._name, "keyboard" );
	strcpy( openFirmware6100Tree[8]._devType, "keyboard" );
	
	// 9 - mouse node
	openFirmware6100Tree[9]._nodeType = emptyNodeType;
	openFirmware6100Tree[9]._parentNode = 7;
	openFirmware6100Tree[9]._prevSibling = 8;
	openFirmware6100Tree[9]._nextSibling = -1;
	openFirmware6100Tree[9]._childNode = -1;
	strcpy( openFirmware6100Tree[9]._name, "mouse" );
	strcpy( openFirmware6100Tree[9]._devType, "mouse" );
	
	// 10 - PRAM node
	openFirmware6100Tree[10]._nodeType = emptyNodeType;
	openFirmware6100Tree[10]._parentNode = 6;
	openFirmware6100Tree[10]._prevSibling = 7;
	openFirmware6100Tree[10]._nextSibling = 11;
	openFirmware6100Tree[10]._childNode = -1;
	strcpy( openFirmware6100Tree[10]._name, "pram" );
	openFirmware6100Tree[10]._devType[0] = 0;
	
	// 11 - rtc node
	openFirmware6100Tree[11]._nodeType = emptyNodeType;
	openFirmware6100Tree[11]._parentNode = 6;
	openFirmware6100Tree[11]._prevSibling = 10;
	openFirmware6100Tree[11]._nextSibling = 12;
	openFirmware6100Tree[11]._childNode = -1;
	strcpy( openFirmware6100Tree[11]._name, "rtc" );
	strcpy( openFirmware6100Tree[11]._devType, "rtc" );
	
	// 11 - power-mgt node
	openFirmware6100Tree[12]._nodeType = emptyNodeType;
	openFirmware6100Tree[12]._parentNode = 6;
	openFirmware6100Tree[12]._prevSibling = 11;
	openFirmware6100Tree[12]._nextSibling = -1;
	openFirmware6100Tree[12]._childNode = -1;
	strcpy( openFirmware6100Tree[12]._name, "power-mgt" );
	strcpy( openFirmware6100Tree[12]._devType, "power-mgt" );
	
	// 13 - via2 chip
	openFirmware6100Tree[13]._nodeType = deviceNodeType;
	openFirmware6100Tree[13]._parentNode = 4;
	openFirmware6100Tree[13]._prevSibling = 6;
	openFirmware6100Tree[13]._nextSibling = 14;
	openFirmware6100Tree[13]._childNode = -1;
	strcpy( openFirmware6100Tree[13]._name, "via2" );
	openFirmware6100Tree[13]._devType[0] = 0;
	openFirmware6100Tree[13].deviceNode._address[0] = 0x50F26000;
	openFirmware6100Tree[13].deviceNode._address[1] = -1;
	openFirmware6100Tree[13].deviceNode._address[2] = -1;
	openFirmware6100Tree[13].deviceNode._address[3] = -1;
	openFirmware6100Tree[13].deviceNode._interrupts[0] = -1;
	openFirmware6100Tree[13].deviceNode._interrupts[1] = -1;
	openFirmware6100Tree[13].deviceNode._interrupts[2] = -1;
	openFirmware6100Tree[13].deviceNode._interrupts[3] = -1;
	
	// 14 - SWIM3 node
	openFirmware6100Tree[14]._nodeType = deviceNodeType;
	openFirmware6100Tree[14]._parentNode = 4;
	openFirmware6100Tree[14]._prevSibling = 13;
	openFirmware6100Tree[14]._nextSibling = -1;
	openFirmware6100Tree[14]._childNode = -1;
	strcpy( openFirmware6100Tree[14]._name, "swim3" );
	openFirmware6100Tree[0]._devType[14] = 0;
	openFirmware6100Tree[14].deviceNode._address[0] = -1;			// Don't know SWIM3 address on PDM
	openFirmware6100Tree[14].deviceNode._address[1] = -1;
	openFirmware6100Tree[14].deviceNode._address[2] = -1;
	openFirmware6100Tree[14].deviceNode._address[3] = -1;
	openFirmware6100Tree[14].deviceNode._interrupts[0] = 19;
	openFirmware6100Tree[14].deviceNode._interrupts[1] = -1;
	openFirmware6100Tree[14].deviceNode._interrupts[2] = -1;
	openFirmware6100Tree[14].deviceNode._interrupts[3] = -1;
	
	// 15 - curio node
	openFirmware6100Tree[15]._nodeType = emptyNodeType;
	openFirmware6100Tree[15]._parentNode = 0;
	openFirmware6100Tree[15]._prevSibling = 4;
	openFirmware6100Tree[15]._nextSibling = 23;
	openFirmware6100Tree[15]._childNode = 16;
	strcpy( openFirmware6100Tree[15]._name, "curio" );
	openFirmware6100Tree[15]._devType[0] = 0;
	
	// 16 - SCC node
	openFirmware6100Tree[16]._nodeType = emptyNodeType;
	openFirmware6100Tree[16]._parentNode = 15;
	openFirmware6100Tree[16]._prevSibling = -1;
	openFirmware6100Tree[16]._nextSibling = 19;
	openFirmware6100Tree[16]._childNode = 17;
	strcpy( openFirmware6100Tree[16]._name, "escc" );
	strcpy( openFirmware6100Tree[16]._devType, "escc" );
	
	// 17 - SCC channel A
	openFirmware6100Tree[17]._nodeType = deviceNodeType;
	openFirmware6100Tree[17]._parentNode = 16;
	openFirmware6100Tree[17]._prevSibling = -1;
	openFirmware6100Tree[17]._nextSibling = 18;
	openFirmware6100Tree[17]._childNode = -1;
	strcpy( openFirmware6100Tree[17]._name, "ch-a" );
	strcpy( openFirmware6100Tree[17]._devType, "serial" );
	openFirmware6100Tree[17].deviceNode._address[0] = 0x50F04020;		// Do something about DMA addresses
	openFirmware6100Tree[17].deviceNode._address[1] = -1;
	openFirmware6100Tree[17].deviceNode._address[2] = -1;
	openFirmware6100Tree[17].deviceNode._address[3] = -1;
	openFirmware6100Tree[17].deviceNode._interrupts[0] = 15;
	openFirmware6100Tree[17].deviceNode._interrupts[1] = 4;
	openFirmware6100Tree[17].deviceNode._interrupts[2] = 5;
	openFirmware6100Tree[17].deviceNode._interrupts[3] = -1;
	
	// 18 - SCC channel B
	openFirmware6100Tree[18]._nodeType = deviceNodeType;
	openFirmware6100Tree[18]._parentNode = 16;
	openFirmware6100Tree[18]._prevSibling = 17;
	openFirmware6100Tree[18]._nextSibling = -1;
	openFirmware6100Tree[18]._childNode = -1;
	strcpy( openFirmware6100Tree[18]._name, "ch-b" );
	strcpy( openFirmware6100Tree[18]._devType, "serial" );
	openFirmware6100Tree[18].deviceNode._address[0] = 0x50F04000;		// Do something about DMA addresses
	openFirmware6100Tree[18].deviceNode._address[1] = -1;
	openFirmware6100Tree[18].deviceNode._address[2] = -1;
	openFirmware6100Tree[18].deviceNode._address[3] = -1;
	openFirmware6100Tree[18].deviceNode._interrupts[0] = 16;
	openFirmware6100Tree[18].deviceNode._interrupts[1] = 6;
	openFirmware6100Tree[18].deviceNode._interrupts[2] = 7;
	openFirmware6100Tree[18].deviceNode._interrupts[3] = -1;
	
	// 19 - MACE ethernet
	openFirmware6100Tree[19]._nodeType = deviceNodeType;
	openFirmware6100Tree[19]._parentNode = 15;
	openFirmware6100Tree[19]._prevSibling = 16;
	openFirmware6100Tree[19]._nextSibling = 20;
	openFirmware6100Tree[19]._childNode = -1;
	strcpy( openFirmware6100Tree[19]._name, "mace" );
	openFirmware6100Tree[19]._devType[0] = 0;
	openFirmware6100Tree[19].deviceNode._address[0] = 0x50F0A000;
	openFirmware6100Tree[19].deviceNode._address[1] = 0x50F31C20;
	openFirmware6100Tree[19].deviceNode._address[2] = -1;
	openFirmware6100Tree[19].deviceNode._address[3] = -1;
	openFirmware6100Tree[19].deviceNode._interrupts[0] = 14;
	openFirmware6100Tree[19].deviceNode._interrupts[1] = 2;
	openFirmware6100Tree[19].deviceNode._interrupts[2] = 3;
	openFirmware6100Tree[19].deviceNode._interrupts[3] = -1;
	
	// 20 - 53C94 chip
	openFirmware6100Tree[20]._nodeType = deviceNodeType;
	openFirmware6100Tree[20]._parentNode = 14;
	openFirmware6100Tree[20]._prevSibling = 19;
	openFirmware6100Tree[20]._nextSibling = -1;
	openFirmware6100Tree[20]._childNode = 21;
	strcpy( openFirmware6100Tree[20]._name, "53c94" );
	strcpy( openFirmware6100Tree[20]._devType, "scsi" );
	openFirmware6100Tree[20].deviceNode._address[0] = 0x50F10000;		// Do something about DMA addresses
	openFirmware6100Tree[20].deviceNode._address[1] = 0x50F32000;		// - OK, but it's never used anyhow...
	openFirmware6100Tree[20].deviceNode._address[2] = -1;
	openFirmware6100Tree[20].deviceNode._address[3] = -1;
	openFirmware6100Tree[20].deviceNode._interrupts[0] = 12;
	openFirmware6100Tree[20].deviceNode._interrupts[1] = 0;
	openFirmware6100Tree[20].deviceNode._interrupts[2] = -1;
	openFirmware6100Tree[20].deviceNode._interrupts[3] = -1;
	
	// 21 - sd for 53c94
	openFirmware6100Tree[21]._nodeType = emptyNodeType;
	openFirmware6100Tree[21]._parentNode = 20;
	openFirmware6100Tree[21]._prevSibling = -1;
	openFirmware6100Tree[21]._nextSibling = 22;
	openFirmware6100Tree[21]._childNode = -1;
	strcpy( openFirmware6100Tree[21]._name, "sd" );
	strcpy( openFirmware6100Tree[21]._devType, "block" );
	
	// 22 - st for 53c94
	openFirmware6100Tree[22]._nodeType = emptyNodeType;
	openFirmware6100Tree[22]._parentNode = 20;
	openFirmware6100Tree[22]._prevSibling = 21;
	openFirmware6100Tree[22]._nextSibling = -1;
	openFirmware6100Tree[22]._childNode = -1;
	strcpy( openFirmware6100Tree[22]._name, "st" );
	strcpy( openFirmware6100Tree[22]._devType, "byted" );
	
	// 23 - BART Nubus controller - don't know anything about this, except that it exists!
	openFirmware6100Tree[23]._nodeType = emptyNodeType;
	openFirmware6100Tree[23]._parentNode = 0;
	openFirmware6100Tree[23]._prevSibling = 15;
	openFirmware6100Tree[23]._nextSibling = -1;
	openFirmware6100Tree[23]._childNode = -1;
	strcpy( openFirmware6100Tree[23]._name, "bart" );
	openFirmware6100Tree[23]._devType[0] = 0;
	
	// Now for 8100 tree (it has an extra 53cf94 scsi controller)
	BlockMoveData( openFirmware6100Tree, openFirmware8100Tree, sizeof(openFirmware6100Tree) );
	openFirmware8100Tree[23]._nextSibling = 24;			// Link next node
	
	// 24 - 53cf94 fast SCSI controller
	openFirmware8100Tree[24]._nodeType = deviceNodeType;
	openFirmware8100Tree[24]._parentNode = 0;
	openFirmware8100Tree[24]._prevSibling = 23;
	openFirmware8100Tree[24]._nextSibling = -1;
	openFirmware8100Tree[24]._childNode = 25;
	strcpy( openFirmware8100Tree[24]._name, "53cf94" );
	strcpy( openFirmware8100Tree[24]._devType, "scsi" );
	openFirmware8100Tree[24].deviceNode._address[0] = 0x50F11000;		// Do something about DMA addresses
	openFirmware8100Tree[24].deviceNode._address[1] = 0x50F32000;		// - see above, they use the same address
	openFirmware8100Tree[24].deviceNode._address[2] = -1;
	openFirmware8100Tree[24].deviceNode._address[3] = -1;
	openFirmware8100Tree[24].deviceNode._interrupts[0] = 13;
	openFirmware8100Tree[24].deviceNode._interrupts[1] = 10;
	openFirmware8100Tree[24].deviceNode._interrupts[2] = -1;
	openFirmware8100Tree[24].deviceNode._interrupts[3] = -1;
	
	// 25 - sd for 53cf94
	openFirmware8100Tree[25]._nodeType = emptyNodeType;
	openFirmware8100Tree[25]._parentNode = 24;
	openFirmware8100Tree[25]._prevSibling = -1;
	openFirmware8100Tree[25]._nextSibling = 26;
	openFirmware8100Tree[25]._childNode = -1;
	strcpy( openFirmware8100Tree[25]._name, "sd" );
	strcpy( openFirmware8100Tree[25]._devType, "block" );
	
	// 26 - st for 53cf94
	openFirmware8100Tree[26]._nodeType = emptyNodeType;
	openFirmware8100Tree[26]._parentNode = 24;
	openFirmware8100Tree[26]._prevSibling = 25;
	openFirmware8100Tree[26]._nextSibling = -1;
	openFirmware8100Tree[26]._childNode = -1;
	strcpy( openFirmware8100Tree[26]._name, "st" );
	strcpy( openFirmware8100Tree[26]._devType, "byted" );
}

void MakeOpenFirmwareTree()
{
	// See if the Name Registry is present
	long	gestaltValue;
	Gestalt(gestaltMachineType,&gestaltValue);
	switch(gestaltValue)
	{
		case gestaltPowerMac6100_60:
		case 111:	// 7100_60 (not released)
			FillInNuBusTrees();
			openFirmware6100Tree[1].cpuNode._clockFrequency = 60000000;
			openFirmware6100Tree[1].cpuNode._busFrequency = 30000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware6100Tree);
		break;
		
		case gestaltPowerMac6100_66:
		case gestaltPowerMac7100_66:
			FillInNuBusTrees();
			openFirmware6100Tree[1].cpuNode._clockFrequency = 66000000;
			openFirmware6100Tree[1].cpuNode._busFrequency = 33000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware6100Tree);
		break;
		
		case 101:	// 6100_80 (not released)
		case gestaltPowerMac7100_80:
		case 113:	// 7100_80 with pre-release ROM
			FillInNuBusTrees();
			openFirmware6100Tree[1].cpuNode._clockFrequency = 80000000;
			openFirmware6100Tree[1].cpuNode._busFrequency = 40000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware6100Tree);
		break;
		
		case 114:	// 7100_82 (not released)
			FillInNuBusTrees();
			openFirmware6100Tree[1].cpuNode._clockFrequency = 82000000;
			openFirmware6100Tree[1].cpuNode._busFrequency = 41000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware6100Tree);
		break;
		
		case gestaltPowerMac8100_110:
			FillInNuBusTrees();
			openFirmware8100Tree[1].cpuNode._clockFrequency = 110000000;
			openFirmware8100Tree[1].cpuNode._busFrequency = 36666667;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware8100Tree);
		break;
		
		case gestaltPowerMac8100_100:
		case 66:	// 8100_100 with pre-rel ROM
			FillInNuBusTrees();
			openFirmware8100Tree[1].cpuNode._clockFrequency = 100000000;
			openFirmware8100Tree[1].cpuNode._busFrequency = 33333333;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware8100Tree);
		break;
		
		case 61:	// 8100_80 at 60 MHz
			FillInNuBusTrees();
			openFirmware8100Tree[1].cpuNode._clockFrequency = 60000000;
			openFirmware8100Tree[1].cpuNode._busFrequency = 30000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware8100Tree);
		break;
		
		case 64:	// 8100_60/80 at 66.6 MHz
			FillInNuBusTrees();
			openFirmware8100Tree[1].cpuNode._clockFrequency = 66666667;
			openFirmware8100Tree[1].cpuNode._busFrequency = 33333333;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware8100Tree);
		break;
		
		case gestaltPowerMac8100_80:
			FillInNuBusTrees();
			openFirmware8100Tree[1].cpuNode._clockFrequency = 80000000;
			openFirmware8100Tree[1].cpuNode._busFrequency = 40000000;
			openFirmwareTree = new OpenFirmwareTree((char*)openFirmware8100Tree);
		break;
		
		default:	// OpenFirmware compatible machine
			if(Gestalt('nreg',&gestaltValue))
				return;
			if(gestaltValue < 0)
				return;
			
			// Find the root:Devices:device-tree node
			RegEntryID	deviceNode;
			if(RegistryCStrEntryLookup(nil,"Devices:device-tree",&deviceNode))
				return;
			
			IterateChildren(&deviceNode,nil);
			openFirmwareTree = new OpenFirmwareTree(openFirmwareNodeCount);
			IterateChildren(&deviceNode,nil);
		break;
	}
	bootInfo.openFirmwareTreeLen = openFirmwareTree->getFlattenedTreeLength();
}

// For PCI bridges
typedef struct InterruptMapEntry
{
	UInt32	devFN;
	UInt32	junk[4];
	UInt32	irq;
}InterruptMapEntry;

void IterateChildren(RegEntryID* currNode,OpenFirmwareNode* parent)
{
	// Get the name of the current node
	OpenFirmwareNode*	currOFNode;
	char				name[48];
	RegEntryID		parentNode;
	Boolean			done;
	if(RegistryCStrEntryToName(currNode,&parentNode,name,&done))
		return;
	
	// Make sure this isn't the root node
	if(strcmp(name,"device-tree"))
	{
		// See if we should ignore this node
		for(long i=0;i<sizeof(ignoredOpenFirmwareNodes)/sizeof(ignoredOpenFirmwareNodes[0]);i++)
		{
			if(!strcmp(name,ignoredOpenFirmwareNodes[i]))
				return;
		}
		
		// Find out what kind of node this is
		if(!strncmp(name,"PowerPC",7))
		{
			// This is a CPU node
			if(openFirmwareTree)
			{
				unsigned long			cpuVersion = 0;
				unsigned long			clockFrequency = 0;
				unsigned long			dCacheSize = 0;
				unsigned long			iCacheSize = 0;
				
				RegPropertyValueSize	size = sizeof(cpuVersion);
				RegistryPropertyGet(currNode,"cpu-version",(void*)&cpuVersion,&size);
				size = sizeof(clockFrequency);
				RegistryPropertyGet(currNode,"clock-frequency",(void*)&clockFrequency,&size);
				size = sizeof(dCacheSize);
				RegistryPropertyGet(currNode,"d-cache-size",(void*)&dCacheSize,&size);
				size = sizeof(iCacheSize);
				RegistryPropertyGet(currNode,"i-cache-size",(void*)&iCacheSize,&size);
				
				currOFNode = new OpenFirmwareCPUNode(parent,name,cpuVersion,clockFrequency,busFrequency,dCacheSize,iCacheSize);
			}
			else
				openFirmwareNodeCount++;
		}
		else if(!strcmp(name,"memory"))
		{
			// This is a memory node
			if(openFirmwareTree)
			{
				unsigned long	reg[2] = {0,0};
				
				RegPropertyValueSize	size = sizeof(reg);
				RegistryPropertyGet(currNode,"reg",(void*)reg,&size);
				
				currOFNode = new OpenFirmwareMemoryNode(parent,name,reg[0],reg[1]);
			}
			else
				openFirmwareNodeCount++;
		}
		else if(!strcmp(name,"l2-cache"))
		{
			// This is a cache node
			if(openFirmwareTree)
			{
				unsigned long	iCacheSize = 0;
				unsigned long	dCacheSize = 0;
				unsigned long	iCacheSets = 0;
				unsigned long	dCacheSets = 0;
				char			unified = true;
				
				RegPropertyValueSize	size = sizeof(iCacheSize);
				RegistryPropertyGet(currNode,"i-cache-size",(void*)&iCacheSize,&size);
				size = sizeof(dCacheSize);
				RegistryPropertyGet(currNode,"d-cache-size",(void*)&dCacheSize,&size);
				size = sizeof(iCacheSets);
				RegistryPropertyGet(currNode,"i-cache-sets",(void*)&iCacheSets,&size);
				size = sizeof(dCacheSets);
				RegistryPropertyGet(currNode,"d-cache-sets",(void*)&dCacheSets,&size);
				
				currOFNode = new OpenFirmwareCacheNode(parent,name,iCacheSize,dCacheSize,iCacheSets,dCacheSets,unified);
			}
			else
				openFirmwareNodeCount++;
		}
		else if(	!strcmp(name,"bandit") ||
				!strcmp(name,"chaos") ||
				!strcmp(name,"pci") ||
				!strcmp(name,"pci-bridge"))
		{
			// This is a PCI bridge
			if(openFirmwareTree)
			{
				unsigned long	reg[6] = {0,0};
				unsigned long	busRange[4] = {0,0};
				
				RegPropertyValueSize	size = 8;
				RegistryPropertyGet(currNode,"reg",(void*)reg,&size);
				size = 8;
				RegistryPropertyGet(currNode,"bus-range",(void*)busRange,&size);
				
				// Build the interrupt map table
				InterruptMapEntry	intMap[MAX_PCI_IRQ_COUNT];
				UInt8			devFNList[MAX_PCI_IRQ_COUNT];
				UInt8			devIRQList[MAX_PCI_IRQ_COUNT];
				UInt8			numIRQs = 0;
				if(RegistryPropertyGetSize(currNode,"interrupt-map",&size))
					size = 0;
				if(size)
				{
					// We have an "interrupt-map" property, this makes it easier
					numIRQs = size/sizeof(InterruptMapEntry);
					if(numIRQs > MAX_PCI_IRQ_COUNT)
						UnrecoverableError("\pNumber of IRQs for a PCI bridge was > MAX_PCI_IRQ_COUNT - please bump it up.");
					RegistryPropertyGet(currNode,"interrupt-map",(void*)intMap,&size);
					for(UInt32 i=0;i<numIRQs;i++)
					{
						devFNList[i] = (intMap[i].devFN >> 8);
						devIRQList[i] = intMap[i].irq;
					}
				}
				else
				{
					// No "interrupt-map", construct our own by parsing all the "AAPL,interrupts" properties of the children of this node
					RegEntryIter	pciCookie;
					if(RegistryEntryIterateCreate(&pciCookie))
						UnrecoverableError("\pCouldn't create cookie to parse PCI bridge children.");
					if(RegistryEntryIterateSet(&pciCookie,currNode))
						UnrecoverableError("\pCouldn't set cookie to parse PCI bridge children.");
					
					RegEntryID	parseEntry;
					Boolean		donePCIParse;
					Boolean		iterSiblings = true;
					RegistryEntryIterate(&pciCookie,kRegIterChildren,&parseEntry,&donePCIParse);
					while(!donePCIParse)
					{
						UInt32	AAPLinterrupts;
						UInt32	devFN;
						if(RegistryPropertyGetSize(&parseEntry,"AAPL,interrupts",&size))
							size = 0;
						if(size)
						{
							if(RegistryPropertyGetSize(&parseEntry,"reg",&size))
								size = 0;
							if(size)
							{
								size = 4;
								RegistryPropertyGet(&parseEntry,"AAPL,interrupts",(void*)&AAPLinterrupts,&size);
								size = 4;
								RegistryPropertyGet(&parseEntry,"reg",(void*)&devFN,&size);
								devFNList[numIRQs] = (devFN >> 8);
								devIRQList[numIRQs] = AAPLinterrupts;
								numIRQs++;
							}
						}
						if(iterSiblings)
						{
							RegistryEntryIterate(&pciCookie,kRegIterSibling,&parseEntry,&donePCIParse);
							iterSiblings = false;
						}
						else
							RegistryEntryIterate(&pciCookie,kRegIterContinue,&parseEntry,&donePCIParse);
					}
				}
				
				// Make the node
				currOFNode = new OpenFirmwarePCINode(parent,name,reg[0],reg[1],busRange[0],busRange[1],devFNList,devIRQList,numIRQs);
			}
			else
				openFirmwareNodeCount++;
		}
		else
		{
			// This is a device node if it has an "AAPL,interrupts" or "AAPL,address" property
			unsigned long 			interrupts[4] = {-1,-1,-1,-1};
			RegPropertyValueSize	size;
			char					isDevice = false;
			if(!isDevice)
			{
				size = sizeof(interrupts);
				if(!RegistryPropertyGet(currNode,"interrupts",(void*)interrupts,&size))
					isDevice = true;
			}
			if(!isDevice)
			{
				size = sizeof(interrupts);
				if(!RegistryPropertyGet(currNode,"AAPL,interrupts",(void*)interrupts,&size))
					isDevice = true;
			}
			
			unsigned long			addresses[4] = {-1,-1,-1,-1};
			size = sizeof(addresses);
			if(!RegistryPropertyGet(currNode,"AAPL,address",(void*)addresses,&size))
				isDevice = true;
			ASCII8				devType[32];
			RegPropertyValueSize	devTypeLen;
			if(RegistryPropertyGetSize(currNode,"device_type",&devTypeLen))
				devType[0] = 0;
			else if(!devTypeLen)
				devType[0] = 0;
			else if(RegistryPropertyGet(currNode,"device_type",(void*)devType,&devTypeLen))
				devType[0] = 0;
			if(isDevice)
			{
				// This is a device of some sort
				if(openFirmwareTree)
					currOFNode = new OpenFirmwareDeviceNode(parent,name,devType,addresses[0],addresses[1],addresses[2],addresses[3],interrupts[0],interrupts[1],interrupts[2],interrupts[3]);
				else
					openFirmwareNodeCount++;
			}
			else
			{
				// This is an empty node
				if(openFirmwareTree)
					currOFNode = new OpenFirmwareEmptyNode(parent,name,devType);
				else
					openFirmwareNodeCount++;
			}
		}
	}
	else
	{
		RegPropertyValueSize	size = sizeof(busFrequency);
		RegistryPropertyGet(currNode,"clock-frequency",(void*)&busFrequency,&size);
		if(openFirmwareTree)
			currOFNode = openFirmwareTree->getNode(0);
		else
			currOFNode = nil;
	}
	
	// Iterate through all the children nodes
	RegEntryIter	cookie;
	if(RegistryEntryIterateCreate(&cookie))
		return;
	if(RegistryEntryIterateSet(&cookie,currNode))
		return;
	
	// Find the first child
	RegEntryID	nextEntry;
	if(RegistryEntryIterate(&cookie,kRegIterChildren,&nextEntry,&done))
		return;
	
	// Do the first sibling
	if(!done)
	{
		IterateChildren(&nextEntry,currOFNode);
		if(RegistryEntryIterate(&cookie,kRegIterSibling,&nextEntry,&done))
			return;
		
		// Now iterate over all the other siblings
		while(!done)
		{
			IterateChildren(&nextEntry,currOFNode);
			if(RegistryEntryIterate(&cookie,kRegIterContinue,&nextEntry,&done))
				return;
		}
	}
	
	RegistryEntryIterateDispose(&cookie);
}

void SetUpMPStuff()
{
	/*
	if(MPLibraryIsLoaded())
		bootInfo.numProcessors = MPProcessors();
	else
	*/
		bootInfo.numProcessors = 1;
	
	if(bootInfo.numProcessors == 1)
		return;
	
	// Set up a task for each of the other processors
	
}
