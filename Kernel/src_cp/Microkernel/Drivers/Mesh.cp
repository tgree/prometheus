/*
	Mesh.cp
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
	mesh.c				Mach DR2.1 update 6		???			Originally based on this file, however our SCSI
														classes have changed the functionality significantly.
															
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 19 June 98	-	Removed _dcbi from readResidualDMAIn as it is no longer required.
*/
#include "Macros.h"
#include "Mesh.h"
#include "Time.h"
#include "Kernel Types.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "NKAtomicOperations.h"
#include "NKInterruptVectors.h"
#include "NKDebuggerNub.h"
#include "NKProcesses.h"
#include "Assembly.h"
#include "Kernel Console.h"
#include "SCSI Device.h"
#include "SCSI Driver.h"
#include "DMA.h"
#include "External Interrupt.h"
#include "PCI Machine Registers.h"
#include "Chip Debugger.h"

#define	MESH_DISP_STATUS	0
#if		MESH_DISP_STATUS
#define	MESH_MSG(str)	cout << str
#else
#define	MESH_MSG(str)	do {} while(0)
#endif

static RegisterDescriptor	meshRegisterDescriptor[] =	{	CHIP_REGISTER(mesh_regmap,r_count0,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_count1,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_fifo,REG_SIDE_EFFECTS),
												CHIP_REGISTER(mesh_regmap,r_cmd,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_bus0status,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_bus1status,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_fifo_cnt,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_excpt,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_error,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_intmask,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_interrupt,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_sourceid,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_destid,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_sync,REG_NOFLAGS),
												CHIP_REGISTER(mesh_regmap,r_meshid,REG_READ_ONLY),
												CHIP_REGISTER(mesh_regmap,r_sel_timeout,REG_NOFLAGS),
												LAST_REGISTER
											};

struct MeshDriver	:	public SCSIDriver,
					public InterruptHandler
{
	mesh_regmap*		regs;
	DMADriver*		dmaDriver;
	
	MeshDriver(MachineDevice<mesh_regmap>* device);
	
			void	flushFifo();
			void	loadFifo(const Int8* data,UInt32 len);
	
	// Stuff for Driver
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// Stuff for SCSIDriver
	virtual	void	reset();
	virtual	void	arbitrate();
	virtual	void	selection(UInt32 targetID);
	virtual	void	messageOut(const Int8* data,UInt32 len,Boolean atn);
	virtual	void	requestMessage();
	virtual	Int8	messageIn();
	virtual	void	acceptMessage();
	virtual	void	commandOut(const Int8* data,UInt32 len);
	virtual	void	dataOut(const Int8* data,UInt32 len);
	virtual	void	dmaOut(const Int8* data,UInt32 len);
	virtual	void	dataIn(Int8* data,UInt32 len);
	virtual	void	dmaIn(Int8* data,UInt32 len);
	virtual	UInt32	getDataTransferLen();
	virtual	UInt32	readResidualDMAIn(Int8* data);
	virtual	void		requestStatus();
	virtual	UInt8	status();
	virtual	void		freeBus();
	
	// InterruptHandler stuff
	virtual	void	handleInterrupt();
};

void InitMesh(void)
{
	SCSIBus*	theSCSIBus;
	if(machine.meshDevice.physicalAddr)
	{
		MeshDriver* meshDriver = new MeshDriver(&machine.meshDevice);
		machine.driverList.enqueue((SCSIDriver*)meshDriver);
		theSCSIBus = new SCSIBus(meshDriver,internalSCSIBus);
	}
}

MeshDriver::MeshDriver(MachineDevice<mesh_regmap>* device):
	SCSIDriver("Mesh"),
	InterruptHandler(device->interrupts[0])
{
	// Map the Mesh registers
	regs = device->logicalAddr = (mesh_regmap*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// Create the DMA driver
	dmaDriver = DMADriver::newDMADriver(device->interrupts[1],"Mesh DMA Driver");
	
	// For the chip debugger
	new Chip("mesh",meshRegisterDescriptor,regs);
}

void MeshDriver::initialize()
{
	dmaDriver->initialize();
	reset();
}

void MeshDriver::start()
{
	dmaDriver->start();
	enable();
	
	regs->r_intmask = 0x07;	// Enable all interrupts
	_eieio();
}

void MeshDriver::stop()
{
	regs->r_intmask = 0;	// Disable all interrupts
	_eieio();
	
	disable();
	dmaDriver->stop();
}

void MeshDriver::handleInterrupt()
{
	MESH_MSG("MeshDriver::handleInterrupt()\n");
	UInt8	int_status;
	UInt8	exception;
	UInt8	error;
	UInt8	bus0status;
	UInt8	bus1status;
	
	int_status = regs->r_interrupt;
	_eieio();
	error = regs->r_error;
	_eieio();
	exception = regs->r_excpt;
	_eieio();
	bus0status = regs->r_bus0status;
	_eieio();
	bus1status = regs->r_bus1status;
	_eieio();
	
	if(int_status == 0) 
		return;
	
	/* Clear out interrupts */
	regs->r_interrupt = int_status;
	_eieio();
	
	if(int_status & MESH_INTR_ERROR)
	{
		if(error & MESH_ERR_SCSI_RESET)
			scsiError(scsiResetErr);
		
		if(error & MESH_ERR_DISCONNECT)
			scsiError(scsiDisconnectErr);

		if(error & MESH_ERR_PARITY0)
			scsiError(scsiParityErr);
	}
	else if(int_status & MESH_INTR_EXCPT)
	{
		if(exception & MESH_EXCPT_SELTO)
			scsiError(scsiSelectionTimeoutErr);
		
		if(exception & MESH_EXCPT_ARBLOST)
			scsiError(scsiLostArbErr);
		
		if(exception & MESH_EXCPT_RESEL)
			scsiError(scsiReselectionErr);
		
		if(exception & MESH_EXCPT_PHASE)
			scsiPhaseChanged(bus0status & 0x07);
	}
	else if(int_status & MESH_INTR_DONE)
		scsiReady();
}

void MeshDriver::flushFifo()
{
	if(regs->r_fifo_cnt)
	{
		regs->r_cmd = MESH_CMD_FLUSH_FIFO;
		_eieio();
	}
}

void MeshDriver::loadFifo(const Int8* data,UInt32 len)
{
	Assert(len <= 16);
	
	for(Int32 i=0;i < len;i++)
	{
		regs->r_fifo = data[i];
		_eieio();
	}
}

void MeshDriver::reset()
{
	// Turn off interrupts
	regs->r_intmask = 0;
	_eieio();
	
	// Reset the bus
	regs->r_bus1status = MESH_BUS1_STATUS_RESET;
	_eieio();
	Wait_us(300);
	regs->r_bus1status = 0;
	_eieio();
	
	// Reset the Mesh chip
	regs->r_cmd = MESH_CMD_RESET_MESH;
	_eieio();
	Wait_us(1);
	
	do
	{
		_eieio();
	} while(regs->r_interrupt == 0);
	
	// Clear interrupts
	regs->r_interrupt = 0xFF;
	_eieio();
	
	regs->r_sync = 2;
	_eieio();
	regs->r_sourceid = scsiID();	// SCSI ID [7 is the ID of the CPU]
	_eieio();
	regs->r_sel_timeout = 25;	// Selection timeout
	_eieio();
	regs->r_cmd = MESH_CMD_ENABLE_PARITY;
	_eieio();
}

void MeshDriver::arbitrate()
{
	UInt32 bus0status = regs->r_bus0status;
	_eieio();
	UInt32 bus1status = regs->r_bus1status;
	_eieio();
	MESH_MSG("MeshDriver::arbitrate()\n");
	if(bus0status || bus1status)
	{
		MESH_MSG("Bus status 0 = " << bus0status << ", Bus status 1 = " << bus1status << "\n");
		scsiError(scsiBusDownErr);
	}
	else
	{
		_sync();
		MESH_SET_XFER(regs,0);
		regs->r_destid = scsiID();
		_eieio();
		regs->r_cmd = MESH_CMD_ARBITRATE/* | MESH_SEQ_ACT_NEG*/;
		_eieio();
	}
}

void MeshDriver::selection(UInt32 targetID)
{
	MESH_MSG("MeshDriver::selection()\n");
	flushFifo();
	regs->r_destid = targetID;
	_eieio();
	regs->r_cmd = MESH_CMD_SELECT | MESH_SEQ_ATN;
	_eieio();
}

void MeshDriver::messageOut(const Int8* data,UInt32 len,Boolean atn)
{
	MESH_MSG("MeshDriver::messageOut()\n");
	Assert(len <= 16);
	
	MESH_SET_XFER(regs, len);
	regs->r_cmd = MESH_CMD_MSGOUT | (atn ? MESH_SEQ_ATN : 0);
	_eieio();
	loadFifo(data,len);
}

void MeshDriver::requestMessage()
{
	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_MSGIN;
	_eieio();
}
	
Int8 MeshDriver::messageIn()
{
	MESH_MSG("MeshDriver::messageIn()\n");
	
	while(!regs->r_fifo_cnt)
		_eieio();
	
	Int8 retVal = regs->r_fifo;
	_eieio();
	
	return retVal;
}

void MeshDriver::acceptMessage()
{
	MESH_MSG("MeshDriver::acceptMessage()\n");
}

void MeshDriver::commandOut(const Int8* data,UInt32 len)
{
	MESH_MSG("MeshDriver::commandOut(), " << len << " bytes\n");
	Assert(len <= 16);
	
	flushFifo();
	
	MESH_SET_XFER(regs,len);

	regs->r_cmd = MESH_CMD_COMMAND;
	_eieio();
	loadFifo(data,len);
}

void MeshDriver::dataOut(const Int8* /*data*/,UInt32 /*len*/)
{
	Panic("MeshDriver::dataOut not implemented!\n");
}

void MeshDriver::dmaOut(const Int8* /*data*/,UInt32 /*len*/)
{
	Panic("MeshDriver::dmaOut not implemented!\n");
}

void MeshDriver::dataIn(Int8* /*data*/,UInt32 /*len*/)
{
	Panic("MeshDriver::dataIn not implemented!\n");
}

void MeshDriver::dmaIn(Int8* data,UInt32 len)
{
	MESH_MSG("MeshDriver::dmaIn" << len << " bytes\n");
	Assert(((UInt32)data & 0x00000007) == 0);
	
	flushFifo();
	
	/*
	len = (len > 65536 ? 65536 : len);
	clearDMACommands();
	newDMACommand(DBDMA_CMD_IN_LAST,0,len,data,DBDMA_INT_NEVER,DBDMA_BRANCH_NEVER,DBDMA_WAIT_NEVER);
	newDMACommand(DBDMA_CMD_STOP,0,0,0,DBDMA_INT_NEVER,DBDMA_BRANCH_NEVER,DBDMA_WAIT_NEVER);
	*/
	
	dmaDriver->prepareDMAPhysical(data,len,DMA_READ);
	len = dmaDriver->startDMA(65536);
	
	MESH_SET_XFER(regs,len);
	
	/*
	startDMA();
	*/
	
	regs->r_cmd = MESH_CMD_DATAIN | MESH_SEQ_DMA;
	_eieio();
}

UInt32 MeshDriver::getDataTransferLen()
{
	/*
	endDMA();
	*/
	dmaDriver->stopDMA();
	return MESH_GET_XFER(regs);
}

UInt32 MeshDriver::readResidualDMAIn(Int8* data)
{
	UInt32 count = regs->r_fifo_cnt;
	Assert(count < 2);
	
	if(count)
	{
		data -= count;
		//_dcbi(data);
		UInt32 residual = count;
		while(residual--)
		{
			*data++ = regs->r_fifo;
			_eieio();
		}
		_dcbf(data);
	}
	
	return count;
}

void MeshDriver::requestStatus()
{
	MESH_MSG("MeshDriver::requestStatus()\n");
	MESH_SET_XFER(regs, 1);
	regs->r_cmd = MESH_CMD_STATUS;
	_eieio();
}

UInt8 MeshDriver::status()
{
	MESH_MSG("MeshDriver::status()\n");
	FatalAssert(regs->r_fifo_cnt != 0);
	
	UInt8     statusByte = regs->r_fifo;
	_eieio();
	
	flushFifo();
	
	return statusByte;
}

void MeshDriver::freeBus()
{
	regs->r_cmd = MESH_CMD_BUSFREE;
	_eieio();
}
