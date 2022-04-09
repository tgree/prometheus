#ifndef __SWIM3__
#define __SWIM3__

#include "Driver.h"
#include "NKMachineInit.h"
#include "External Interrupt.h"
#include "DMA.h"
#include "Block Device.h"
#include "ShutDown.h"

class SWIM3Driver	:	public IOCommandDriver,
					public InterruptHandler
{
	struct SWIM3Regs*		regs;
	DMADriver*			dmaDriver;
	struct SWIM3Command*	currCommand;
	Boolean				diskInserted;
protected:
	SWIM3Driver(MachineDevice<SWIM3Regs>* device);
	~SWIM3Driver();
			
			Boolean	probe();
			void		select(UInt8 sel);
			void		action(UInt8 action);
			Boolean	readBit(UInt8 bit);
			void		command(SWIM3Command* cmd);
			
			void				seek();
			void				locate();
			void				transfer();
public:
	// Stuff for Driver
	virtual	void	initialize();
	virtual	void	start();
	virtual	void	stop();
	
	// Stuff for IOCommandDriver
	virtual	void	startAsyncIO(IOCommand* cmd);
	
	// Stuff for InterruptHandler
	virtual	void	handleInterrupt();
	
	friend	void	InitSWIM3();
	friend	class SWIM3Device;
};

class SWIM3Device	:	public BlockDevice,
					public ShutDownHandler
{
	class SWIM3Driver*	driver;
	
			Boolean			probe();
public:
	SWIM3Device(SWIM3Driver* driver);
	virtual ~SWIM3Device();
	
	// For BlockDevice
	virtual	UInt32			bus();
	virtual	UInt32			deviceID();
	virtual	UInt32			unit();
	virtual	UInt32			sectorSize();
	virtual	UInt32			maxSectorTransfer();
	virtual	class IOCommand*	readSectorsAsync(Int8* p,UInt32 sector,UInt32 numSectors);
	virtual	class IOCommand*	writeSectorsAsync(const Int8* p,UInt32 sector,UInt32 numSectors);
	
	// For ShutDownHandler
	virtual	void				shutDown(Boolean isShutDown);
	
	// My stuff
			SWIM3Command*	eject();
			
	friend void InitSWIM3Volumes();
};

enum
{
	// For SWIM3Command cmdType
	swim3Read	=	1,
	swim3Eject	=	2
};

enum
{
	// For SWIM3Command cmdState
	swim3Starting	=	0,
	swim3Done	=	1,
	swim3Ejecting	=	2,
	swim3Locating	=	3,
	swim3Seeking	=	4,
	swim3Settling	=	5,
	swim3Transferring	=	6
};

struct SWIM3Command	:	public IOCommand
{
	UInt32		cmdType;		// Command type
	UInt32		cmdState;		// Where we're at
	Ptr			dataPtr;		// A pointer to the data for any DMA commands
	UInt32		startSector;	// First sector to read
	UInt32		numSectors;	// Number of sectors left to transfer
	UInt32		sectCount;	// Number of sectors on the last transfer operation
	volatile UInt32	error;
	UInt32		reqTrack;
	UInt32		reqSector;
	
	virtual	UInt32	ioError();
};

// Taken from Paul Mackerras' swim3.c driver from LinuxPPC
typedef struct SWIM3Regs
{
	UReg8	data;		// For reading/writing without DMA?
	UInt8	rsrv1[15];
	
	UReg8	timer;	// Counts down at 1MHz
	UInt8	rsrv2[15];
	
	UReg8	error;
	UInt8	rsrv3[15];
	
	UReg8	mode;
	UInt8	rsrv4[15];
	
	UReg8	select;	// Controls CA0, CA1, CA2 and LSTRB signals -- whatever that means...
	UInt8	rsrv5[15];
	
	UReg8	setup;
	UInt8	rsrv6[15];
	
	union
	{
		UReg8	control;
		UReg8	control_bic;	// Writing a 1 clears that bit in control
	};
	UInt8	rsrv7[15];
	
	union
	{
		UReg8	control_bis;	// Writing a 1 sets that bit in control
		UReg8	status;		// Reading returns status
	};
	UInt8	rsrv8[15];
	
	UReg8	intr;
	UInt8	rsrv9[15];
	
	UReg8	nseek;	// Number of tracks to seek
	UInt8	rsrv10[15];
	
	UReg8	ctrack;	// Current track number
	UInt8	rsrv11[15];
	
	UReg8	csect;	// Current sector number
	UInt8	rsrv12[15];
	
	UReg8	gap3;	// Size of gap 3 in track format - whatever that means...
	UInt8	rsrv13[15];
	
	UReg8	sector;	// Sector number to read or write
	UInt8	rsrv14[15];
	
	UReg8	nsect;	// Number of sectors to read or write
	UInt8	rsrv15[15];
	
	UReg8	intr_enable;
}SWIM3Regs;

enum
{
	// Bits in select register
	CA_MASK			=	0x07,
	LSTRB			=	0x08
};

enum
{
	// Bits in control register
	DO_SEEK			=	0x80,
	FORMAT			=	0x40,
	SELECT			=	0x20,
	WRITE_SECTORS	=	0x10,
	DO_ACTION		=	0x08,
	DRIVE2_ENABLE	=	0x04,
	DRIVE_ENABLE		=	0x02,
	INTR_ENABLE		=	0x01
};

enum
{
	// Bits in status register
	FIF0_1BYTE		=	0x80,
	FIF0_2BYTE		=	0x40,
	ERROR			=	0x20,
	DATA			=	0x08,
	RDDATA			=	0x04,
	INTR_PENDING		=	0x02,
	MARK_BYTE		=	0x01
};

enum
{
	// Bits in intr and intr_enable registers
	ERROR_INTR		=	0x20,
	DATA_CHANGED	=	0x10,
	TRANSFER_DONE	=	0x08,
	SEEN_SECTOR		=	0x04,
	SEEK_DONE		=	0x02,
	TIMER_DONE		=	0x01
};

enum
{
	// Bits in error register
	ERR_DATA_CRC	=	0x80,
	ERR_ADDR_CRC	=	0x40,
	ERR_OVERRUN		=	0x04,
	ERR_UNDERRUN		=	0x01
};

enum
{
	// Bits in setup register
	S_SW_RESET		=	0x80,
	S_GCR_WRITE		=	0x40,
	S_IBM_DRIVE		=	0x20,
	S_TEST_MODE		=	0x10,
	S_FCLK_DIV2		=	0x08,
	S_GCR			=	0x04,
	S_COPY_PROT		=	0x02,
	S_INV_WDATA		=	0x01
};

enum
{
	// Select values for action
	SEEK_POSITIVE		=	0x00,
	SEEK_NEGATIVE	=	0x04,
	STEP				=	0x01,
	MOTOR_ON		=	0x02,
	MOTOR_OFF		=	0x06,
	INDEX			=	0x03,
	EJECT			=	0x07,
	SETMFM			=	0x09,
	SETGCR			=	0x0D
};

enum
{
	// Select values for select and readBit
	STEP_DIR			=	0x00,
	STEPPING			=	0x01,
	RELAX			=	0x03,
	READ_DATA_0		=	0x04,
	TWOMEG_DRIVE		=	0x05,
	SINGLE_SIDED		=	0x06,
	DRIVE_PRESENT	=	0x07,
	DISK_IN			=	0x08,
	WRITE_PROT		=	0x09,
	TRACK_ZERO		=	0x0A,
	TACHO			=	0x0B,
	READ_DATA_1		=	0x0C,
	MFM_MODE		=	0x0D,
	SEEK_COMPLETE	=	0x0E,
	ONEMEG_MEDIA		=	0x0F
};

#endif /* __SWIM3__ */