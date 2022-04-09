/*
	Mace.cp
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
	if_mace.c				Mach DR2.1 update 6		???			MACEDriver hierarchy loosely based on this source.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKVideo.h"
#include "Assembly.h"
#include "DMA.h"
#include "Mace.h"
#include "MACE DMA.h"
#include "Kernel Console.h"
#include "External Interrupt.h"
#include "Ethernet Vendor Table.h"
#include "Chip Debugger.h"

struct MACEDriver	:	public Driver,
					public InterruptHandler
{
	volatile UInt8	gotPacket;
	mace_board*	regs;
	DMADriver*	txDMADriver;
	DMADriver*	rxDMADriver;
	
	MACEDriver(MachineDevice<mace_board>* device);
	virtual ~MACEDriver()	{}
	
	// Stuff for Driver
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// Stuff for InterruptHandler
	virtual	void	handleInterrupt();
	
	// Sniffer
			void	sniff();
			void	info();
};

static RegisterDescriptor	maceRegisterDescriptor[]	=	{	CHIP_REGISTER(mace_board,rcvfifo,REG_READ_ONLY | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,xmtfifo,REG_WRITE_ONLY),
												CHIP_REGISTER(mace_board,xmtfc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,xmtfs,REG_READ_ONLY | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,xmtrc,REG_READ_ONLY | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,rcvfc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,rcvfs,REG_READ_ONLY | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,fifofc,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,ir,REG_READ_ONLY | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,imr,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,pr,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,biucc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,fifocc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,maccc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,plscc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,phycc,REG_NOFLAGS),
												CHIP_REGISTER(mace_board,chipid1,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,chipid2,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,iac,REG_NOFLAGS | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,ladrf,REG_NOFLAGS | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,padr,REG_NOFLAGS | REG_SIDE_EFFECTS),
												CHIP_REGISTER(mace_board,mpc,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,rntpc,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,rcvcc,REG_READ_ONLY),
												CHIP_REGISTER(mace_board,utr,REG_NOFLAGS),
												LAST_REGISTER
											};

MACEDriver*	mace;

void InitMace(void)
{
	if(machine.maceDevice.physicalAddr)
	{
		mace = new MACEDriver(&machine.maceDevice);
		machine.driverList.enqueue(mace);
	}
}

void Sniff(void)
{
	mace->sniff();
}

void DumpRegs(void)
{
	mace->info();
}

MACEDriver::MACEDriver(MachineDevice<mace_board>* device):
	Driver("Mace"),
	InterruptHandler(device->interrupts[0])
{
	regs = device->logicalAddr = (mace_board*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	txDMADriver = DMADriver::newDMADriver(PMAC_DMA_ETHERNET_TX,"Ethernet Transmit DMA Driver");
	rxDMADriver = DMADriver::newDMADriver(PMAC_DMA_ETHERNET_RX,"Ethernet Receive DMA Driver");
	
	// For the Chip Debugger
	new Chip("mace",maceRegisterDescriptor,regs);
}

void MACEDriver::initialize()
{
	txDMADriver->initialize();
	rxDMADriver->initialize();
	
	WriteUReg8(ReadUReg8(&regs->fifocc) | FIFOCC_XFWU,&regs->fifocc);
	
	ReadUReg8(&regs->ir);	// Clear out any interrupts
	
	WriteUReg8(BIUCC_SWRST,&regs->biucc);	// Reset board
	
	while(ReadUReg8(&regs->biucc) & BIUCC_SWRST)	// Wait until board is reset
		;
	
	WriteUReg8(0xFF,&regs->imr);	// Disable all interrupts
	
	// Print the hardware address
	cout << greenMsg << "Physical Ethernet Address: ";
	WriteUReg8(ReadUReg8(&regs->iac) | 0x04,&regs->iac);	// PHYADDR bit
	
	UInt64	addr = 0;
	
	for(UInt32 i=0;i<6;i++)
	{
		UInt32 byte = ReadUReg8(&regs->padr);
		addr = ( (addr << 8) | byte );
		cout << byte;
		if(i < 5)
			cout << ".";
	}
	cout << "\n";
	
	ConstASCII8Str	vendorName;
	ConstASCII8Str	vendorInfo;
	SearchEthernetTable(addr >> 24,&vendorName,&vendorInfo);
	if(vendorName)
	{
		cout << "Vendor: " << vendorName << "\n";
		if(vendorInfo)
			cout << "\t" << vendorInfo << "\n";
	}
	cout << whiteMsg;
}

void MACEDriver::start()
{
	txDMADriver->start();
	rxDMADriver->start();
	enable();
}

void MACEDriver::stop()
{
	disable();
	txDMADriver->stop();
	rxDMADriver->stop();
}

void MACEDriver::handleInterrupt()
{
	nkVideo << redMsg << "MACEDriver::handleInterrupt()\n" << whiteMsg;
	
	UInt8	interrupts = ReadUReg8(&regs->ir);	// Get interrupt status, this clears on read
	if(interrupts & 0x80)
		nkVideo << "\tJabber error\n";
	if(interrupts & 0x40)
		nkVideo << "\tBabble error\n";
	if(interrupts & 0x20)
		nkVideo << "\tColllision error\n";
	if(interrupts & 0x10)
		nkVideo << "\tReceive Collision Count Overflow\n";
	if(interrupts & 0x08)
		nkVideo << "\tRunt Packet Count Overflow\n";
	if(interrupts & 0x04)
		nkVideo << "\tMissed Packet Count Overflow\n";
	if(interrupts & 0x02)
		nkVideo << "\tReceive Interrupt\n";
	if(interrupts & 0x01)
		nkVideo << "\tTransmit Interrupt\n";
	
	gotPacket = true;
}

void MACEDriver::sniff()
{
	gotPacket = false;
	
	initialize();
	
	//WriteUReg8(ReadUReg8(&regs->phycc) | 0x40,&regs->phycc);		// Disable link test function
	WriteUReg8(0x06,&regs->plscc);	// Select GPSI port
	WriteUReg8(0x07,&regs->plscc);	// Set ENPLSIO bit
	WriteUReg8(0,&regs->imr);		// Enable all interrupts
	WriteUReg8(0x81,&regs->maccc);	// Turn on PROM and ENRCV bits
	
	//cout << redMsg << "Dumping loop registers\n" << whiteMsg;
	//info();
	
	while(!gotPacket)
		;
	
	WriteUReg8(ReadUReg8(&regs->imr) & 0x02,&regs->imr);		// Turn on RCVINTM bit
	WriteUReg8(ReadUReg8(&regs->maccc) & ~0x81,&regs->maccc);	// Turn off PROM and ENRCV bits
}

void MACEDriver::info()
{
	cout << hexMsg << "XMTFC: " << ReadUReg8(&regs->xmtfc) << "\n";
	cout << "XMTFS:  " << ReadUReg8(&regs->xmtfs) << "\n";
	cout << "XMTRC:  " << ReadUReg8(&regs->xmtrc) << "\n";
	cout << "RCVFC: "  << ReadUReg8(&regs->rcvfc) << "\n";
	cout << "RCVFS: "  << ReadUReg8(&regs->rcvfs) << "\n";
	cout << "RNTPC: "  << ReadUReg8(&regs->rntpc) << "\n";
	cout << "RCVCC: "  << ReadUReg8(&regs->rcvcc) << "\n";
	cout << "FIFOFC: " << ReadUReg8(&regs->fifofc) << "\n";
	cout << "IR:     " << ReadUReg8(&regs->ir) << "\n";
	cout << "IMR:    " << ReadUReg8(&regs->imr) << "\n";
	cout << "PR:     " << ReadUReg8(&regs->pr) << "\n";
	cout << "BIUCC:  " << ReadUReg8(&regs->biucc) << "\n";
	cout << "FIFOCC: " << ReadUReg8(&regs->fifocc) << "\n";
	cout << "MACCC:  " << ReadUReg8(&regs->maccc) << "\n";
	cout << "PLSCC:  " << ReadUReg8(&regs->plscc) << "\n";
	cout << "PHYCC:  " << ReadUReg8(&regs->phycc) << "\n";
	cout << "IAC:    " << ReadUReg8(&regs->iac) << "\n";
	cout << "MPC:    " << ReadUReg8(&regs->mpc) << "\n";
	cout << "RNTPC:  " << ReadUReg8(&regs->rntpc) << "\n";
	cout << "RCVCC:  " << ReadUReg8(&regs->rcvcc) << "\n";
	cout << decMsg;
}