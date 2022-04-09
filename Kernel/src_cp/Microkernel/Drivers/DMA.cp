/*
	DMA.cp
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
	dbdma.c				Mach DR2.1 update 6		???			PCIDMADriver based on this source.
	scsi_amic.c			Mach DR2.1 update 6		???			PDMDMADriver based on this source.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98		-	Added GNU license to file
	Terry Greeniaus	-	Wednesday, 27 Oct 99	-	Updated so it can find DBDMA on NewWorld machines
	Terry Greeniaus	-	Friday, 29 Oct 99		-	Fixed problem on beige G3/PowerBook G3 where it thought it was NewWorld
*/
#include "Assembly.h"
#include "PCI Machine Registers.h"
#include "PDM Machine Registers.h"
#include "DMA.h"
#include "NKVirtualMemory.h"
#include "Kernel Console.h"
#include "NKDebuggerNub.h"
#include "NKProcesses.h"
#include "ASC.h"
#include "Macros.h"
#include "NKMachineInit.h"
#include "Chip Debugger.h"

#define NUM_DBDMA_COMMANDS	64

static RegisterDescriptor	dbdmaRegisterDescriptor[]	=	{	CHIP_REGISTER(DBDMARegs,channelControl,REG_WRITE_ONLY | REG_LE),
													CHIP_REGISTER(DBDMARegs,channelStatus,REG_READ_ONLY | REG_LE),
													CHIP_REGISTER(DBDMARegs,commandPtrLo,REG_LE),
													CHIP_REGISTER(DBDMARegs,interruptSelect,REG_LE),
													CHIP_REGISTER(DBDMARegs,branchSelect,REG_LE),
													CHIP_REGISTER(DBDMARegs,waitSelect,REG_LE),
													LAST_REGISTER
												};
static RegisterDescriptor	scsiDMARegisterDescriptor[]	=	{	CHIP_REGISTER(PDMSCSIDMARegs,slowASCBase[0],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCBase[1],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCBase[2],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCBase[3],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCBase[0],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCBase[1],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCBase[2],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCBase[3],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCCtrl,REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCCtrl,REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCPosition[0],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCPosition[1],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCPosition[2],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,slowASCPosition[3],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCPosition[0],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCPosition[1],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCPosition[2],REG_NOFLAGS),
													CHIP_REGISTER(PDMSCSIDMARegs,fastASCPosition[3],REG_NOFLAGS),
													LAST_REGISTER
												};
static RegisterDescriptor	maceDMARegisterDescriptor[]	=	{	CHIP_REGISTER(PDMMACEDMARegs,xmtcs,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,rcvcs,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,rcvhp,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,rcvtp,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,xmtbch_0,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,xmtbcl_0,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,xmtbch_1,REG_NOFLAGS),
													CHIP_REGISTER(PDMMACEDMARegs,xmtbcl_1,REG_NOFLAGS),
													LAST_REGISTER
												};

DMADriver::DMADriver(ConstASCII8Str name):
	Driver(name)
{
	physDest = nil;
	logDest = nil;
	remainBytes = 0;
}

DMADriver* DMADriver::newDMADriver(UInt32 interruptNum,ConstASCII8Str name)
{
	if(machine.machineClass == classPDM)
	{
		nkVideo << "   New DMADriver IRQ " << interruptNum << "\n";
		switch(interruptNum)
		{
			case PMAC_DMA_SCSI0:	// External SCSI Bus
				return new PDMSCSIDMADriver(name,slowASC);
			break;
			case PMAC_DMA_SCSI1:	// Internal SCSI Bus
				return new PDMSCSIDMADriver(name,fastASC);
			break;
			case PMAC_DMA_FLOPPY:
				Panic("Floppy DMA not supported yet!\n");
			break;
			case PMAC_DMA_ETHERNET_TX:
				return new PDMMACEDMATxDriver(name,&machine.maceDMA);
			break;
			case PMAC_DMA_ETHERNET_RX:
				return new PDMMACEDMARxDriver(name,&machine.maceDMA);
			break;
			case PMAC_DMA_SCC_A_TX:
			case PMAC_DMA_SCC_A_RX:
			case PMAC_DMA_SCC_B_TX:
			case PMAC_DMA_SCC_B_RX:
				Panic("Serial DMA not supported yet!\n");
			break;
			case PMAC_DMA_AUDIO_OUT:
			case PMAC_DMA_AUDIO_IN:
				Panic("Audio DMA not supported yet!\n");
			break;
		}
	}
	else if(machine.machineClass != classNewWorld)
	{
		DBDMARegs*	regs = (DBDMARegs*)PCI_IO_PHYS_ADDR(PCI_DMA_BASE + (interruptNum << 8));
		return new DBDMADriver(regs,interruptNum,name);
	}
	else
	{
		DBDMARegs*	regs = (DBDMARegs*)NEWWORLD_IO_PHYS_ADDR(PCI_DMA_BASE + (interruptNum << 8));
		return new DBDMADriver(regs,interruptNum,name);
	}
	return nil;
}

DBDMADriver::DBDMADriver(DBDMARegs* _regs,UInt32 _interruptNum,ConstASCII8Str _name):
	DMADriver(_name),
	InterruptHandler(_interruptNum)
{
	regs = (DBDMARegs*)NKIOMap(_regs,sizeof(DBDMARegs),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// For the Chip Debugger
	new Chip(_name,dbdmaRegisterDescriptor,regs);
	
	cmds = new DBDMACommand[NUM_DBDMA_COMMANDS]; // *** Lock cmds in place in physical memory
	FatalAssert(((UInt32)cmds & 0x0000000F) == 0);	// Make sure it's 16-byte aligned
	cmdsPhys = (DBDMACommand*)NKGetPhysical(cmds,PROCESS_KERNEL);
}

void DBDMADriver::setControlBit(UInt32 bit)
{
	WriteUReg32LE((1UL << (bit + 16)) | (1UL << bit),&regs->channelControl);
}

void DBDMADriver::clearControlBit(UInt32 bit)
{
	WriteUReg32LE(1UL << bit,&regs->channelControl);
}

void DBDMADriver::handleInterrupt()
{
	Panic("DBDMADriver should never have to handle an interrupt!\n");
}

void DBDMADriver::initialize()
{
	// Mask all interrupts, disable all bits, including CS_RUN
	WriteUReg32LE(0xFFFF00FF,&regs->channelControl);
	
	// Wait for it to terminate
	while(ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE))
		;
}

void DBDMADriver::start()
{
	// Don't do anything
	enable();
}

void DBDMADriver::stop()
{
	// Just pause the channel
	setControlBit(CS_PAUSE);
	
	// Wait for it to pause
	while(ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE))
		;
	
	disable();
}

void DBDMADriver::prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction)
{
	// Make sure no transfer is in progress!
	FatalAssert((ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE)) == 0);
	
	DBDMACommand*	cmd = &cmds[0];
	
	logDest = nil;
	physDest = physAddr;
	remainBytes = count;
	
	do
	{
		UInt32 len = (count >= 65536 ? 65520 : count);
		
		WriteUInt16LE(len,&cmd->reqCount);
		cmd->rsrv1 = 0;
		cmd->i = INT_NEVER;
		cmd->b = BRANCH_NEVER;
		cmd->w = WAIT_NEVER;
		cmd->cmd = (direction == DMA_READ ? CMD_INPUT_MORE : CMD_OUTPUT_MORE);
		cmd->rsrv2 = 0;
		cmd->key = KEY_STREAM0;
		WriteUInt32LE((UInt32)physAddr,&cmd->address);
		cmd->cmdDep = 0;
		cmd->resCount = 0;
		cmd->xferStatus = 0;
		
		_dcbf(cmd);
		
		count -= len;
		physAddr += len;
		cmd++;
	}while(count);
	
	cmd--;
	cmd->cmd = (direction == DMA_READ ? CMD_INPUT_LAST : CMD_OUTPUT_LAST);
	_dcbf(cmd);
	cmd++;
	
	cmd->reqCount = 0;
	cmd->rsrv1 = 0;
	cmd->i = INT_NEVER;
	cmd->b = BRANCH_NEVER;
	cmd->w = WAIT_NEVER;
	cmd->cmd = CMD_STOP;
	cmd->rsrv2 = 0;
	cmd->key = 0;
	cmd->address = 0;
	cmd->cmdDep = 0;
	cmd->resCount = 0;
	cmd->xferStatus = 0;
	
	_dcbf(cmd);
}

void DBDMADriver::prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process)
{
	// Make sure no transfer is in progress!
	FatalAssert((ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE)) == 0);
	FatalAssert(count <= 65536);
	
	DBDMACommand*	cmd = &cmds[0];
	
	logDest = logAddr;
	physDest = nil;
	remainBytes = count;
	
	do
	{
		UInt32	len = NKMaxContig(logAddr,process);
		len = (len > count ? count : len);
		len = (len == 65536 ? 32768 : len);
		
		WriteUInt16LE(count,&cmd->reqCount);
		cmd->rsrv1 = 0;
		cmd->i = INT_NEVER;
		cmd->b = BRANCH_NEVER;
		cmd->w = WAIT_NEVER;
		cmd->cmd = (direction == DMA_READ ? CMD_INPUT_MORE : CMD_OUTPUT_MORE);
		cmd->rsrv2 = 0;
		cmd->key = KEY_STREAM0;
		WriteUInt32LE((UInt32)NKGetPhysical(logAddr,process),&cmd->address);	// *** Lock logAddr to logAddr + len bytes in memory here
		cmd->cmdDep = 0;
		cmd->resCount = 0;
		cmd->xferStatus = 0;
		
		count -= len;
		logAddr += len;
		cmd++;
	}while(count);
	
	cmd--;
	cmd->cmd = (direction == DMA_READ ? CMD_INPUT_LAST : CMD_OUTPUT_LAST);
	cmd++;
	
	cmd->reqCount = 0;
	cmd->rsrv1 = 0;
	cmd->i = INT_NEVER;
	cmd->b = BRANCH_NEVER;
	cmd->w = WAIT_NEVER;
	cmd->cmd = CMD_STOP;
	cmd->rsrv2 = 0;
	cmd->key = 0;
	cmd->address = 0;
	cmd->cmdDep = 0;
	cmd->resCount = 0;
	cmd->xferStatus = 0;
}

UInt32 DBDMADriver::startDMA(UInt32 maxTransfer)
{
	// Make sure no transfer is in progress!
	FatalAssert((ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE)) == 0);
	
	// Make sure the channel isn't dead!
	FatalAssert((ReadUReg32LE(&regs->channelStatus) & (1UL << CS_DEAD)) == 0);
	
	// Erase the controller
	WriteUReg32LE(0xFDFF0000,&regs->channelControl);
	
	// Set up the command pointer
	WriteUReg32LE((UInt32)0,&regs->rsrv1);
	WriteUReg32LE((UInt32)cmdsPhys,&regs->commandPtrLo);
	
	// Start the transfer
	setControlBit(CS_RUN);
	
	return (remainBytes > maxTransfer ? maxTransfer : remainBytes);
}

void DBDMADriver::stopDMA()
{
	// See if we stopped early
	/*
	if(ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE))
		cout << "\tDMA stopped early\n";
	*/
	
	// Flush and stop
	WriteUReg32LE((1UL << (CS_FLUSH + 16)) | (1UL << (CS_RUN + 16)) | (1UL << CS_FLUSH) ,&regs->channelControl);
	
	// Wait for termination
	while(ReadUReg32LE(&regs->channelStatus) & (/*(1UL << CS_FLUSH) | */(1UL << CS_ACTIVE)) )
		;
}

void DBDMADriver::waitForDMAEnd()
{
	// Wait for it to finish
	while(ReadUReg32LE(&regs->channelStatus) & (1UL << CS_ACTIVE))
		;
}

PDMSCSIDMADriver::PDMSCSIDMADriver(ConstASCII8Str name,UInt32 unit):
	DMADriver(name),
	InterruptHandler(unit == slowASC ? PMAC_DMA_SCSI0 : PMAC_DMA_SCSI1)
{
	PDMSCSIDMARegs* dmaRegs = (PDMSCSIDMARegs*)NKIOMap((void*)PDM_IO_PHYS_ADDR(PDM_SCSI_DMA_CTRL_BASE),sizeof(PDMSCSIDMARegs),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// For the Chip Debugger
	new Chip(name,scsiDMARegisterDescriptor,dmaRegs);
	
	switch(unit)
	{
		case fastASC:
			base = &dmaRegs->fastASCBase[0];
			ctrl = &dmaRegs->fastASCCtrl;
			rsrv = &dmaRegs->fastRsrv1[0];
			position = &dmaRegs->fastASCPosition[0];
		break;
		case slowASC:
			base = &dmaRegs->slowASCBase[0];
			ctrl = &dmaRegs->slowASCCtrl;
			rsrv = &dmaRegs->slowRsrv1[0];
			position = &dmaRegs->slowASCPosition[0];
		break;
	}
}

void PDMSCSIDMADriver::handleInterrupt()
{
	Panic("PDMSCSIDMADriver::handleInterrupt() shouldn't be called!\n");
}

void PDMSCSIDMADriver::initialize()
{
	// Reset the controller
	WriteUReg8((1 << PDM_SCSI_DMA_CTRL_RESET_BIT),ctrl);
	
	// Wait for it to stop
	while(ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_RUN_BIT))
		;
}

void PDMSCSIDMADriver::start()
{
	// Don't do anything
	//enable();
}

void PDMSCSIDMADriver::stop()
{
	// Kill the controller
	WriteUReg8( (ReadUReg8(ctrl) & ~(1 << PDM_SCSI_DMA_CTRL_RUN_BIT)),ctrl);
	
	// Wait for it to die
	while(ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_RUN_BIT))
		;
}

void PDMSCSIDMADriver::prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 _direction)
{
	// Make sure no transfer is in progress!
	FatalAssert((ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_RUN_BIT)) == 0);
	FatalAssert(((UInt32)physAddr & 0x00000007) == 0);
	
	WriteUReg8( ((UInt32)physAddr >> 24), &base[0] );
	WriteUReg8( ((UInt32)physAddr >> 16), &base[1] );
	WriteUReg8( ((UInt32)physAddr >> 8), &base[2] );
	WriteUReg8( (UInt32)physAddr, &base[3] );
	
	remainBytes = count;
	direction = _direction;
}

void PDMSCSIDMADriver::prepareDMALogical(Ptr,UInt32,UInt32,ProcessID)
{
	Panic("PDMSCSIDMADriver::prepareDMALogical() not implemented yet!\n");
}

UInt32 PDMSCSIDMADriver::startDMA(UInt32 maxTransfer)
{
	// Make sure no transfer is in progress!
	FatalAssert((ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_RUN_BIT)) == 0);
	
	// Start the transfer
	if(direction == DMA_READ)
		WriteUReg8((1 << PDM_SCSI_DMA_CTRL_RUN_BIT),ctrl);
	else
		WriteUReg8((1 << PDM_SCSI_DMA_CTRL_DIR_BIT) | (1 << PDM_SCSI_DMA_CTRL_RUN_BIT),ctrl);
		
	return (remainBytes > maxTransfer ? maxTransfer : remainBytes);
}

void PDMSCSIDMADriver::stopDMA()
{
	if(direction == DMA_READ)
	{
		// Flush
		WriteUReg8((1 << PDM_SCSI_DMA_CTRL_FLUSH_BIT),ctrl);
		
		// Wait for the flush to complete
		while(ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_FLUSH_BIT))
			;
	}
	
	// Stop the transfer
	WriteUReg8( (ReadUReg8(ctrl) & ~(1 << PDM_SCSI_DMA_CTRL_RUN_BIT)),ctrl);
	
	// Wait for it to complete
	while(ReadUReg8(ctrl) & (1 << PDM_SCSI_DMA_CTRL_RUN_BIT))
		;
}

void PDMSCSIDMADriver::waitForDMAEnd()
{
	stopDMA();
	// It just did!
}

PDMMACEDMATxDriver::PDMMACEDMATxDriver(ConstASCII8Str name,MachineDevice<PDMMACEDMARegs>* dev):
	DMADriver(name),
	InterruptHandler(PMAC_DMA_ETHERNET_TX)
{
	if(!dev->logicalAddr)
	{
		regs = dev->logicalAddr = (PDMMACEDMARegs*)NKIOMap(dev->physicalAddr,dev->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
		// For the Chip Debugger
		new Chip("mace dma",maceDMARegisterDescriptor,regs);
	}
	else
		regs = dev->logicalAddr;
}

void PDMMACEDMATxDriver::handleInterrupt()
{
	Panic("PDMMACEDMATxDriver::handleInterrupt() not implemented!\n");
}

void PDMMACEDMATxDriver::initialize()
{
	WriteUReg8(RST,&regs->rcvcs);
}

void PDMMACEDMATxDriver::start()
{
	enable();
}

void PDMMACEDMATxDriver::stop()
{
	disable();
}

void PDMMACEDMATxDriver::prepareDMAPhysical(Ptr /*physAddr*/,UInt32 /*count*/,UInt32 /*direction*/)
{
	Panic("PDMMACEDMATxDriver::prepareDMAPhysical() not implemented!\n");
}

void PDMMACEDMATxDriver::prepareDMALogical(Ptr /*logAddr*/,UInt32 /*count*/,UInt32 direction,ProcessID /*process*/)
{
	FatalAssert(direction == DMA_WRITE);
	Panic("PDMMACEDMATxDriver::prepareDMALogical() not implemented!\n");
}

UInt32 PDMMACEDMATxDriver::startDMA(UInt32 /*maxTransfer*/)
{
	Panic("PDMMACEDMATxDriver::startDMA() not implemented!\n");
	return 0;
}

void PDMMACEDMATxDriver::stopDMA()
{
	Panic("PDMMACEDMATxDriver::stopDMA() not implemented!\n");
}

void PDMMACEDMATxDriver::waitForDMAEnd()
{
	Panic("PDMMACEDMATxDriver::waitForDMAEnd() not implemented!\n");
}

PDMMACEDMARxDriver::PDMMACEDMARxDriver(ConstASCII8Str name,MachineDevice<PDMMACEDMARegs>* dev):
	DMADriver(name),
	InterruptHandler(PMAC_DMA_ETHERNET_RX)
{
	if(!dev->logicalAddr)
	{
		regs = dev->logicalAddr = (PDMMACEDMARegs*)NKIOMap(dev->physicalAddr,dev->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
		// For the Chip Debugger
		new Chip("mace dma",maceDMARegisterDescriptor,regs);
	}
	else
		regs = dev->logicalAddr;
}

void PDMMACEDMARxDriver::handleInterrupt()
{
	Panic("PDMMACEDMATxDriver::handleInterrupt() not implemented!\n");
}

void PDMMACEDMARxDriver::initialize()
{
	WriteUReg8(RST,&regs->rcvcs);
}

void PDMMACEDMARxDriver::start()
{
	enable();
}

void PDMMACEDMARxDriver::stop()
{
	disable();
}

void PDMMACEDMARxDriver::prepareDMAPhysical(Ptr /*physAddr*/,UInt32 /*count*/,UInt32 /*direction*/)
{
	Panic("PDMMACEDMATxDriver::prepareDMAPhysical() not implemented!\n");
}

void PDMMACEDMARxDriver::prepareDMALogical(Ptr /*logAddr*/,UInt32 /*count*/,UInt32 direction,ProcessID /*process*/)
{
	FatalAssert(direction == DMA_READ);
	Panic("PDMMACEDMATxDriver::prepareDMALogical() not implemented!\n");
}

UInt32 PDMMACEDMARxDriver::startDMA(UInt32 /*maxTransfer*/)
{
	Panic("PDMMACEDMATxDriver::startDMA() not implemented!\n");
	return 0;
}

void PDMMACEDMARxDriver::stopDMA()
{
	Panic("PDMMACEDMATxDriver::stopDMA() not implemented!\n");
}

void PDMMACEDMARxDriver::waitForDMAEnd()
{
	Panic("PDMMACEDMATxDriver::waitForDMAEnd() not implemented!\n");
}
