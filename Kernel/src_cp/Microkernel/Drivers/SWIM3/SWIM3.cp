#include "SWIM3.h"
#include "NKMachineInit.h"
#include "Chip Debugger.h"
#include "NKVirtualMemory.h"
#include "Kernel Console.h"
#include "NKThreads.h"
#include "Macros.h"

static RegisterDescriptor	swim3RegisterDescriptor[] =	{	CHIP_REGISTER(SWIM3Regs,data,REG_SIDE_EFFECTS),
												CHIP_REGISTER(SWIM3Regs,timer,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,error,REG_READ_ONLY),
												CHIP_REGISTER(SWIM3Regs,mode,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,select,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,setup,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,control,REG_READ_ONLY),
												CHIP_REGISTER(SWIM3Regs,control_bic,REG_WRITE_ONLY),
												CHIP_REGISTER(SWIM3Regs,control_bis,REG_WRITE_ONLY),
												CHIP_REGISTER(SWIM3Regs,status,REG_READ_ONLY),
												CHIP_REGISTER(SWIM3Regs,intr,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,nseek,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,ctrack,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,csect,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,gap3,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,sector,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,nsect,REG_NOFLAGS),
												CHIP_REGISTER(SWIM3Regs,intr_enable,REG_NOFLAGS),
												LAST_REGISTER
											};

SWIM3Device* swim3Device;
void InitSWIM3()
{
	if(machine.swim3.physicalAddr && machine.machineClass != classPDM)
	{
		cout << "Building SWIM3Driver\n";
		SWIM3Driver* swim3Driver = new SWIM3Driver(&machine.swim3);
		cout << "Building SWIM3Device\n";
		swim3Device = new SWIM3Device(swim3Driver);
		cout << "Enqueuing swim3Driver\n";
		machine.driverList.enqueue(swim3Driver);
		cout << "Done\n";
	}
}

void InitSWIM3Volumes()
{
	if(swim3Device)
	{
		cout << greenMsg << "SWIM3 Bus Present\n" << whiteMsg;
		if(swim3Device->probe())
		{
			BlockDeviceManager::buildPartitionList(swim3Device);
			swim3Device->buildFileSystems();
		}
		else
			cout << redMsg << "\t\tBut Down\n" << whiteMsg;
	}
}

SWIM3Driver::SWIM3Driver(MachineDevice<SWIM3Regs>* device):
	IOCommandDriver("SWIM3"),
	InterruptHandler(device->interrupts[0])
{
	// Map the SWIM3 registers
	regs = device->logicalAddr = (SWIM3Regs*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// Make the DMA driver
	dmaDriver = DMADriver::newDMADriver(device->interrupts[1],"SWIM3 DMA Driver");
	
	// For the chip debugger
	new Chip("SWIM 3",swim3RegisterDescriptor,regs);
	
	diskInserted = false;
}

SWIM3Driver::~SWIM3Driver()
{
}

void SWIM3Driver::initialize()
{
	WriteUReg8(S_SW_RESET,&regs->setup);	// Reset the controller
	Wait_us(1);
	if(ReadUReg8(&regs->error))
		nkVideo << redMsg << "Error resetting SWIM3 controller\n" << whiteMsg;
	
	WriteUReg8(0x95,&regs->mode);		// What does this do?
	WriteUReg8(0xFF,&regs->control_bic);	// Clear control register
	WriteUReg8(S_IBM_DRIVE | S_FCLK_DIV2,&regs->setup);
	Wait_us(10);
	
	// See if the drive is present
	/*
	if(!readBit(DRIVE_PRESENT))
	{
		nkVideo << "SWIM3: No drive present\n";
		return;
	}
	*/
	
	WriteUReg8(0x00,&regs->intr_enable);	// Disable all interrupts
}

void SWIM3Driver::start()
{
	enable();
	WriteUReg8(INTR_ENABLE,&regs->control_bis);
}

void SWIM3Driver::stop()
{
	WriteUReg8(INTR_ENABLE,&regs->control_bic);
	disable();
}

Boolean SWIM3Driver::probe()
{
	Boolean	success = true;
	
	WriteUReg8(DRIVE_ENABLE/* | INTR_ENABLE*/,&regs->control_bis);	// Enable the drive
	action(MOTOR_ON);	// Turn on the motor and wait for the seek to complete
	for(UInt32 i=0;i<100;i++)
	{
		if(readBit(SEEK_COMPLETE))
			break;
		Wait_ms(10);
	}
	if(!readBit(SEEK_COMPLETE))	// Seek timed out, possibly no disk or no drive
		success = false;
	if(!readBit(DISK_IN))	// No disk
		success = false;
	
	action(SETMFM);
	
	if(!success)
	{
		action(MOTOR_OFF);	// Turn off the motor
		WriteUReg8(DRIVE_ENABLE/* | INTR_ENABLE*/,&regs->control_bic);	// Disable the drive
		Wait_ms(1);
		WriteUReg8(S_SW_RESET,&regs->setup);
	}
	
	diskInserted = success;
	return success;
}

void SWIM3Driver::startAsyncIO(IOCommand* cmd)
{
	if(cmd)
	{
		currCommand = static_cast<SWIM3Command*>(cmd);
		switch(currCommand->cmdType)
		{
			case swim3Eject:
				currCommand->cmdState = swim3Ejecting;
				action(EJECT);
				while(!readBit(RELAX))
					CurrThread::sleepNS(ONE_SEC_NANOSECONDS);
				currCommand->error = 0;
				currCommand->cmdState = swim3Done;
				currCommand->doneIO();
				startAsyncIO(dequeue());
			break;
			case swim3Read:
				locate();
			break;
		}
	}
}

void SWIM3Driver::locate()
{
	// Figure out what track we are on
	if(readBit(TRACK_ZERO))
	{
		if(currCommand->reqTrack == 0)
			transfer();
		else
			seek();
		return;
	}
	
	currCommand->cmdState = swim3Locating;
	select(READ_DATA_0);
	ReadUReg8(&regs->intr);
	WriteUReg8(DO_ACTION,&regs->control_bis);
	WriteUReg8(ERROR_INTR | SEEN_SECTOR,&regs->intr_enable);
}

void SWIM3Driver::seek()
{
	UInt32	currTrack = (readBit(TRACK_ZERO) ? 0 : ReadUReg8(&regs->ctrack));
	Int32	deltaTrack = (Int32)currCommand->reqTrack - (Int32)currTrack;
	if(deltaTrack == 0)
	{
		transfer();
		return;
	}
	currCommand->cmdState = swim3Seeking;
	if(deltaTrack > 0)
		action(SEEK_POSITIVE);
	else
	{
		action(SEEK_NEGATIVE);
		deltaTrack = -deltaTrack;
	}
	WriteUReg8(deltaTrack,&regs->nseek);
	select(STEP);
	WriteUReg8(DO_SEEK,&regs->control_bis);
	WriteUReg8(ERROR_INTR | SEEK_DONE,&regs->intr_enable);
}

void SWIM3Driver::transfer()
{
	currCommand->cmdState = swim3Transferring;
	UInt32	nsectors = 18 - currCommand->reqSector + 1;
	UInt32	maxsectors = nsectors;
	if(nsectors > currCommand->numSectors)
		nsectors = currCommand->numSectors;
	if(currCommand->startSector % 36 < 18)
		select(READ_DATA_0);
	else
		select(READ_DATA_1);
	WriteUReg8(currCommand->reqSector,&regs->sector);
	WriteUReg8(nsectors,&regs->nsect);
	WriteUReg8(0,&regs->gap3);
	currCommand->sectCount = nsectors;
	
	dmaDriver->prepareDMALogical(currCommand->dataPtr,nsectors*512,DMA_READ,currCommand->processID);
	dmaDriver->startDMA(maxsectors*512);
	
	WriteUReg8(DO_ACTION,&regs->control_bis);
	WriteUReg8(ERROR_INTR | TRANSFER_DONE,&regs->intr_enable);
}

void SWIM3Driver::handleInterrupt()
{
	UInt32	err = ReadUReg8(&regs->error);
	UInt32	intr = ReadUReg8(&regs->intr);
	UInt32	ctrack = ReadUReg8(&regs->ctrack);
	UInt32	csect = ReadUReg8(&regs->csect);
	
	if(intr & ERROR_INTR)
	{
		if(err & ERR_DATA_CRC)
			dout << "SWIM3::handleInterrupt() - data crc error\n";
		if(err & ERR_ADDR_CRC)
			dout << "SWIM3::handleInterrupt() - address crc error\n";
		if(err & ERR_OVERRUN)
			dout << "SWIM3::handleInterrupt() - overrun error\n";
		if(err & ERR_UNDERRUN)
			dout << "SWIM3::handleInterrupt() - underrun error\n";
	}
	
	switch(currCommand->cmdState)
	{
		case swim3Locating:
			if(intr & SEEN_SECTOR)
			{
				WriteUReg8(DO_ACTION,&regs->control_bic);
				WriteUReg8(RELAX,&regs->select);
				WriteUReg8(0,&regs->intr_enable);
				if(ctrack >= 0x80 && !readBit(TRACK_ZERO))
					locate();
				else
					seek();
			}
		break;
		case swim3Seeking:
			if(ReadUReg8(&regs->nseek) == 0)
			{
				// We should be there!
				WriteUReg8(DO_SEEK,&regs->control_bic);
				WriteUReg8(RELAX,&regs->select);
				WriteUReg8(0,&regs->intr_enable);
				select(SEEK_COMPLETE);
				Wait_us(10);
				ReadUReg8(&regs->intr);
				while(ReadUReg8(&regs->status) & DATA)
					;
				locate();
			}
		break;
		case swim3Transferring:
			dmaDriver->stopDMA();
			WriteUReg8(0,&regs->intr_enable);
			WriteUReg8(WRITE_SECTORS | DO_ACTION,&regs->control_bic);
			WriteUReg8(RELAX,&regs->select);
			if(intr & ERROR_INTR)
			{
				currCommand->error = -1;
				currCommand->cmdState = swim3Done;
				currCommand->doneIO();
			}
			else
			{
				currCommand->numSectors -= currCommand->sectCount;
				if(currCommand->numSectors)
				{
					currCommand->dataPtr += 512*currCommand->sectCount;
					currCommand->startSector += currCommand->sectCount;
					currCommand->reqTrack = currCommand->startSector / 36;
					currCommand->reqSector = (currCommand->startSector%18) + 1;
					locate();
				}
				else
				{
					currCommand->error = 0;
					currCommand->cmdState = swim3Done;
					currCommand->doneIO();
				}
			}
		break;
	}
	
	startAsyncIO(dequeue());
}

void SWIM3Driver::select(UInt8 sel)
{
	WriteUReg8(RELAX,&regs->select);
	if(sel & 8)
		WriteUReg8(SELECT,&regs->control_bis);
	else
		WriteUReg8(SELECT,&regs->control_bic);
	WriteUReg8(sel & CA_MASK,&regs->select);
}

void SWIM3Driver::action(UInt8 act)
{
	select(act);
	Wait_us(1);
	WriteUReg8((ReadUReg8(&regs->select) | LSTRB),&regs->select);
	Wait_us(2);
	WriteUReg8((ReadUReg8(&regs->select) & ~LSTRB),&regs->select);
	Wait_us(1);
	WriteUReg8(RELAX,&regs->select);
}

Boolean SWIM3Driver::readBit(UInt8 bit)
{
	select(bit);
	Wait_us(10);
	UInt8 retVal = ReadUReg8(&regs->status);
	WriteUReg8(RELAX,&regs->select);
	return ((retVal & DATA) == 0);
}

void SWIM3Driver::command(SWIM3Command* cmd)
{
	// Invalidate all parts of the DMA transfer
	UInt32		startInval;
	UInt32		endInval;
	if(cmd->cmdType == swim3Read)
	{
		startInval = ROUND_UP(32,(UInt32)cmd->dataPtr);
		endInval = ROUND_DOWN(32,(UInt32)cmd->dataPtr + (cmd->numSectors*512));
		
		for(UInt32 i=startInval;i<endInval;i += 32)
			_dcbi((void*)i);
	}
	
	enqueue(cmd);
}

SWIM3Device::SWIM3Device(SWIM3Driver* _driver)
{
	driver = _driver;
}

SWIM3Device::~SWIM3Device()
{
}

Boolean SWIM3Device::probe()
{
	return driver->probe();
}

UInt32 SWIM3Device::bus()
{
	return floppyBus;
}

UInt32 SWIM3Device::deviceID()
{
	return 0;
}

UInt32 SWIM3Device::unit()
{
	return 0;
}

UInt32 SWIM3Device::sectorSize()
{
	return 512;
}

UInt32 SWIM3Device::maxSectorTransfer()
{
	return 18;
}

IOCommand* SWIM3Device::readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors)
{
	SWIM3Command*	cmd = new SWIM3Command;
	cmd->cmdType = swim3Read;
	cmd->cmdState = swim3Starting;
	cmd->dataPtr = p;
	cmd->startSector = sector;
	cmd->numSectors = numSectors;
	cmd->error = 0;
	cmd->reqTrack = sector/36;
	cmd->reqSector = (sector % 18) + 1;
	driver->command(cmd);
	
	return cmd;
}

IOCommand* SWIM3Device::writeSectorsAsync(const Int8*,UInt32,UInt32)
{
	Panic("Someone called SWIM3Device::writeSectorsAsync");
}

void SWIM3Device::shutDown(Boolean)
{
	IOCommand*	cmd = eject();
	CurrThread::blockForIO(cmd);
	delete cmd;
}

SWIM3Command* SWIM3Device::eject()
{
	SWIM3Command*	cmd = new SWIM3Command;
	cmd->cmdType = swim3Eject;
	cmd->cmdState = swim3Starting;
	cmd->dataPtr = nil;
	cmd->startSector = nil;
	cmd->numSectors = nil;
	cmd->error = 0;
	if(driver->diskInserted == false)
	{
		cmd->cmdState = swim3Done;
		cmd->doneIO();
	}
	else
		driver->command(cmd);
	
	return cmd;
}

UInt32 SWIM3Command::ioError()
{
	return error;
}
