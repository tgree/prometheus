/*
	Serial Driver.cp
	Copyright © 1998 by Terry Greeniaus, based heavily on work by Alessandro Forin

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
	serial_io.c			Mach DR2.1 update 6		Alessandro Forin	Heavily based on this source (however that is
															likely to change when I re-write the driver).
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "Types.h"
#include "Assembly.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "Serial Driver.h"
#include "External Interrupt.h"
#include "Driver.h"

#define DEFAULT_SERIAL_PORT	modemDriver
#define DEFAULT_MODEM_SPEED	14400
#define DEFAULT_PRINTER_SPEED	9600

enum
{
	PRINTER_PORT		=	0,	// SCC Channel B
	MODEM_PORT		=	1	// SCC Channel A
};

// Register bits
#define	SCC_RR0_TX_EMPTY	0x04	// xmit buffer empty
#define	SCC_RESET_HIGHEST_IUS	0x38	// channel A only

// Control codes
#define	SCC_RESET_EXT_IP			0x10
#define	SCC_IE_NEXT_CHAR			0x20
#define	SCC_RESET_TX_IP			0x28
#define	SCC_RESET_ERROR			0x30
#define	SCC_RESET_TXURUN_LATCH	0xC0
#define	SCC_RR0_RX_AVAIL			0x01	// recv fifo not empty
#define	SCC_RR0_TX_UNDERRUN		0x40	// xmit buffer empty/end of message
#define	SCC_RR1_FRAME_ERR		0x40	// ..bad frame
#define	SCC_RR1_RX_OVERRUN		0x20	// rcv fifo overflow
#define	SCC_RR1_PARITY_ERR		0x10	// incorrect parity in data
#define	SCC_RR2_B_XMIT_DONE		0x00
#define	SCC_RR2_B_EXT_STATUS	0x02
#define	SCC_RR2_B_RECV_DONE		0x04
#define	SCC_RR2_B_RECV_SPECIAL	0x06
#define	SCC_RR2_A_XMIT_DONE		0x08
#define	SCC_RR2_A_EXT_STATUS	0x0A
#define	SCC_RR2_A_RECV_DONE		0x0C
#define	SCC_RR2_A_RECV_SPECIAL	0x0E
#define	SCC_WR1_EXT_IE			0x01
#define	SCC_WR1_TX_IE			0x02
#define	SCC_WR1_RXI_ALL_CHAR		0x10	// on each char, or special
#define	SCC_WR3_RX_ENABLE		0x01
#define	SCC_WR3_RX_8_BITS		0xC0
#define	SCC_WR4_PARITY_ENABLE	0x01
#define	SCC_WR4_EVEN_PARITY		0x02
#define	SCC_WR4_1_STOP			0x04
#define	SCC_WR4_CLK_x16			0x40
#define	SCC_WR4_CLK_x32			0x80
#define	SCC_WR5_RTS				0x02	// drive RTS pin
#define	SCC_WR5_TX_ENABLE		0x08
#define	SCC_WR5_TX_8_BITS		0x60
#define	SCC_WR5_DTR				0x80	// drive DTR pin
#define	SCC_WR9_MASTER_IE		0x08
#define	SCC_WR9_NV				0x02	// no vector
#define	SCC_WR9_RESET_CHA_A		0x80
#define	SCC_WR9_RESET_CHA_B		0x40
#define	SCC_WR10_8BIT_SYNCH		0x00
#define	SCC_WR11_RCLK_BAUDR		0x40	// .. on BRG
#define	SCC_WR11_XTLK_BAUDR		0x10
#define	SCC_WR14_BAUDR_ENABLE	0x01

// Flags
#define	TF_ODDP				0x00000002	// get/send odd parity
#define	TF_EVENP				0x00000004	// get/send even parity
#define	TF_LITOUT			0x00000008	// output all 8 bits

// Other stuff
#define	SCC_DATA_OFFSET		4

#define	SERIAL_CLOCK_FREQUENCY (115200*2)										// Power Mac value
#define	convert_baud_rate(rate)	((((SERIAL_CLOCK_FREQUENCY) + (rate)) / (2 * (rate))) - 2)

struct SCCInterruptHandler	:	public InterruptHandler
{
	UInt32	chan;
	SCCInterruptHandler(UInt32 _chan):InterruptHandler((_chan == 0) ? PMAC_DEV_SCC_B : PMAC_DEV_SCC_A) {chan = _chan;}
	
	virtual	void	handleInterrupt(void);
};

struct SCCDriver	:	public IOStream,
					public Driver
{
	volatile const Int8*		writeData;
	volatile UInt32			writeLen;
	volatile Int8*			readData;
	volatile UInt32			readLen;
	SCCInterruptHandler		*interruptHandler;
	UInt32				chan;
	UInt8				wr[16];
	
	SCCDriver(UInt32 port);
	virtual ~SCCDriver();
	
	// Stuff for driver
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// My stuff
	virtual	void	write_str(ConstASCII8Str str);
	virtual	void	read(Int8* data,UInt32 len);
	virtual	void	write(const Int8* data,UInt32 len);
	
	// Private stuff to control the SCC
			void			write_reg(UInt8 reg,UInt8 val);
			UInt8		read_reg(UInt8 reg);
			UInt8		read_reg_zero(void);
			
			void			write_data(UInt8 val);
			UInt8		read_data(void);
			
			void			setParams(UInt32 baudRate);
			void			set_timing_base(UInt16 val);
};

//static SCCDriver	modemDriver(MODEM_PORT);
//static SCCDriver	printerDriver(PRINTER_PORT);
//IOStream&		serial = DEFAULT_SERIAL_PORT;
static ConstASCII8Str		sccDriverName[] = {"SCC Printer Port Driver","SCC Modem Port Driver"};
static SCCDriver*			modemDriver;
static SCCDriver*			printerDriver;
IOStreamWrapper<IOStream>	serial("serial");

void InitSerial(void)
{
	// Initialize the serial driver.  Must initialize serial port 0 (printer) first!!!
	switch(machine.machineClass)
	{
		case classPCI:
		case classPDM:
		case classPowerBookPCI:
		case classG3:
			cout << "\nModem Port will be running at " << DEFAULT_MODEM_SPEED << " bps today\n";
			cout << "Printer Port will be running at " << DEFAULT_PRINTER_SPEED << " bps today\n";
			printerDriver = new SCCDriver(PRINTER_PORT);
			modemDriver = new SCCDriver(MODEM_PORT);
			serial.set(DEFAULT_SERIAL_PORT);
			machine.driverList.enqueue(modemDriver);
			if(machine.machineClass != classPDM)		// PDM uses only one interrupt, so we don't enable it twice
				machine.driverList.enqueue(printerDriver);
		break;
		case classPowerBook:
		case classPerforma:
		default:
			cout << "Don't have a serial driver for your machine!\n";
		break;
	}
}

SCCDriver::SCCDriver(UInt32 port):
	Driver(sccDriverName[port])
{
	chan = port;
	
	if( !((machine.machineClass != classPCI) && (chan != MODEM_PORT)) )
		interruptHandler = new SCCInterruptHandler( chan );
	else
		interruptHandler = nil;
	
	machine.sccDevice.logicalAddr = (volatile UInt8*)NKIOMap(machine.sccDevice.physicalAddr,machine.sccDevice.len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	cout << "Initializing serial port " << chan << "\n";
	
	readLen = writeLen = 0;
}

SCCDriver::~SCCDriver()
{
}

void SCCDriver::initialize()
{
	// Reset the channel
	write_reg(9,SCC_WR9_RESET_CHA_A | SCC_WR9_RESET_CHA_B);
	
	write_reg(2,0);			// Interrupt vector high nibble = 0
	write_reg(10,SCC_WR10_8BIT_SYNCH);
}

void SCCDriver::start()
{
	if(interruptHandler)
		interruptHandler->enable();
	setParams((chan == MODEM_PORT) ? DEFAULT_MODEM_SPEED : DEFAULT_PRINTER_SPEED);
}

void SCCDriver::stop()
{
	write_reg(9,wr[9] & ~SCC_WR9_MASTER_IE);
	if(interruptHandler)
		interruptHandler->disable();
}

void SCCDriver::write_str(ConstASCII8Str str)
{
	while(*str)
	{
		write(str,1);
		if(*str++ == '\n')
			write("\r",1);
	}
}

void SCCDriver::write(const Int8* data,UInt32 len)
{
	if(!len)
		return;
	
	// What for previous transmissions to complete.  This happens asynchronously
	while(writeLen > 0)
		;
	
	UInt64 timeOut = 10*len + GetTime_ms();	// Wait 10 milliseconds for each character.  That should be plenty for a timeout test.
	
	writeData = data;
	writeLen = len;
	write_reg(1,wr[1] | SCC_WR1_TX_IE);	// Enable interrupts on completion of transmission
	
	// We need a first character transmitted to generate the transmission complete
	// interrupt that starts the transfer.
	write_data(*data);
	
	// Wait for the write to complete
	while(writeLen > 0 && GetTime_ms() < timeOut)
		;
	
	if(writeLen > 0)
	{
		//cout << "Serial output timeout\n";
		writeLen = 0;
	}
	
	len = 0;
}

void SCCDriver::read(Int8* data,UInt32 len)
{
	readData = data;
	readLen = len;
	while(readLen)
		;
	
	return;
}

void SCCDriver::write_reg(UInt8 reg,UInt8 val)
{
	wr[reg] = val;
	machine.sccDevice.logicalAddr[(chan<<1)] = reg;
	_eieio();
	machine.sccDevice.logicalAddr[(chan<<1)] = val;
	_eieio();
}

UInt8 SCCDriver::read_reg(UInt8 reg)
{
	machine.sccDevice.logicalAddr[(chan<<1)] = reg;
	_eieio();
	volatile UInt8 retVal = machine.sccDevice.logicalAddr[(chan<<1)];
	_eieio();
	
	return retVal;
}

UInt8 SCCDriver::read_reg_zero(void)
{
	volatile UInt8 retVal = machine.sccDevice.logicalAddr[(/*chan*/0<<1)];
	_eieio();
	return retVal;
}

void SCCDriver::write_data(UInt8 val)
{
	machine.sccDevice.logicalAddr[(chan<<1)+SCC_DATA_OFFSET] = val;
	_eieio();
}

UInt8 SCCDriver::read_data(void)
{
	volatile UInt8 retVal = machine.sccDevice.logicalAddr[(chan<<1)+SCC_DATA_OFFSET];
	_eieio();
	return retVal;
}

void SCCDriver::setParams(UInt32 baudRate)
{
	// If baudRate is zero, disable port
	if(baudRate == 0)
	{
		write_reg(5,(wr[5] & ~SCC_WR5_DTR));
		return;
	}
	
	UInt8 value = SCC_WR4_1_STOP;
	value |= (baudRate == 115200) ? SCC_WR4_CLK_x32 : SCC_WR4_CLK_x16;
	/*
	if( (flags & (TF_ODDP | TF_EVENP)) == TF_EVENP)
		value |= (SCC_WR4_EVEN_PARITY | SCC_WR4_PARITY_ENABLE);
	else if( (flags & (TF_ODDP | TF_EVENP)) == TF_ODDP)
		value |= SCC_WR4_PARITY_ENABLE;
	*/
	
	// Set Parity and Stop bits
	write_reg(4,value);
	
	// Set up for 8 bits
	write_reg(3,SCC_WR3_RX_8_BITS);
	
	// Set DTR, RTS, and transmitter bits/character
	write_reg(5,(SCC_WR5_TX_8_BITS | SCC_WR5_RTS | SCC_WR5_DTR));
	
	// Disable baud rate
	write_reg(14,0);
	
	UInt16 speed_value = convert_baud_rate(baudRate);
	speed_value = (speed_value == 0xFFFF) ? 0 : speed_value;
	
	set_timing_base(speed_value);
	
	if(baudRate == 115200 || baudRate == 230400)
	{
		// Change clock rate source / disable baud rate generator
		write_reg(11,0);
	}
	else
	{
		write_reg(11,(SCC_WR11_RCLK_BAUDR | SCC_WR11_XTLK_BAUDR));
		write_reg(14,SCC_WR14_BAUDR_ENABLE);	// Enable baud rate generator
	}
	
	write_reg(3,SCC_WR3_RX_8_BITS | SCC_WR3_RX_ENABLE);
	write_reg(1,SCC_WR1_RXI_ALL_CHAR | SCC_WR1_EXT_IE);
	write_reg(15,0);
	
	// Clear out any pending external or status interrupts
	write_reg(0,SCC_RESET_EXT_IP);
	write_reg(0,SCC_RESET_EXT_IP);
	write_reg(0,SCC_IE_NEXT_CHAR);
	
	// Enable SCC interrupts
	write_reg(9,SCC_WR9_MASTER_IE | SCC_WR9_NV);
	
	read_reg_zero();	// Clear the status
	
	write_reg(1,SCC_WR1_RXI_ALL_CHAR | SCC_WR1_EXT_IE);
	write_reg(5,wr[5] | SCC_WR5_TX_ENABLE);
}

void SCCDriver::set_timing_base(UInt16 val)
{
	write_reg(12,(val & 0x00FF));
	write_reg(13,((val >> 8) & 0x00FF));
}

void SCCInterruptHandler::handleInterrupt(void)
{
	SCCDriver*	driver;
	
	UInt8 status = printerDriver->read_reg_zero();	// Read reg zero is the same for both modem & printer
	UInt8 rr2 = (printerDriver->read_reg(2) & 0x0E);
	if(rr2 == SCC_RR2_A_XMIT_DONE || rr2 == SCC_RR2_B_XMIT_DONE)
	{
		driver = (rr2 == SCC_RR2_A_XMIT_DONE) ? modemDriver : printerDriver;
		modemDriver->write_reg(0,SCC_RESET_TX_IP);
		if(--driver->writeLen > 0)
			driver->write_data(*driver->writeData++);
		else
			driver->write_reg(1,(driver->wr[1] & ~SCC_WR1_TX_IE));
	}
	else if(rr2 == SCC_RR2_A_RECV_DONE || rr2 == SCC_RR2_B_RECV_DONE)
	{
		driver = (rr2 == SCC_RR2_A_RECV_DONE) ? modemDriver : printerDriver;
		modemDriver->write_reg(0,SCC_RESET_HIGHEST_IUS);
		if(driver->readLen)
		{
			*driver->readData++ = driver->read_data();
			driver->readLen--;
		}
		else
			driver->read_data();
	}
	else if(rr2 == SCC_RR2_A_EXT_STATUS || rr2 == SCC_RR2_B_EXT_STATUS)
	{
		driver = (rr2 == SCC_RR2_A_EXT_STATUS) ? modemDriver : printerDriver;
		status = driver->read_reg(0);
		if(status & SCC_RR0_TX_UNDERRUN)
		{
			driver->write_reg(0,SCC_RESET_TXURUN_LATCH);
			cout << "          Serial Interrupt: Transmission underrun\n";
		}
		driver->write_reg(0,SCC_RESET_EXT_IP);
		//driver->write_reg(0,SCC_RESET_HIGHEST_IUS);
		modemDriver->write_reg(0,SCC_RESET_HIGHEST_IUS);
		driver->write_reg(0,SCC_RESET_ERROR);
	}
	else if(rr2 == SCC_RR2_A_RECV_SPECIAL || rr2 == SCC_RR2_B_RECV_SPECIAL)
	{
		driver = (rr2 == SCC_RR2_A_RECV_SPECIAL) ? modemDriver : printerDriver;
		UInt8 rr1 = driver->read_reg(1);
		if(rr1 & (SCC_RR1_PARITY_ERR | SCC_RR1_RX_OVERRUN | SCC_RR1_FRAME_ERR))
		{
			driver->write_reg(0,SCC_RESET_ERROR);
			cout << "          Serial Interrupt: Serial error\n";
		}
		//driver->write_reg(0,SCC_RESET_HIGHEST_IUS);
		modemDriver->write_reg(0,SCC_RESET_HIGHEST_IUS);
	}
}
