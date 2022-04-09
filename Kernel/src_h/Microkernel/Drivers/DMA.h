/*
 * Copyright 1996 1995 by Open Software Foundation, Inc. 1997 1996 1995 1994 1993 1992 1991  
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	dbdma.h				Mach DR2.1 update 6		???				DBDMA stuff is a direct copy, PCIDMADriver/PDMDMADriver
															stuff is new for Prometheus.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright notices to file
*/
 #ifndef __DMA__
#define __DMA__

#include "NKMachineInit.h"
#include "DBDMA.h"
#include "Driver.h"
#include "External Interrupt.h"

enum
{
	DMA_READ	=	0,
	DMA_WRITE	=	1
};

class DMADriver	:	public Driver
{
protected:
	Ptr		physDest;
	Ptr		logDest;
	UInt32	remainBytes;
	
	DMADriver(ConstASCII8Str name);
public:
	virtual	void		prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction) = 0;	// Sets up a DMA physical transfer.  Use DMA_READ/DMA_WRITE for direction.
	virtual	void		prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process) = 0;		// Sets up a DMA logical transfer
	virtual	UInt32	startDMA(UInt32 maxTransfer) = 0;					// Starts DMA.  maxTransfer is the maximum DMA length that the device can issue
																// without requiring an interrupt.  The returned value indicates the number of bytes
																// that should be read during this interrupt.  A return of 0 indicates the transfer is complete.
	virtual	void		stopDMA() = 0;									// Stops a DMA transfer cold.  It cannot be resumed if this is called.
	virtual	void		waitForDMAEnd() = 0;							// Waits until DMA finishes
	
	static DMADriver*	newDMADriver(UInt32 channel,ConstASCII8Str name = "Unnamed DMA Driver");	// Call this to allocate the correct DMA driver.  Pass the interrupt number (channel) from the OpenFirmware please...
};

class DBDMADriver	:	public DMADriver,
					public InterruptHandler
{
	struct DBDMARegs*		regs;
	struct DBDMACommand*	cmds;
	struct DBDMACommand*	cmdsPhys;
	
	DBDMADriver(DBDMARegs* regs,UInt32 interruptNum,ConstASCII8Str name);
			void		setControlBit(UInt32 bit);
			void		clearControlBit(UInt32 bit);
protected:
	virtual	void		handleInterrupt();
	
	virtual	void		initialize();
	virtual	void		start();
	virtual	void		stop();
public:
	virtual	void		prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction);
	virtual	void		prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process);
	virtual	UInt32	startDMA(UInt32 maxTransfer);
	virtual	void		stopDMA();
	virtual	void		waitForDMAEnd();
	
	friend class DMADriver;
};

class PDMSCSIDMADriver	:	public DMADriver,
						public InterruptHandler
{
	// This driver is only used for PDM SCSI access
	UReg8*	base;
	UReg8*	ctrl;
	UReg8*	rsrv;
	UReg8*	position;
	
	UInt32	direction;
	
	PDMSCSIDMADriver(ConstASCII8Str name,UInt32 unit);
protected:
	virtual	void		handleInterrupt();
	
	virtual	void		initialize();
	virtual	void		start();
	virtual	void		stop();
public:
	virtual	void		prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction);
	virtual	void		prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process);
	virtual	UInt32	startDMA(UInt32 maxTransfer);
	virtual	void		stopDMA();
	virtual	void		waitForDMAEnd();
	
	friend class DMADriver;
};

typedef struct PDMSCSIDMARegs
{
	UReg8	slowASCBase[4];		// The base of the current DMA transfer
	UReg8	fastASCBase[4];
	UReg8	slowASCCtrl;			// The control register
	UReg8	fastASCCtrl;
	UReg8	slowRsrv1[3];
	UReg8	fastRsrv1[3];
	UReg8	slowASCPosition[4];		// The position of the current DMA transfer
	UReg8	fastASCPosition[4];
}PDMSCSIDMARegs;

class PDMMACEDMATxDriver	:	public DMADriver,
							public InterruptHandler
{
	// This driver is only used for PDM MACE transmit access
	struct PDMMACEDMARegs*	regs;
	
	PDMMACEDMATxDriver(ConstASCII8Str name,MachineDevice<PDMMACEDMARegs>* dev);
protected:
	virtual	void		handleInterrupt();
	
	virtual	void		initialize();
	virtual	void		start();
	virtual	void		stop();
public:
	virtual	void		prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction);
	virtual	void		prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process);
	virtual	UInt32	startDMA(UInt32 maxTranfer);
	virtual	void		stopDMA();
	virtual	void		waitForDMAEnd();
	
	friend class DMADriver;
};

class PDMMACEDMARxDriver	:	public DMADriver,
							public InterruptHandler
{
	// This driver is only used for PDM MACE receive access
	struct PDMMACEDMARegs*	regs;
	
	PDMMACEDMARxDriver(ConstASCII8Str name,MachineDevice<PDMMACEDMARegs>* dev);
protected:
	virtual	void		handleInterrupt();
	
	virtual	void		initialize();
	virtual	void		start();
	virtual	void		stop();
public:
	virtual	void		prepareDMAPhysical(Ptr physAddr,UInt32 count,UInt32 direction);
	virtual	void		prepareDMALogical(Ptr logAddr,UInt32 count,UInt32 direction,ProcessID process);
	virtual	UInt32	startDMA(UInt32 maxTranfer);
	virtual	void		stopDMA();
	virtual	void		waitForDMAEnd();
	
	friend class DMADriver;
};

typedef struct PDMMACEDMARegs
{
	UReg8		xmtcs;
	const UInt8	enetdmapad0[1031];
	UReg8		rcvcs;
	const UInt8	enetdmapad1[7];
	UReg8		rcvhp;
	const UInt8	enetdmapad2[3];
	UReg8		rcvtp;
	const UInt8	enetdmapad3[15];
	UReg8		xmtbch_0;
	UReg8		xmtbcl_0;
	const UInt8	enetdmapad4[14];
	UReg8		xmtbch_1;
	UReg8		xmtbcl_1;
}PDMMACEDMARegs;

/* rcv dma control and status */
#define IF			0x80
#define OVERRUN	0x40
#define IE			0x08 
#define DMARUN	0x02
#define RST		0x01

/* xmt dma control and status */
#define IF			0x80
#define SET_1		0x40
#define SET_0		0x20
#define IE			0x08 
#define DMARUN	0x02
#define RST		0x01

#endif /* __DMA__ */
