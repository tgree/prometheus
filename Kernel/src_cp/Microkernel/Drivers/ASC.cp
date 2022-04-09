/*
	ASC.cp
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
	scsi_53C94_hdw.c		Mach DR2.1 update 6		Alessandro Forin	Originally based on this source.
	mesh.c				Mach DR2.1 update 6		???				Originally based driver structure on this source, rather than
															the scripts-based source in scsi_53C94_hdw.c.  However our
															SCSI classes have changed this significantly
															
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Monday, 15 June 98	-	Fixed _nasty_ bug with chained i/o where the scsiDisconnect() from the previous
											command would overlap the beginning of the next command, cancelling it before it
											barely started.
	Terry Greeniaus	-	Friday, 19 June 98	-	Removed _dcbi from readResidualDMAIn as it is no longer required.
*/
#include "Macros.h"
#include "ASC.h"
#include "SCSI Device.h"
#include "SCSI Driver.h"
#include "NKAtomicOperations.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "NKProcesses.h"
#include "Time.h"
#include "Assembly.h"
#include "DMA.h"
#include "Kernel Console.h"
#include "External Interrupt.h"
#include "Chip Debugger.h"

#define	ASC_DISP_STATUS	0
#if		ASC_DISP_STATUS
#define	ASC_MSG(str) cout << str
#else
#define	ASC_MSG(str) do {} while(0)
#endif

#define	readback(a)	do {} while(0);
//{ register int foo;  _eieio(); foo = (a);}

struct ASCDriver		:	public SCSIDriver,
						public InterruptHandler
{
	volatile	asc_curio_regmap*	regs;
	DMADriver*				dmaDriver;
	
	Int32					clk;
	UInt8					nondma_cnfg3;
	UInt8					dma_cnfg3;
	Boolean					freeingBus;
	
	UInt32					chipType;
	UInt32					ccf;
	UInt32					timeout;
	UInt32					min_period;
	UInt32					scsi_phase;
	
	ASCDriver(ConstASCII8Str driverName,MachineDevice<asc_curio_regmap>* device);
	
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
	virtual	void	handleInterrupt(void);
};

static RegisterDescriptor	asc53C94RegisterDescriptor[]	=	{	CHIP_REGISTER(asc_curio_regmap,asc_tc_lsb,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_tc_msb,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_fifo,REG_SIDE_EFFECTS),
													CHIP_REGISTER(asc_curio_regmap,asc_cmd,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_csr,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_dest_id,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_intr,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_timeout,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_ss,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_sync_tp,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_flags,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_sync_offset,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg1,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_ccf,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_test,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg2,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg3,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg4,REG_NOFLAGS),
													LAST_REGISTER
												};
static RegisterDescriptor	asc53CF94RegisterDescriptor[] =	{	CHIP_REGISTER(asc_curio_regmap,asc_tc_lsb,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_tc_msb,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_fifo,REG_SIDE_EFFECTS),
													CHIP_REGISTER(asc_curio_regmap,asc_cmd,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_csr,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_dest_id,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_intr,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_timeout,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_ss,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_sync_tp,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_flags,REG_READ_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_sync_offset,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg1,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_ccf,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_test,REG_WRITE_ONLY),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg2,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg3,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_cnfg4,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,asc_tc_lsb,REG_NOFLAGS),
													CHIP_REGISTER(asc_curio_regmap,fas_tc_hi,REG_NOFLAGS),
													LAST_REGISTER
												};

void InitASC(void)
{
	SCSIBus*	theSCSIBus;
	if(machine.fastASCDevice.physicalAddr)
	{
		// There's only a 53CF94 on an 8100 so no need to worry about it - it's always the internal bus
		ASCDriver*	driver = new ASCDriver("53CF94",&machine.fastASCDevice);
		machine.driverList.enqueue(driver);
		theSCSIBus = new SCSIBus(driver,internalSCSIBus);
	}
	if(machine.slowASCDevice.physicalAddr)
	{
		// There's a slow 53C94 on many many machines, running the external bus.  It has the following functions:
		//
		//	8100				-	external bus
		//	7100				-	internal (unified) bus
		//	6100				-	internal (unified) bus
		//	7200				-	internal (unified) bus
		//	PCI Machines (not 7200)	-	external bus
		//
		// So, we have to make sure we get the bus right here.
		ASCDriver*	driver = new ASCDriver("53C94",&machine.slowASCDevice);
		machine.driverList.enqueue(driver);
		if(machine.machineClass == classPDM)
		        theSCSIBus = new SCSIBus(driver,(machine.kernelMachineType == machine8100 ? externalSCSIBus : internalSCSIBus));
		else
		        theSCSIBus = new SCSIBus(driver,(machine.kernelMachineType == machine7200 ? internalSCSIBus : externalSCSIBus));
	}
}

ASCDriver::ASCDriver(ConstASCII8Str driverName,MachineDevice<asc_curio_regmap>* device):
	SCSIDriver(driverName),
	InterruptHandler(device->interrupts[0])
{
	switch(device->otherInfo)
	{
		case fastASC:
			regs = device->logicalAddr = (asc_curio_regmap*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
			
			// For the Chip Debugger
			new Chip("53cf94",asc53CF94RegisterDescriptor,(void*)regs);
			
			clk = 40;
			ccf = FAS_CCF_40MHz;
			min_period = ASC_MIN_PERIOD_40;
			chipType = ASC_NCR_53CF94;
		break;
		case slowASC:
			regs = device->logicalAddr = (asc_curio_regmap*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
			
			// For the Chip Debugger
			new Chip("53c94",asc53C94RegisterDescriptor,(void*)regs);
			
			clk = 20;
			ccf = ASC_CCF_20MHz;
			min_period = ASC_MIN_PERIOD_20;
			chipType = ASC_NCR_53C94;
		break;
	}
	timeout = asc_timeout_250(clk,ccf);
	
	dmaDriver = DMADriver::newDMADriver(device->interrupts[1],(device->otherInfo == slowASC ? "53C94 DMA Driver" : "53CF94 DMA Driver"));
}

void ASCDriver::initialize()
{
	dmaDriver->initialize();
	reset();
}

void ASCDriver::start()
{
	dmaDriver->start();
	enable();
}

void ASCDriver::stop()
{
	disable();
	dmaDriver->stop();
}

void ASCDriver::handleInterrupt()
{
	Int32	ss;
	Int32	ir;
	Int32	status;
	
	status = regs->asc_csr;
	_eieio();
again:
	ss = regs->asc_ss;
	_eieio();
	
	// Make sure there really is an interrupt
	if((status & ASC_CSR_INT) == 0)
		return;
	
	ir = regs->asc_intr;
	_eieio();
	
	if(status & ASC_CSR_PE)
		scsiError(scsiParityErr);
	
	if(status & ASC_CSR_GE && !(ir & ASC_INT_BS))
		scsiError(scsiUnknownErr);
	
	if(ir & ASC_INT_DISC)
	{
		// The 53C94 generates a disconnect interrupt for every operation.  It is only an error
		// if it happens when we aren't trying to free the bus, though!
		scsi_phase = busFreePhase;
		if(freeingBus)
		{
			freeingBus = false;
			scsiReady();
		}
		else
			scsiError(scsiDisconnectErr);
	}
	
	if(ir & ASC_INT_RESET)
		scsiError(scsiResetErr);
	
	if(ir & ASC_INT_ILL)
		scsiError(scsiUnknownErr);
	
	if(ir & ASC_INT_RESEL)
		scsiError(scsiReselectionErr);
	
	if(ir & (ASC_INT_SEL | ASC_INT_SEL_ATN))
		scsiError(scsiDontSelectMeErr);
	
	// Check interrupt states
	if(ir & (ASC_INT_FC | ASC_INT_BS))
	{
		if(scsi_phase != (status & 0x07))
		{
			scsi_phase = (status & 0x07);
			scsiPhaseChanged(scsi_phase);
		}
		else
			scsiReady();
	}
	
done:
	// Finish up
	_eieio();
	ir = regs->asc_csr;
	_eieio();
	while(ir != (status = regs->asc_csr))
	{
		ir = status;
		_eieio();
	}
	
	if(status & ASC_CSR_INT)
		goto again;
}

void ASCDriver::flushFifo()
{
	if(regs->asc_flags & ASC_FLAGS_FIFO_CNT)
	{
		regs->asc_cmd = ASC_CMD_FLUSH;
		_eieio();
		readback(regs->asc_cmd);
	}
}

void ASCDriver::loadFifo(const Int8* data,UInt32 len)
{
	Assert(len <= 16);
	
	for(Int32 i=0;i< len;i++)
	{
		regs->asc_fifo = data[i];
		_eieio();
	}
}

void ASCDriver::reset()
{
	// Turn off interrupts, and assign ourselves a CPU id
	regs->asc_cnfg1 = scsiID() | ASC_CNFG1_SLOW | ASC_CNFG1_SRD;
	_eieio();
	
	// Reset the bus
	regs->asc_cmd = ASC_CMD_BUS_RESET;
	readback(regs->asc_cmd);
	_eieio();
	//Wait_us(25);
	
	// Reset the ASC chip
	regs->asc_cmd = ASC_CMD_RESET;
	readback(regs->asc_cmd);
	_eieio();
	//Wait_us(25);
	regs->asc_cmd = ASC_CMD_NOP;
	readback(regs->asc_cmd);
	_eieio();
	//Wait_us(25);
	
	// Turn off interrupts, and assign ourselves a CPU id (again)
	regs->asc_cnfg1 = scsiID() | ASC_CNFG1_SLOW | ASC_CNFG1_P_CHECK;
	_eieio();
	
	// Set up the input frequency range of the device
	regs->asc_ccf = (ccf & 0x07);
	_eieio();
	//Wait_us(25);
	
	// Set up the timeout for how long to wait for a target to respond to (re)selection
	regs->asc_timeout = timeout;
	_eieio();
	
	switch(chipType)
	{
		case ASC_NCR_53C94:
			// Enable SCSI-2 features
			regs->asc_cnfg2 = ASC_CNFG2_SCSI2;
			_eieio();
			if(machine.machineClass == classPCI)
			{
				// Last byte of an odd tranfer will come through the FIFO
				nondma_cnfg3 = dma_cnfg3 = ASC_CNFG3_SRB;
			}
			else
			{
				// Last byte of an odd, non-dma transfer will come through the FIFO
				nondma_cnfg3 = ASC_CNFG3_SRB;
				// Last byte of an odd tranfer comes through FIFO, burst size is 8 bytes, and some other crap
				dma_cnfg3 = ASC_CNFG3_T8 | ASC_CNFG3_ALT_DMA | ASC_CNFG3_SRB;
			}
		break;
		case ASC_NCR_53CF94:
			// Enable SCSI-2 and some other stuff
			regs->asc_cnfg2 = FAS_CNFG2_FEATURES | ASC_CNFG2_SCSI2;
			_eieio();
			nondma_cnfg3 = ASC_CNFG3_SRB | FAS_CNFG3_FASTSCSI | FAS_CNFG3_FASTCLK;
			dma_cnfg3 = ASC_CNFG3_ALT_DMA | ASC_CNFG3_SRB | ASC_CNFG3_T8 | FAS_CNFG3_FASTSCSI | FAS_CNFG3_FASTCLK;
			regs->asc_cnfg4 = FSC_CNFG4_EAN;
			_eieio();
			regs->fas_tc_hi = 0;
			_eieio();
		break;
		default:
			cout << "Ooops, unknown chip type!!!\n";
		break;
	}
	
	// Set up data transfer info
	regs->asc_cnfg3 = nondma_cnfg3;
	_eieio();
	
	// Zero the transfer register
	ASC_TC_PUT(this,0);
	
	// Set up the sync transfer period - number of clocks that each byte takes to go over the bus in synchronous transfer mode.
	regs->asc_sync_tp = min_period;
	_eieio();
	
	// Number of bytes that can be sent over the bus without an ACK/REQ handshake.  Why 0??
	regs->asc_sync_offset = 0;
	_eieio();
}

void ASCDriver::arbitrate()
{
	ASC_MSG("ASCDriver::arbitrate()\n");
	flushFifo();
	scsi_phase = busFreePhase;
	freeingBus = false;
	scsiReady();
}

void ASCDriver::selection(UInt32 targetID)
{
	ASC_MSG("ASCDriver::selection()\n");
	regs->asc_dest_id = targetID;
	readback(regs->asc_dest_id);
	_eieio();
	regs->asc_timeout = timeout;
	readback(regs->asc_timeout);
	_eieio();
	
	scsiReady();
}

void ASCDriver::messageOut(const Int8* data,UInt32 len,Boolean)
{
	ASC_MSG("ASCDriver::messageOut\n");
	
	loadFifo(data,len);
	
	scsiReady();
}

void ASCDriver::requestMessage()
{
	ASC_MSG("ASCDriver::requestMessage()\n");
}

Int8 ASCDriver::messageIn()
{
	ASC_MSG("ASCDriver::messageIn()\n");
	
	while(!(regs->asc_flags & ASC_FLAGS_FIFO_CNT))
		_eieio();
	
	Int8 retVal = regs->asc_fifo;
	_eieio();
	
	return retVal;
}

void ASCDriver::acceptMessage()
{
	regs->asc_cmd = ASC_CMD_MSG_ACPT;
	readback(regs->asc_cmd);
	_eieio();
}

void ASCDriver::commandOut(const Int8* data,UInt32 len)
{
	ASC_MSG("ASCDriver::commandOut(), " << len << " bytes\n");
	
	loadFifo(data,len);
	
	ASC_TC_PUT(this,0);
	readback(regs->asc_cmd);
	
	regs->asc_cmd = ASC_CMD_SEL_ATN;
	readback(regs->asc_cmd);
	_eieio();
}

void ASCDriver::dataOut(const Int8* /*data*/,UInt32 /*len*/)
{
	Panic("ASCDriver::dataOut not implemented!\n");
}

void ASCDriver::dmaOut(const Int8* /*data*/,UInt32 /*len*/)
{
	Panic("ASCDriver::dmaOut not implemented!\n");
}

void ASCDriver::dataIn(Int8* /*data*/,UInt32 /*len*/)
{
	Panic("ASCDriver::dataIn not implemented!\n");
}

void ASCDriver::dmaIn(Int8* data,UInt32 len)
{
	ASC_MSG("ASCDriver::dmaIn" << len << " bytes to " << (UInt32)data << "\n");
	Assert(((UInt32)data & 0x00000007) == 0);
	
	len = (len > 65536 ? 65536 : len);
	
	flushFifo();
	
	dmaDriver->prepareDMAPhysical(data,len,DMA_READ);
	dmaDriver->startDMA(65536);
	
	regs->asc_cnfg1 = scsiID() | ASC_CNFG1_P_CHECK;
	_eieio();
	regs->asc_cnfg3 = dma_cnfg3;
	_eieio();
	ASC_TC_PUT(this,len);
	
	regs->asc_cmd = ASC_CMD_XFER_INFO | ASC_CMD_DMA;
	readback(regs->asc_cmd);
	_eieio();
}

UInt32 ASCDriver::getDataTransferLen()
{
	dmaDriver->stopDMA();
	
	UInt32 transferLen;
	ASC_TC_GET(this,transferLen);						// Get the number of remaining bytes
	
	regs->asc_cnfg1 = scsiID() | ASC_CNFG1_SLOW | ASC_CNFG1_P_CHECK;
	_eieio();
	regs->asc_cnfg3 = nondma_cnfg3;
	_eieio();
	
	return transferLen;
}

UInt32 ASCDriver::readResidualDMAIn(Int8* data)
{
	UInt32 count = regs->asc_flags & ASC_FLAGS_FIFO_CNT;
	Assert(count < 2);
	
	if(count)
	{
		ASC_MSG("ASCDriver::readResidualDMAIn - " << count << " bytes\n");
		data -= count;
		
		UInt32 residual = count;
		while(residual--)
		{
			*data++ = regs->asc_fifo;
			_eieio();
		}
		_dcbf(data);
	}
	
	return count;
}

void ASCDriver::requestStatus()
{
	ASC_MSG("ASCDriver::requestStatus()\n");
	flushFifo();
	regs->asc_cmd = ASC_CMD_I_COMPLETE;
	readback(regs->asc_cmd);
	_eieio();
	scsi_phase = msgInPhase;
	scsiReady();
}

UInt8 ASCDriver::status()
{
	ASC_MSG("ASCDriver::status()\n");
	
	// Wait for a byte to arrive (this is what the Wait_ms(1) trick did to make it work)...
	while((regs->asc_flags & ASC_FLAGS_FIFO_CNT) < 2)
		_eieio();
	
	UInt8	statusByte = regs->asc_fifo;
	_eieio();
	
	return statusByte;
}

void ASCDriver::freeBus()
{
	// If a disconnect error already occurred, the bus is already free.  If it hasn't, we wait for that interrupt
	// to happen, and the scsiReady() call will be made in handleInterrupt().
	if(scsi_phase == busFreePhase)
		scsiReady();
	else
		freeingBus = true;
}
