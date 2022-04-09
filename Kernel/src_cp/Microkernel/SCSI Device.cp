/*
	SCSI Device.cp
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
	scsi.c				Mach DR2.1 update 6		Alessandro Forin	SCSIDirectAccessDevice::initialize() based on scsi_attach().
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Thurday, 18 June 98	-	Bunch of mods to fit in new BlockDevice architecture
*/
#include "Kernel Console.h"
#include "NKVideo.h"
#include "Memory Utils.h"
#include "SCSI Commands.h"
#include "SCSI Driver.h"
#include "SCSI Device.h"
#include "SCSI Command Factory.h"
#include "ASC.h"
#include "Mesh.h"
#include "Apple Block Device.h"
#include "NKMachineInit.h"
#include "NKThreads.h"

#define RETRY_TIMES	30
#define SCSI_DISP_MSGS	0
#if SCSI_DISP_MSGS
#define SCSI_MSG(msg)	nkVideo << msg
#else
#define SCSI_MSG(msg)
#endif

static void PrintDevInfo(SCSIDevice* dev);
static void PrintSense(scsi_sense_data* data,UInt32 scsiID);

void InitSCSI(void)
{
	InitMesh();
	InitASC();
}

void InitSCSIVolumes(void)
{
	SCSIBus*	scsiBus = machine.scsiBusses;
	
	if(!scsiBus)
		return;
	
	cout << "\nDetermining SCSI partition types:\n\n";
	while(scsiBus)
	{
		for(Int32 i=0;i<7;i++)
		{
			if(scsiBus->device[i]->probe())
			{
				cout << greenMsg << "Bus " << scsiBus->bus() << ", SCSI ID " << i << " Present:\n" << whiteMsg;
				SCSIDevice*	realDevice = SCSIDevice::makeMeIntoARealSCSIDevice(scsiBus->device[i]);
				if(realDevice)
				{
					delete scsiBus->device[i];
					scsiBus->device[i] = realDevice;
				}
				else
					cout << redMsg << "\t\tBut not supported yet\n" << whiteMsg;
				if(!scsiBus->device[i]->initialize())
					cout << redMsg << "\t\tBut Down\n" << whiteMsg;
				cout << "\n";
			}
		}
		scsiBus = scsiBus->next();
	}
}

void PrintDevInfo(SCSIDevice* dev)
{
	SCSIInquiryData	inqDataBlock;
	ASCII8			string[17];
	SCSICommand*		cmd = dev->inquiry(&inqDataBlock);
	CurrThread::blockForIO(cmd);
	SCSIInquiryData*	inqData = &inqDataBlock;
	
	if(cmd->error == SCSI_RET_SUCCESS)
	{
		cout << "\tDev Type: ";
		switch(inqData->peripheralDevType)
		{
			case directAccessDev:		cout << "Direct access (magnetic disk)\n";	break;
			case sequentialAccessDev:	cout << "Sequential access (magnetic tape)\n";	break;
			case printerDev:			cout << "Printer\n";						break;
			case processorDev:			cout << "Processor\n";					break;
			case writeOnceDev:			cout << "Write-once\n";					break;
			case CDROMDev:			cout << "CD-ROM\n";					break;
			case scannerDev:			cout << "Scanner\n";					break;
			case opticalMemDev:			cout << "Optical Memory\n";				break;
			case mediumChangerDev:		cout << "Medium changer\n";				break;
			case communicationDev:		cout << "Communications\n";				break;
			default:					cout << "Unknown!\n";					break;
		}
		if(inqData->rmb)
			cout << "\tRemovable\n";
		MemCopy(inqData->vendorID,string,8);
		string[8] = '\0';
		cout << "\tVendor: " << string << "\n";
		MemCopy(inqData->productID,string,16);
		string[16] = '\0';
		cout << "\tProduct: " << string << "\n";
		MemCopy(inqData->revisionLevel,string,4);
		string[4] = '\0';
		cout << "\tRev: " << string << "\n";
	}
	delete cmd;
}

SCSIDevice::SCSIDevice(SCSIDriver* _driver,UInt32 bus,UInt32 id,UInt32 unit)
{
	driver = _driver;
	_bus = bus;
	_deviceID = id;
	_unit = unit;
}

SCSIDevice::~SCSIDevice()
{
}

SCSIDevice* SCSIDevice::makeMeIntoARealSCSIDevice(SCSIDevice* oldDev)
{
	SCSI_MSG("SCSIDevice::makeMeIntoARealSCSIDevice\n");
	SCSIInquiryData	inqDataBlock;
	SCSIDevice*		retVal = nil;
	SCSICommand*		cmd = oldDev->inquiry(&inqDataBlock);
	CurrThread::blockForIO(cmd);
	SCSIInquiryData*	inqData = &inqDataBlock;
	
	if(cmd->error == SCSI_RET_SUCCESS)
	{
		switch(inqData->peripheralDevType)
		{
			case directAccessDev:
				retVal = new SCSIDirectAccessDevice(oldDev->bus(),oldDev->deviceID(),oldDev->unit(),oldDev->driver);
			break;
			case CDROMDev:
				retVal = new SCSICDRomDevice(oldDev->bus(),oldDev->deviceID(),oldDev->unit(),oldDev->driver);
			break;
		}
	}
	
	delete cmd;
	return retVal;
}

void SCSIDevice::commandSync(SCSICommand* cmd)
{
	SCSI_MSG("SCSIDevice::commandSync\n");
	driver->command(cmd);
	CurrThread::blockForIO(cmd);
}

void SCSIDevice::commandAsync(SCSICommand* cmd)
{
	SCSI_MSG("SCSIDevice::commandAsync - MSR = " << GetMSR() << "\n");
	driver->command(cmd);
	SCSI_MSG("driver->command() returned - MSR = " << GetMSR() << "\n");
}

UInt32 SCSIDevice::bus()
{
	return _bus;
}

UInt32 SCSIDevice::deviceID()
{
	return _deviceID;
}

UInt32 SCSIDevice::unit()
{
	return _unit;
}

Boolean SCSIDevice::probe()
{
	SCSI_MSG("SCSIDevice::probe - MSR = " << GetMSR() << "\n";);
	SCSICommand*	cmd = inquiry();
	SCSI_MSG("inquiry() returned - blocking - MSR = " << GetMSR() << "\n");
	CurrThread::blockForIO(cmd);
	UInt32	error = cmd->error;
	delete cmd;
	return (error == SCSI_RET_SUCCESS);
}

Boolean SCSIDevice::initialize()
{
	// Get inquiry data
	SCSI_MSG("SCSIDevice::initialize\n");
	SCSIInquiryData	inquiryData;
	SCSICommand*		cmd = inquiry(&inquiryData);
	CurrThread::blockForIO(cmd);
	UInt32		error = cmd->error;
	delete cmd;
	if(error != SCSI_RET_SUCCESS)
		return false;
	
	// Change to SCSI-2 operating definition if possible.  If this doesn't succeed, we still work under SCSI-1
	if(inquiryData.ansiVersion != 2)
	{
		cmd = changeDefinition(3);
		CurrThread::blockForIO(cmd);
		delete cmd;
	}
	
	removable = inquiryData.rmb;
	
	PrintDevInfo(this);
	
	return true;
}

SCSICommand* SCSIDevice::inquiry(struct SCSIInquiryData* data)
{
	SCSI_MSG("SCSIDevice::inquiry - MSR = " << GetMSR() << "\n");
	SCSICommand*	cmd = SCSICommandFactory::inquiry(this,data);
	commandAsync(cmd);
	SCSI_MSG("commandAsync returned - MSR = " << GetMSR() << "\n");
	return cmd;
}

SCSICommand* SCSIDevice::requestSense(scsi_sense_data* data)
{
	SCSI_MSG("SCSIDevice::requestSense\n");
	SCSICommand*	cmd = SCSICommandFactory::requestSense(this,data);
	commandAsync(cmd);
	return cmd;
}

SCSICommand* SCSIDevice::testUnitReady()
{
	SCSI_MSG("SCSIDevice::testUnitReady\n");
	SCSICommand*	cmd = SCSICommandFactory::testUnitReady(this);
	commandAsync(cmd);
	return cmd;
}

SCSICommand* SCSIDevice::changeDefinition(UInt32 defParam)
{
	SCSI_MSG("SCSIDevice::changeDefinition\n");
	SCSICommand*	cmd = SCSICommandFactory::changeDefinition(this,defParam);
	commandAsync(cmd);
	return cmd;
}

SCSIDirectAccessDevice::SCSIDirectAccessDevice(UInt32 bus,UInt32 deviceID,UInt32 unit,SCSIDriver* _driver):
	SCSIDevice(_driver,bus,deviceID,unit)
{
}

SCSIDirectAccessDevice::~SCSIDirectAccessDevice()
{
}

UInt32 SCSIDirectAccessDevice::bus()
{
	return SCSIDevice::bus();
}

UInt32 SCSIDirectAccessDevice::deviceID()
{
	return SCSIDevice::deviceID();
}

UInt32 SCSIDirectAccessDevice::unit()
{
	return SCSIDevice::unit();
}

Boolean SCSIDirectAccessDevice::initialize()
{
	if(!SCSIDevice::initialize())
		return false;
	
	SCSI_MSG("SCSIDirectAccessDevice::initialize");
	
	SCSICommand*	cmd;
	
	// Initialize the device by starting it up
	for(Int32 i=0,inc;i<10;i += inc)
	{
		inc = 1;
		
		// Try to start the unit
		cmd = startStopUnit(true,false,true);
		CurrThread::blockForIO(cmd);
		UInt32 error = cmd->error;
		delete cmd;
		
		if(error == SCSI_RET_SUCCESS)
		{
			// Make sure the unit is ready
			cmd = testUnitReady();
			CurrThread::blockForIO(cmd);
			error = cmd->error;
			delete cmd;
		}
		
		if(error == SCSI_RET_SUCCESS)
			break;
		
		if(error == SCSI_RET_NEED_SENSE)
		{
			scsi_sense_data	senseData;
			cmd = requestSense(&senseData);
			CurrThread::blockForIO(cmd);
			delete cmd;
			if(senseData.sense_key == SCSI_SNS_UNIT_ATN)
			{
				inc = 0;
				continue;
			}
			else if(	(senseData.sense_key == SCSI_SNS_NOTREADY && senseData.asc == 0x3A && senseData.ascq == 0) ||	// Medium not present, but device is OK
					(senseData.sense_key == SCSI_SNS_NOSENSE) )		// Something bad happened
				return false;
		}
		
		CurrThread::sleepS(1);	// Wait for a second for unit to start up
	}
	
	UInt32*	sectorSize;
	UInt32*	numBlocks;
	
	cmd = readCapacity(&sectorSize,&numBlocks);
	CurrThread::blockForIO(cmd);
	
	FatalAssert(*sectorSize == 512);
	
	delete cmd;
	
	// Prevent removal
	cmd = preventAllowMediumRemoval(false);
	CurrThread::blockForIO(cmd);
	delete cmd;
	
	BlockDeviceManager::buildPartitionList(this);
	buildFileSystems();
	
	return true;
}

void SCSIDirectAccessDevice::shutDown(Boolean isShutDown)
{
	// If shutting down, eject all removable media and spin down the disks.
	// If just a restart, simply allow removal of disks
	SCSICommand* cmd = preventAllowMediumRemoval(true);
	CurrThread::blockForIO(cmd);
	delete cmd;
	
	if(isShutDown)
	{
		SCSICommand* cmd = startStopUnit(true,true,false);
		CurrThread::blockForIO(cmd);
		delete cmd;
	}
}

UInt32 SCSIDirectAccessDevice::sectorSize()
{
	return 512;
}

UInt32 SCSIDirectAccessDevice::maxSectorTransfer()
{
	return 65535;
}

IOCommand* SCSIDirectAccessDevice::readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors)
{
	FatalAssert(p != nil);
	FatalAssert(numSectors != 0);
	return read10(p,sector,numSectors);
}

IOCommand* SCSIDirectAccessDevice::writeSectorsAsync(const Int8* /*p*/,UInt32 /*sector*/,UInt32 /*numSectors*/)
{
	Panic("SCSIDirectAccessDevice::writeSectorsAsync() unimplemented!\n");
	return nil;
}

SCSICommand* SCSIDirectAccessDevice::read10(Int8* p,UInt32 sector,UInt32 numSectors)
{
	SCSI_MSG("SCSIDirectAccessDevice::read10\n");
	SCSICommand*	cmd = SCSICommandFactory::read10(this,p,sector,numSectors,sectorSize());
	commandAsync(cmd);
	return cmd;
}

SCSICommand* SCSIDirectAccessDevice::readCapacity(UInt32** size,UInt32** num)
{
	SCSI_MSG("SCSIDirectAccessDevice::readCapacity\n");
	SCSICommand* cmd = SCSICommandFactory::readCapacity(this,size,num);
	commandAsync(cmd);
	return cmd;
}

SCSICommand* SCSIDirectAccessDevice::startStopUnit(Boolean immediate,Boolean loadEject,Boolean start)
{
	SCSI_MSG("SCSIDirectAccessDevice::startStopUnit\n");
	SCSICommand*	cmd = SCSICommandFactory::startStopUnit(this,immediate,loadEject,start);
	commandAsync(cmd);
	return cmd;
}

SCSICommand* SCSIDirectAccessDevice::preventAllowMediumRemoval(Boolean allow)
{
	SCSI_MSG("SCSIDirectAccessDevice::preventAllowMediumRemoval\n");
	SCSICommand* cmd = SCSICommandFactory::preventAllowMediumRemoval(this,allow);
	commandAsync(cmd);
	return cmd;
}

SCSICDRomDevice::SCSICDRomDevice(UInt32 bus,UInt32 deviceID,UInt32 unit,SCSIDriver* _driver):
	SCSIDevice(_driver,bus,deviceID,unit)
{
}

SCSICDRomDevice::~SCSICDRomDevice()
{
}

UInt32 SCSICDRomDevice::bus()
{
	return SCSIDevice::bus();
}

UInt32 SCSICDRomDevice::deviceID()
{
	return SCSIDevice::deviceID();
}

UInt32 SCSICDRomDevice::unit()
{
	return SCSIDevice::unit();
}

UInt32 SCSICDRomDevice::sectorSize()
{
	return 2048;
}

UInt32 SCSICDRomDevice::maxSectorTransfer()
{
	return 65535;
}

Boolean SCSICDRomDevice::initialize()
{
	if(!SCSIDevice::initialize())
		return false;
	
	/// This actually ejects the CD Rom on an 8100, doesn't seem to on my 7500 though.
	SCSICommand*	cmd;
	cmd = startStopUnit(true,true,false);
	CurrThread::blockForIO(cmd);
	delete cmd;
	
	return false;
}

IOCommand* SCSICDRomDevice::readSectorsAsync(Int8* /*p*/,UInt32 /*sector*/,UInt32 /*numSectors*/)
{
	Panic("SCSICDRomDevice::readSectorsAsync() unimplemented!\n");
	return nil;
}

IOCommand* SCSICDRomDevice::writeSectorsAsync(const Int8* /*p*/,UInt32 /*sector*/,UInt32 /*numSectors*/)
{
	Panic("SCSICDRomDevice::writeSectorsAsync() unimplemented!\n");
	return nil;
}

SCSICommand* SCSICDRomDevice::startStopUnit(Boolean immediate,Boolean loadEject,Boolean start)
{
	SCSI_MSG("SCSICDRomDevice::startStopUnit\n");
	SCSICommand*	cmd = SCSICommandFactory::startStopUnit(this,immediate,loadEject,start);
	commandAsync(cmd);
	return cmd;
}

UInt32 SCSICommand::ioError()
{
	return error;
}

SCSIBus::SCSIBus(SCSIDriver* driver,UInt32 busID)
{
	_busID = busID;
	_next = nil;
	
	for(Int32 i=0;i<7;i++)
		device[i] = new SCSIDevice(driver,busID,i,0);
	
	SCSIBus*	bus = machine.scsiBusses;
	SCSIBus*	prev = nil;
	while(bus)
	{
		prev = bus;
		bus = bus->_next;
	}
	if(prev)
		prev->_next = this;
	else
		machine.scsiBusses = this;
}

UInt32 SCSIBus::bus()
{
	return _busID;
}

SCSIBus* SCSIBus::next()
{
	return _next;
}

ConstASCII8Str senseKeyDescriptions[] =	{	"No Sense",
									"Recovered Error",
									"Not Ready",
									"Medium Error",
									"Hardware Error",
									"Illegal Request",
									"Unit Attention",
									"Data Protect",
									"Blank Check",
									"Vendor-Specific",
									"Copy Aborted",
									"Aborted Command",
									"Equal",
									"Volume Overflow",
									"Miscompare",
									"Reserved"
								};

ConstASCII8Str sense_code0[] =	{	"No additional sense information",
								"Filemark detected",
								"End-of-partition/medium detected",
								"Setmark detected",
								"Beginning-of-partition/medium detected",
								"End-of-data detected",
								"I/O process terminated",
								"???",
								"???",
								"???",
								"???",
								"???",
								"???",
								"???",
								"???",
								"???",
								"???",
								"Audio play operation in progress",
								"Audio play operator paused",
								"Audio play operation succesfully completed",
								"Audio play operation stopped due to error",
								"No current audio status to return"
							};
ConstASCII8Str sense_code1[] =	{	"No index/sector signal"
							};
ConstASCII8Str sense_code2[] =	{	"No seek complete"
							};
ConstASCII8Str sense_code3[] =	{	"Peripheral device write fault",
								"No write current",
								"Excessive write errors"
							};
ConstASCII8Str sense_code4[] =	{	"Logical unit not ready, cause not reportable",
								"Logical unit is in process of becoming ready",
								"Logical unit not ready, initializing command required",
								"Logical unit not ready, manual intervention required",
								"Logical unit not ready, format in progress"
							};
ConstASCII8Str sense_code5[] =	{	"Logical unit does not respond to selection"
							};
ConstASCII8Str sense_code6[] =	{	"No reference postion found"
							};
ConstASCII8Str sense_code7[] =	{	"Multiple peripheral devices selected"
							};
ConstASCII8Str sense_code8[] =	{	"Logical unit communication failure",
								"Logical unit communication time-out",
								"Logical unit communication parity error"
							};
ConstASCII8Str sense_code9[] =	{	"Track following error",
								"Tracking servo failure",
								"Focus servo failure",
								"Spindle servo failure"
							};
ConstASCII8Str sense_codeA[] =	{	"Error log overflow"
							};
ConstASCII8Str sense_codeB[] =	{	"???"
							};
ConstASCII8Str sense_codeC[] =	{	"Write error",
								"Write error recovered with auto reallocation",
								"Write error - auto reallocation failed"
							};
ConstASCII8Str sense_codeD[] =	{	"???"
							};
ConstASCII8Str sense_codeE[] =	{	"???"
							};
ConstASCII8Str sense_codeF[] =	{	"???"
							};
ConstASCII8Str sense_code10[] =	{	"ID CRC or ECC error"
							};
ConstASCII8Str sense_code11[] =	{	"Unrecovered read error",
								"Read retries exhausted",
								"Error too long to correct",
								"Multiple read errors",
								"Unrecovered read error - auto reallocate failed",
								"L-EC uncorrectable error",
								"CIRC unrecovered error",
								"Data resynchronization error",
								"Incomplete block read",
								"No gap found",
								"Miscorrected error",
								"Unrecovered read error - recommend reassignment",
								"Unrecovered read error - recommed rewrite the data"
							};
ConstASCII8Str sense_code12[] =	{	"Address mark not found for ID field"
							};
ConstASCII8Str sense_code13[] =	{	"Address mark not found for data field"
							};
ConstASCII8Str sense_code14[] =	{	"Recorded entity not found",
								"Record not found",
								"Filemark or setmark found",
								"End-of-data not found",
								"Block sequence error"
							};
ConstASCII8Str sense_code15[] =	{	"Random positioning error",
								"Mechanical positioning error",
								"Positioning error detected by read of medium"
							};
ConstASCII8Str sense_code16[] =	{	"Data synchronization mark error"
							};
ConstASCII8Str sense_code17[] =	{	"Recovered data with no error correction applied",
								"Recovered data with retries",
								"Recovered data with positive head offset",
								"Recovered data with negative head offset",
								"Recovered data with retries and/or CIRC applied",
								"Recovered data using previous sector ID",
								"Recovered data without ECC - data auto-reallocated",
								"Recovered data without ECC - recommend reassignment",
								"Recovered data without ECC - recommend rewrite"
							};
ConstASCII8Str sense_code18[] =	{	"Recovered data with error correction applied",
								"Recovered data with error correction & retries applied",
								"Recovered data - data auto-reallocated",
								"Recovered data with CIRC",
								"Recovered data with L-EC",
								"Recovered data - recommend reassignment",
								"Recovered data - recommend rewrite"
							};
ConstASCII8Str sense_code19[] =	{	"Defect list error",
								"Defect list not available",
								"Defect list error in primary list",
								"Defect list error in grown list"
							};
ConstASCII8Str sense_code1A[] =	{	"Parameter list length error"
							};
ConstASCII8Str sense_code1B[] =	{	"Synchronous data transfer error"
							};
ConstASCII8Str sense_code1C[] =	{	"Defect list not found",
								"Primary defect list not found",
								"Grown defect list not found"
							};
ConstASCII8Str sense_code1D[] =	{	"Miscompare during verify operation"
							};
ConstASCII8Str sense_code1E[] =	{	"Recovered ID with ECC correction"
							};
ConstASCII8Str sense_code1F[] =	{	"???"
							};
ConstASCII8Str sense_code20[] =	{	"Invalid command operation code"
							};
ConstASCII8Str sense_code21[] =	{	"Logical block address out of range",
								"Invalid element address"
							};
ConstASCII8Str sense_code22[] =	{	"Illegal function (should use 20 00, 24 00, or 26 00)"
							};
ConstASCII8Str sense_code23[] =	{	"???"
							};
ConstASCII8Str sense_code24[] =	{	"Invalid field in CDB"
							};
ConstASCII8Str sense_code25[] =	{	"Logical unit not supported"
							};
ConstASCII8Str sense_code26[] =	{	"Invalid field in parameter list",
								"Parameter not supported",
								"Parameter value invalide",
								"Threshold parameters not supported"
							};
ConstASCII8Str sense_code27[] =	{	"Write protected"
							};
ConstASCII8Str sense_code28[] =	{	"Not ready to ready transition, medium may have changed",
								"Import or export element accessed"
							};
ConstASCII8Str sense_code29[] =	{	"Power on, reset, or bus device reset occurred"
							};
ConstASCII8Str sense_code2A[] =	{	"Parameters changed",
								"Mode parameters changed",
								"Log parameters changed"
							};
ConstASCII8Str sense_code2B[] =	{	"Copy cannot execute since host cannot disconnect"
							};
ConstASCII8Str sense_code2C[] =	{	"Command sequence error",
								"Too many windows specified",
								"Invalid combination of windows specified"
							};
ConstASCII8Str sense_code2D[] =	{	"Overwrite error on update in place"
							};
ConstASCII8Str sense_code2E[] =	{	"???"
							};
ConstASCII8Str sense_code2F[] =	{	"Commands cleared by another initiator"
							};
ConstASCII8Str sense_code30[] =	{	"Incompatible medium installed",
								"Cannot read medium - unknown format",
								"Cannot read medium - incompatible format",
								"Cleaning cartridge installed"
							};
ConstASCII8Str sense_code31[] =	{	"Medium format corrupted",
								"Format command failed"
							};
ConstASCII8Str sense_code32[] =	{	"No defect spare location available",
								"Defect list update failure"
							};
ConstASCII8Str sense_code33[] =	{	"Tape length error"
							};
ConstASCII8Str sense_code34[] =	{	"???"
							};
ConstASCII8Str sense_code35[] =	{	"???"
							};
ConstASCII8Str sense_code36[] =	{	"Ribbon, ink, or toner failure"
							};
ConstASCII8Str sense_code37[] =	{	"Rounded parameter"
							};
ConstASCII8Str sense_code38[] =	{	"???"
							};
ConstASCII8Str sense_code39[] =	{	"Saving parameters not supported"
							};
ConstASCII8Str sense_code3A[] =	{	"Medium not present"
							};
ConstASCII8Str sense_code3B[] =	{	"Sequential positioning error",
								"Tape position error at beginning-of-medium",
								"Tape position error at end-of-medium",
								"Tape or electronic vertical forms unit not ready",
								"Slew failure",
								"Paper jam",
								"Failed to sense top-of-form",
								"Failed to sense bottom-of-form",
								"Reposition error",
								"Read past end of medium",
								"Read past beginning of medium",
								"Position past end of medium",
								"Position past beginning of medium",
								"Medium destination element full",
								"Medium source element empty"
							};
ConstASCII8Str sense_code3C[] =	{	"???"
							};
ConstASCII8Str sense_code3D[] =	{	"Invalid bits in identify message"
							};
ConstASCII8Str sense_code3E[] =	{	"Logical unit has not self-configured yet"
							};
ConstASCII8Str sense_code3F[] =	{	"Target operating conditions have changed",
								"Microcode has been changed",
								"Changed operating definition",
								"Inquiry data has changed"
							};
ConstASCII8Str sense_code40[] =	{	"RAM failure (should use 40 NN)",
								"Diagnostic failure on component NN"	// 0x80 - 0xFF
							};
ConstASCII8Str sense_code41[] =	{	"Data path failure (should use 40 nn)"
							};
ConstASCII8Str sense_code42[] =	{	"Power-on or self-test failure (should use 40 NN)"
							};
ConstASCII8Str sense_code43[] =	{	"Message error"
							};
ConstASCII8Str sense_code44[] =	{	"Internal target failure"
							};
ConstASCII8Str sense_code45[] =	{	"Select or reselect failure"
							};
ConstASCII8Str sense_code46[] =	{	"Unsuccesful soft reset"
							};
ConstASCII8Str sense_code47[] =	{	"SCSI parity error"
							};
ConstASCII8Str sense_code48[] =	{	"Initiator detected error message received"
							};
ConstASCII8Str sense_code49[] =	{	"Invalid message error"
							};
ConstASCII8Str sense_code4A[] =	{	"Command phase error"
							};
ConstASCII8Str sense_code4B[] =	{	"Data phase error"
							};
ConstASCII8Str sense_code4C[] =	{	"Logical unit failed self-configuration"
							};
ConstASCII8Str sense_code4D[] =	{	"???"
							};
ConstASCII8Str sense_code4E[] =	{	"Overlapped commands attempted"
							};
ConstASCII8Str sense_code4F[] =	{	"???"
							};
ConstASCII8Str sense_code50[] =	{	"Write append error",
								"Write append position error",
								"Position error related to timing"
							};
ConstASCII8Str sense_code51[] =	{	"Erase failure"
							};
ConstASCII8Str sense_code52[] =	{	"Cartridge fault"
							};
ConstASCII8Str sense_code53[] =	{	"Media load or eject failed",
								"Unload tape failure",
								"Medium removal prevented"
							};
ConstASCII8Str sense_code54[] =	{	"SCSI to host system interface failure"
							};
ConstASCII8Str sense_code55[] =	{	"System resource failure"
							};
ConstASCII8Str sense_code56[] =	{	"???"
							};
ConstASCII8Str sense_code57[] =	{	"Unable to recover table-of-contents"
							};
ConstASCII8Str sense_code58[] =	{	"Generation does not exist"
							};
ConstASCII8Str sense_code59[] =	{	"Updated block read"
							};
ConstASCII8Str sense_code5A[] =	{	"Operator request or state change input (unspecified)",
								"Operator medium removal request",
								"Operator selected write protect",
								"Operator selected write permit"
							};
ConstASCII8Str sense_code5B[] =	{	"Log exception",
								"Threshold condition met",
								"Log counter at maximum",
								"Log list codes exhausted"
							};
ConstASCII8Str sense_code5C[] =	{	"RPL status change",
								"Spindles synchronized",
								"Spindles not synchronized"
							};
ConstASCII8Str sense_code5D[] =	{	"???"
							};
ConstASCII8Str sense_code5E[] =	{	"???"
							};
ConstASCII8Str sense_code5F[] =	{	"???"
							};
ConstASCII8Str sense_code60[] =	{	"Lamp failure"
							};
ConstASCII8Str sense_code61[] =	{	"Video acquisition error",
								"Unable to acquire video",
								"Out of focus"
							};
ConstASCII8Str sense_code62[] =	{	"Scan head positioning error"
							};
ConstASCII8Str sense_code63[] =	{	"End of user area encountered on this track"
							};
ConstASCII8Str sense_code64[] =	{	"Illegal mode for this track"
							};
						
ConstASCII8Str* sense_codes[]	=	{	sense_code0,sense_code1,sense_code2,sense_code3,sense_code4,sense_code5,sense_code6,sense_code7,
									sense_code8,sense_code9,sense_codeA,sense_codeB,sense_codeC,sense_codeD,sense_codeE,sense_codeF,
									sense_code10,sense_code11,sense_code12,sense_code13,sense_code14,sense_code15,sense_code16,sense_code17,
									sense_code18,sense_code19,sense_code1A,sense_code1B,sense_code1C,sense_code1D,sense_code1E,sense_code1F,
									sense_code20,sense_code21,sense_code22,sense_code23,sense_code24,sense_code25,sense_code26,sense_code27,
									sense_code28,sense_code29,sense_code2A,sense_code2B,sense_code2C,sense_code2D,sense_code2E,sense_code2F,
									sense_code30,sense_code31,sense_code32,sense_code33,sense_code34,sense_code35,sense_code36,sense_code37,
									sense_code38,sense_code39,sense_code3A,sense_code3B,sense_code3C,sense_code3D,sense_code3E,sense_code3F,
									sense_code40,sense_code41,sense_code42,sense_code43,sense_code44,sense_code45,sense_code46,sense_code47,
									sense_code48,sense_code49,sense_code4A,sense_code4B,sense_code4C,sense_code4D,sense_code4E,sense_code4F,
									sense_code50,sense_code51,sense_code52,sense_code53,sense_code54,sense_code55,sense_code56,sense_code57,
									sense_code58,sense_code59,sense_code5A,sense_code5B,sense_code5C,sense_code5D,sense_code5E,sense_code5F,
									sense_code60,sense_code61,sense_code62,sense_code63,sense_code64
								};

static void PrintSense(scsi_sense_data* data,UInt32 scsiID)
{
	cout << "\nSense Data for SCSI ID " << scsiID << ":\n";
	UInt32* hexData = (UInt32*)data;
	cout << hexData[0] << " " << hexData[1] << " " << hexData[2] << " " << hexData[3] << " " << hexData[4] << "\n";
	
	if(data->error_code != 0x70 && data->error_code != 0x71)
		cout << "Invalid Error Code\n";
	else
	{
		cout << "Key: " << senseKeyDescriptions[data->sense_key] << "\n";
		cout << "Error: " << sense_codes[data->asc][data->ascq] << "\n";
	}
	cout << "\n";
}
