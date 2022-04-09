/*
	SCSI Command Factory.h
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
#ifndef __SCSI_COMMAND_FACTORY__
#define __SCSI_COMMAND_FACTORY__

struct SCSICommandFactory
{
	/* All device types:	p. 93 (PDF 129)
		Mandatory:
			Inquiry						8.2.5
			Request Sense					8.2.14
			Send Diagnostic					8.2.15
			Test Unit Ready					8.2.16
		Optional:
			Change Definition				8.2.1
			Compare						8.2.2
			Copy							8.2.3
			Copy And Verify				8.2.4
			Log Select						8.2.6
			Log Sense						8.2.7
			Read Buffer					8.2.12
			Receive Diagnostic Results		8.2.13
			Write Buffer					8.2.17
		Device type specific:
			Mode Select(6)					8.2.8
			Mode Select(10)				8.2.9
			Mode Sense(6)					8.2.10
			Mode Sense(10)				8.2.11
	*/
	static SCSICommand*	inquiry(SCSIDevice* device,SCSIInquiryData* data);
	static SCSICommand*	requestSense(SCSIDevice* device,scsi_sense_data* data);
	static SCSICommand*	sendDiagnostic(SCSIDevice* device);
	static SCSICommand*	testUnitReady(SCSIDevice* device);
	
	static SCSICommand*	changeDefinition(SCSIDevice* device,UInt32 defParam);
	static SCSICommand*	compare(SCSIDevice* device);
	static SCSICommand*	copy(SCSIDevice* device);
	static SCSICommand*	copyAndVerify(SCSIDevice* device);
	static SCSICommand*	logSelect(SCSIDevice* device);
	static SCSICommand*	logSense(SCSIDevice* device);
	static SCSICommand*	readBuffer(SCSIDevice* device);
	static SCSICommand*	receiveDiagnosticResults(SCSIDevice* device);
	static SCSICommand*	writeBuffer(SCSIDevice* device);
	
	static SCSICommand*	modeSelect6(SCSIDevice* device);
	static SCSICommand*	modeSelect10(SCSIDevice* device);
	static SCSICommand*	modeSense6(SCSIDevice* device);
	static SCSICommand*	modeSense10(SCSIDevice* device);
	
	/* Direct-access devices: p. 168 (PDF 204)
		Mandatory:
			Format Unit					9.2.1
			Read(6)						9.2.5
			Read(10)						9.2.6
			Read Capacity					9.2.7
			Release						9.2.11
			Reserve						9.2.12
		Optional:
			Lock-Unlock Cache				9.2.2
			Pre-Fetch						9.2.3
			Prevent-Allow Medium Removal	9.2.4
			Read Defect Data				9.2.8
			Read Long						9.2.9
			Reassign Blocks					9.2.10
			Rezero Unit					9.2.13
			Search Data Equal				9.2.14.1
			Search Data High				9.2.14.2
			Search Data Low				9.2.14.3
			Seek(6)						9.2.15
			Seek(10)						9.2.15
			Set Limits						9.2.16
			Start Stop Unit					9.2.17
			Synchronize Cache				9.2.18
			Verify						9.2.19
			Write(6)						9.2.20
			Write(10)						9.2.21
			Write and Verify				9.2.22
			Write Long					9.2.23
			Write Same					9.2.24
	*/
	static SCSICommand*	formatUnit(SCSIDevice* device);
	static SCSICommand*	read6(SCSIDevice* device);
	static SCSICommand*	read10(SCSIDevice* device,Int8* p,UInt32 sector,UInt32 numSectors,UInt32 sectorSize);
	static SCSICommand*	readCapacity(SCSIDevice* device,UInt32** size,UInt32** num);
	static SCSICommand*	release(SCSIDevice* device);
	static SCSICommand*	reserve(SCSIDevice* device);
	
	static SCSICommand*	lockUnlockCache(SCSIDevice* device);
	static SCSICommand*	preFetch(SCSIDevice* device);
	static SCSICommand*	preventAllowMediumRemoval(SCSIDevice* device,Boolean allow);
	static SCSICommand*	readDefectData(SCSIDevice* device);
	static SCSICommand*	readLong(SCSIDevice* device);
	static SCSICommand*	reassignBlocks(SCSIDevice* device);
	static SCSICommand*	rezeroUnit(SCSIDevice* device);
	static SCSICommand*	searchDataEqual(SCSIDevice* device);
	static SCSICommand*	searchDataHigh(SCSIDevice* device);
	static SCSICommand*	searchDataLow(SCSIDevice* device);
	static SCSICommand*	seek6(SCSIDevice* device);
	static SCSICommand*	seek10(SCSIDevice* device);
	static SCSICommand*	setLimits(SCSIDevice* device);
	static SCSICommand*	startStopUnit(SCSIDevice* device,Boolean immediate,Boolean loadEject,Boolean start);
	static SCSICommand*	synchronizeCache(SCSIDevice* device);
	static SCSICommand*	verify(SCSIDevice* device);
	static SCSICommand*	write6(SCSIDevice* device);
	static SCSICommand*	write10(SCSIDevice* device);
	static SCSICommand*	writeAndVerify(SCSIDevice* device);
	static SCSICommand*	writeLong(SCSIDevice* device);
	static SCSICommand*	writeSame(SCSIDevice* device);
	
	/* Sequential-access devices: p. 242 (PDF 278)
		Mandatory:
			Erase						10.2.1
			Mode Select(6)					8.2.8
			Mode Sense(6)					8.2.8
			Read							10.2.4
			Read Block Limits				10.2.5
			Release Unit					10.2.9
			Reserve Unit					10.2.10
			Rewind						10.2.11
			Space						10.2.12
			Write						10.2.14
			Write Filemarks				10.2.15
		Optional:
			Load Unload					10.2.2
			Locate						10.2.3
			Mode Select(10)				8.2.9
			Mode Sense(10)				8.2.11
			Prevent Allow Medium Removal		9.2.4
			Read Position					10.2.6
			Read Reverse					10.2.7
			Recover Buffered Data			10.2.8
			Verify						10.2.13
	*/
	static SCSICommand*	erase(SCSIDevice* device);
	static SCSICommand*	read(SCSIDevice* device);
	static SCSICommand*	readBlockLimits(SCSIDevice* device);
	static SCSICommand*	releaseUnit(SCSIDevice* device);
	static SCSICommand*	reserveUnit(SCSIDevice* device);
	static SCSICommand*	rewind(SCSIDevice* device);
	static SCSICommand*	space(SCSIDevice* device);
	static SCSICommand*	write(SCSIDevice* device);
	static SCSICommand*	writeFilemarks(SCSIDevice* device);
	
	static SCSICommand*	loadUnload(SCSIDevice* device);
	static SCSICommand*	locate(SCSIDevice* device);
	static SCSICommand*	readPosition(SCSIDevice* device);
	static SCSICommand*	readReverse(SCSIDevice* device);
	static SCSICommand*	recoverBufferedData(SCSIDevice* device);
	//static SCSICommand*	verify(SCSIDevice* device);
	
	/* Printer devices: p. 274 (PDF 310)
		Mandatory:
			Print							11.2.2
			Release Unit					10.2.9
			Reserve Unit					10.2.10
		Optional:
			Format						11.2.1
			Mode Select(6)					8.2.8
			Mode Select(10)				8.2.9
			Mode Sense(6)					8.2.10
			Mode Sense(10)				8.2.11
			Recover Buffered Data			11.2.3
			Slew and Print					11.2.4
			Stop Print						11.2.5
			Synchronize Buffer				11.2.6
	*/
	static SCSICommand*	print(SCSIDevice* device);
	
	static SCSICommand*	format(SCSIDevice* device);
	//static SCSICommand*	recoverBufferedData(SCSIDevice* device);
	static SCSICommand*	slewAndPrint(SCSIDevice* device);
	static SCSICommand*	stopPrint(SCSIDevice* device);
	static SCSICommand*	synchronizeBuffer(SCSIDevice* device);
	
	/* Processor devices: p. 289 (PDF 325)
		Mandatory:
			Send							12.2.2
		Optional:
			Receive						12.2.1
	*/
	static SCSICommand*	send(SCSIDevice* device);
	static SCSICommand*	receive(SCSIDevice* device);
	
	/* Write-once devices: p. 295 (PDF 331)
		Mandatory:
			Read(10)						9.2.6
			Read Capacity					9.2.7
			Release						9.2.11
			Reserve						9.2.12
			Write(10)						9.2.21
		Optional:
			Lock Unlock Cache				9.2.2
			Medium Scan					16.2.3
			Mode Select(6)					8.2.8
			Mode Select(10)				8.2.9
			Mode Sense(6)					8.2.10
			Mode Sense(10)				8.2.11
			Pre-Fetch						9.2.3
			Prevent Allow Medium Removal		9.2.4
			Read(6)						9.2.5
			Read(12)						16.2.4
			Read Long						9.2.9
			Reassign Blocks					9.2.10
			Rezero Unit					9.2.13
			Search Data Equal(10)			9.2.14.1
			Search Data Equal(12)			16.2.8
			Search Data High(10)			9.2.14.2
			Search Data High(12)			16.2.8
			Search Data Low(10)				9.2.14.3
			Search Data Low(12)				16.2.8
			Seek(6)						9.2.15
			Seek(10)						9.2.15
			Set Limits(10)					9.2.16
			Set Limits(12)					16.2.9
			Start Stop Unit					9.2.17
			Synchronize Cache				9.2.18
			Verify(10)					16.2.11
			Verify(12)					16.2.12
			Write(6)						9.2.20
			Write(12)						16.2.14
			Write and Verify(10)				9.2.22
			Write and Verify(12)				16.2.16
			Write Long					9.2.23
	*/
	// ***** Doesn't define anything new
	
	/* CD-ROM devices: p. 305 (PDF 341)
		Mandatory:
			Read(10)						9.2.6
			Read CD-ROM Capacity			14.2.8
			Release						9.2.11
			Reserve						9.2.12
		Optional:
			Lock/Unlock Cache				9.2.2
			Mode Select(6)					8.2.8
			Mode Select(10)				8.2.9
			Mode Sense(6)					8.2.10
			Mode Sense(10)				8.2.11
			Pause/Resume					14.2.1
			Play Audio(10)					14.2.2
			Play Audio(12)					14.2.3
			Play Audio MSF					14.2.4
			Play Audio Track/Index			14.2.5
			Play Track Relative(10)			14.2.6
			Play Track Relative(12)			14.2.7
			Pre-Fetch						9.2.3
			Prevent/Allow Medium Removal	9.2.4
			Read(6)						9.2.5
			Read(12)						14.2.4
			Read Header					14.2.9
			Read Long						9.2.9
			Read Sub-Channel				14.2.10
			Read TOC						14.2.11
			Rezero Unit					9.2.13
			Search Data Equal(10)			9.2.14.1
			Search Data Equal(12)			16.2.8
			Search Data High(10)			9.2.14.2
			Search Data High(12)			16.2.8
			Search Data Low(10)				9.2.14.3
			Search Data Low(12)				16.2.8
			Seek(6)						9.2.15
			Seek(10)						9.2.15
			Set Limits(10)					9.2.16
			Set Limits(12)					16.2.9
			Start Stop Unit					9.2.17
			Synchronize Cache				9.2.18
			Verify(10)					16.2.11
			Verify(12)					16.2.12
	*/
	static SCSICommand*	readCDROMCapacity(SCSIDevice* device);
	
	static SCSICommand*	pauseResume(SCSIDevice* device);
	static SCSICommand*	playAudio10(SCSIDevice* device);
	static SCSICommand*	playAudio12(SCSIDevice* device);
	static SCSICommand*	playAudioMSF(SCSIDevice* device);
	static SCSICommand*	playAudioTrackIndex(SCSIDevice* device);
	static SCSICommand*	playTrackRelative10(SCSIDevice* device);
	static SCSICommand*	playTrackRelative12(SCSIDevice* device);
	static SCSICommand*	read12(SCSIDevice* device);
	static SCSICommand*	readHeader(SCSIDevice* device);
	static SCSICommand*	readSubChannel(SCSIDevice* device);
	static SCSICommand*	readTOC(SCSIDevice* device);
	
	/* Scanner devices: p. 342 (PDF 378)
		Mandatory:
			Read							15.2.4
			Release Unit					10.2.9
			Reserve Unit					10.2.10
			Set Window					15.2.7
		Optional:
			Get Data Buffer Status			15.2.1
			Get Window					15.2.2
			Mode Select(6)					8.2.8
			Mode Select(10)				8.2.9
			Mode Sense(6)					8.2.10
			Mode Sense(10)				8.2.11
			Object Position					15.2.3
			Scan							15.2.5
			Send							15.2.6
	*/
	//static SCSICommand*	read(SCSIDevice* device);
	static SCSICommand*	setWindow(SCSIDevice* device);
	
	static SCSICommand*	getDataBufferStatus(SCSIDevice* device);
	static SCSICommand*	getWindow(SCSIDevice* device);
	static SCSICommand*	objectPosition(SCSIDevice* device);
	static SCSICommand*	scan(SCSIDevice* device);
	//static SCSICommand*	send(SCSIDevice* device);
	
	/* Optical Memory devices: p. 361 (PDF 397)
		Fill in later
	*/

	/* Medium-changer devices: p. 385 (PDF 421)
		Fill in later
	*/

	/* Communication devices: p. 417 (PDF 453)
		Fill in later
	*/
};

#endif /*__SCSI_COMMAND_FACTORY__ */
