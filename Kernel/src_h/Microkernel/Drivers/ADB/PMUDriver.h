/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
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
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	pmu_defs.h			MkLinux DR3alpha4		???				Based on this file
	
	Version History
	============
	Terry Greeniaus	-	Sunday, 21 June 98	-	Original creation of file
*/
#ifndef __PMU_DRIVER__
#define __PMU_DRIVER__

#include "ADBHardwareDriver.h"
#include "VIA Chip.h"
#include "External Interrupt.h"
#include "NKMachineInit.h"

class PMUDriver	:	public ADBHardwareDriver,
					public InterruptHandler
{
protected:
	struct VIA_Chip*		via1;	// PMU uses 2 via chips!!?!?
	struct VIA_Chip*		via2;
	ADBIOCommand*		currCommand;
	UInt8				flags;
	UInt16				deviceList;
	Boolean				ready;
	
	inline	void			ackPMUInterrupt() {via1->interruptFlag = ((1<<ifIRQ) | (1<<ifCB1)); _eieio();}
	inline	Boolean		isPendingInterrupt() {Boolean val = ((via1->interruptFlag & (1<<4)) != 0); _eieio(); return val;}
			Boolean		waitForAckHi();
			Boolean		waitForAckLo();
			
			void			sendPMURequest(struct PMURequest* request);
			
			Int32		sendPMUByte(UInt8 byte);	// Returns error code
			UInt8		readPMUByte();
public:
	PMUDriver(ADBHardware* adb,MachineDevice<VIA_Chip>* device);
	
	// Stuff for Driver
	virtual	void			initialize();
	virtual	void			start();
	virtual	void			stop();
	
	// Stuff for IOCommandDriver
	virtual	void			startAsyncIO(IOCommand* cmd);
	
	// Stuff for ADBHardwareDriver
	virtual	Boolean		sendCommand(ADBIOCommand* theCommand);
	virtual	void			checkInterrupt();
	virtual	void			toggleInterruptMode(Boolean noInterruptMode);
	
	// Stuff for InterruptHandler
	virtual	void			handleInterrupt();
};

enum {
	kPMUNoError				= 0,
	kPMUInitError				= 1,	// Failed to initialize library
	kPMUParameterError		= 2,	// NULL/malformed PMURequest block
	kPMURequestError			= 3,	// PMUStartIO before ClientInit
	kPMUIOError				= 4,	// Generic I/O failure
	kPMUsendEndErr			= 5,	//
	kPMUsendStartErr			= 6,	//
	kPMUrecvEndErr			= 7,	//
	kPMUrecvStartErr			= 8,	//
	kPMUPacketCommandError	= 9,	// Packet Cmd invalid
	kPMUPacketSizeError		= 10	// Packet Size invalid
};

enum
{
	kPMUpMgrADB   	= 			0x20,   			// send ADB command
	kPMUpMgrADBoff   	= 			0x21,   			// turn ADB auto-poll off
	kPMUxPramWrite	= 			0x32,   			// write extended PRAM byte(s)
	kPMUtimeRead  	= 			0x38,   			// read the time from the clock chip
	kPMUxPramRead 	= 			0x3A,   			// read extended PRAM byte(s)
	kPMUmaskInts		=			0x70,			// mask interrupts
	kPMUreadINT   		= 			0x78,   			// get PMGR interrupt data
	kPMUPmgrPWRoff  	= 			0x7E,  			// turn system power off
	kPMUresetCPU		=			0xD0   			// reset the CPU
};

enum {
	kPMUMD0Int		= 0x01,	/* interrupt type 0 (machine-specific)*/
	kPMUMD1Int		= 0x02,	/* interrupt type 1 (machine-specific)*/
	kPMUMD2Int		= 0x04,	/* interrupt type 2 (machine-specific)*/
	kPMUbrightnessInt	= 0x08,	/* brightness button has been pressed, value changed*/
	kPMUADBint		= 0x10,	/* ADB*/
	kPMUbattInt		= 0x20,	/* battery*/
	kPMUenvironmentInt	= 0x40,	/* environment*/
	kPMUoneSecInt		= 0x80	/* one second interrupt*/
};

enum
{
	PMU_STATE_INTERRUPT_LIMBO	= -1,	//	Oh-oh
	PMU_STATE_IDLE				= 0,		//	nothing to do..
	PMU_STATE_CMD_START			= 1,		//	sending a command
	PMU_STATE_RESPONSE_EXPECTED	= 2		//	waiting for a response (ADB)
};

typedef struct PMURequest
{
	Int32	pmStatus;
	UInt16	pmCommand;
	UInt8* 	pmSBuffer1;
	UInt16	pmSLength1;
	UInt8* 	pmSBuffer2;
	UInt16	pmSLength2;
	UInt8*	pmRBuffer;
	UInt16	pmRLength;
	UInt8	pmMisc[16];
}PMURequest;

#endif /* __PMU_DRIVER__ */