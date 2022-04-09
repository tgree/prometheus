#ifndef __CMD_PCI_646__
#define __CMD_PCI_646__

#include "PCI.h"
#include "IDE Driver.h"

// The actual CMD 646 PCI driver
class CMD646IDEDriver	:	public IDEDriver
{
	class PCIDevice*			dev;
	struct CMD646GeneralRegs*	regs;
	struct CMD646AlternateRegs*	altRegs;
	struct CMD646PCIRegs*		pciRegs;
	
	// Read stuff for IDEDriver
	virtual	UInt8	readError();
	virtual	UInt8	readSectorCount();
	virtual	UInt8	readSector();
	virtual	UInt8	readCylinderLow();
	virtual	UInt8	readCylinderHigh();
	virtual	UInt8	readHead();
	virtual	UInt8	readStatus();
	
	// Write stuff for IDEDriver
	virtual	void		writeData16(UInt16 data);
	virtual	void		writeFeatures(UInt8 data);
	virtual	void		writeSectorCount(UInt8 data);
	virtual	void		writeSector(UInt8 data);
	virtual	void		writeCylinderLow(UInt8 data);
	virtual	void		writeCylinderHigh(UInt8 data);
	virtual	void		writeHead(UInt8 data);
	virtual	void		writeCommand(UInt8 data);
	virtual	void		writeDeviceControl(UInt8 data);
	
	// For slamming a 512 byte sector
	virtual	void		slamRead512(Ptr destLogical);
	virtual	void		slamRead512LE(Ptr destLogical);
	
	CMD646IDEDriver(PCIDevice* theDev);
	virtual ~CMD646IDEDriver();
	
	friend class CMD646IDEProber;
};

// A prober so we can find CMD 646 devices at PCI probe time
struct CMD646IDEProber	:	public PCIProber
{
	virtual	Boolean	probe(PCIDevice* pciDevice);
};

// IDE Register descriptions
typedef struct CMD646GeneralRegs	// Accessed via PCI Base Address 0/2
{
	UReg8	data;
	union
	{
		UReg8	error_read;
		UReg8	features_write;
	};
	UReg8	sectorCount;
	UReg8	sector;
	UReg8	cylinderLow;
	UReg8	cylinderHigh;
	UReg8	head;
	union
	{
		UReg8	status_read;
		UReg8	command_write;
	};
}CMD646GeneralRegs;

typedef struct CMD646AlternateRegs	// Accessed via PCI Base Address 1/3
{
	UReg8	rsrv[2];
	union
	{
		UReg8	alternateStatus_read;
		UReg8	deviceControl_write;
	};
}CMD646AlternateRegs;

typedef struct CMD646PCIRegs		// Accessed via PCI Base Address 4
{
	UReg8	bmidecr0;	// Offset 0
	UReg8	mrdmode;	// Offset 1
	UReg8	bmidesr0;	// Offset 2
	UReg8	udidetcr0;	// Offset 3
	
	UReg32LE	dtrp0;	// Offset 4
	
	UReg8	bmidecr1;	// Offset 8
	UReg8	rsrv;
	UReg8	bmidesr1;	// Offset A
	UReg8	udidetcr1;	// Offset B
	
	UReg32LE	dtpr1;	// Offset C
}CMD646PCIRegs;

enum
{
	// mrdmode stuff
	MRDMODE_INT1		=	0x04,	// Signals a pending interrupt on device 1
	MRDMODE_INT2		=	0x08,	// Signals a pending interrupt on device 2
	MRDMODE_INT_DISABLE1	=	0x10,	// Disables interrupts on device 1
	MRDMODE_INT_DISABLE2	=	0x20	// Disables interrupts on device 2
};

#endif /* __CMD_PCI_646__ */