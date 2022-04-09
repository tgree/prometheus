#ifndef __IBM_IDE_DRIVER__
#define __IBM_IDE_DRIVER__

#include "IDE Driver.h"

class IBMIDEDriver	:	public IDEDriver
{
	struct IDERegsIBM*		regs;
	
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
	
	// For slamming 512 byte sectors
	virtual	void		slamRead512(Ptr destLogical);
	virtual	void		slamRead512LE(Ptr destLogical);
public:
	IBMIDEDriver(ConstASCII8Str name,MachineDevice<struct IDERegsIBM>* deviceRegs);
	virtual ~IBMIDEDriver();
};

// This is used for IBM drives
typedef struct IDERegsIBM
{
	// Offset 0
	union
	{
		UReg16BE		data_2;
		UReg32BE		data_4;
	};
	const UInt8			rsrv1[12];
	
	// Offset 16
	union
	{
		UReg8	error_read;
		UReg8	features_write;
	};
	const UInt8			rsrv2[15];
	
	// Offset 32
	UReg8				sectorCount;
	const UInt8			rsrv3[15];
	
	// Offset 48
	UReg8				sector;
	const UInt8			rsrv4[15];
	
	// Offset 64
	UReg8				cylinderLow;
	const UInt8			rsrv5[15];
	
	// Offset 80
	UReg8				cylinderHigh;
	const UInt8			rsrv6[15];
	
	// Offset 96
	UReg8				head;
	const UInt8			rsrv7[15];
	
	// Offset 112
	union
	{
		UReg8	status_read;
		UReg8	command_write;
	};
	const UInt8			rsrv8[15];
	
	// Offset 128
	const UInt8			rsrv9[224];
	
	// Offset 352
	union
	{
		UReg8	alternateStatus_read;
		UReg8	deviceControl_write;
	};
}IDERegsIBM;

#endif /* __IBM_IDE_DRIVER__ */