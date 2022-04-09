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
	pmu.c				MkLinux DR3alpha4		???				Basically a port of this file
	
	Version History
	============
	Terry Greeniaus	-	Sunday, 21 June 98		-	Original creation of file
	Patrick Varilly		-	Monday, 29 March 99	-	Added ready flag so start() could be called repeatedly
*/
#include "ADBInternals.h"
#include "PMUDriver.h"
#include "NKMachineInit.h"
#include "NKVirtualMemory.h"
#include "NKThreads.h"
#include "Kernel Console.h"
#include "Memory Utils.h"
#include "Chip Debugger.h"

SInt8	cmdLengthTable[256] =
	{	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// 0x00 - 0x0F
		 1, 1,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1,-1, 0,		// 0x10 - 0x1F
		-1, 0, 2, 1, 1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,		// 0x20 - 0x2F
		 4,20,-1,-1,-1,-1,-1,-1, 0, 0, 2,-1,-1,-1,-1,-1,		// 0x30 - 0x3F
		 1, 1,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1, 1,-1,-1,-1,		// 0x40 - 0x4F
		 1, 0, 2, 2,-1, 1, 3, 1, 0, 1, 0, 0, 0,-1,-1,-1,		// 0x50 - 0x5F
		 2,-1, 2, 0,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0,-1,-1,		// 0x60 - 0x6F
		 1, 1, 1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1, 4, 4,		// 0x70 - 0x7F
		 4,-1, 0,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,		// 0x80 - 0x8F
		 1, 2,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1,-1,-1,		// 0x90 - 0x9F
		 2, 2, 2, 4,-1, 0,-1,-1, 1, 1, 3, 2,-1,-1,-1,-1,		// 0xA0 - 0xAF
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// 0xB0 - 0xBF
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,	// 0xC0 - 0xCF
		 0,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1, 0, 0,-1,-1,		// 0xD0 - 0xDF
		-1, 4, 0,-1,-1,-1,-1,-1, 3,-1, 0,-1, 0,-1,-1, 0,		// 0xE0 - 0xEF
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1		// 0xF0 - 0xFF
	};


//  This table is used to determine how to handle the reply:

//	=0:	no reply should be expected.
//	=1: only a reply byte will be sent (this is a special case for a couple of commands)
//	<0:	a reply is expected and the PMGR will send a count byte.
//	>1:	a reply is expected and the PMGR will not send a count byte,
//		but the count will be (value-1).

SInt8	rspLengthTable[256] =
	{	0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,		// 0x00 - 0x0F
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1,-1,-1,-1, 0,			// 0x10 - 0x1F
		0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,		// 0x20 - 0x2F
		0, 0, 0, 0, 0, 0, 0, 0, 5,21,-1,-1,-1,-1,-1,-1,		// 0x30 - 0x3F
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1, 0,-1,-1,-1,			// 0x40 - 0x4F
		0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 3,-1,-1,-1,-1,			// 0x50 - 0x5F
		0, 0, 0, 3, 0, 0, 0, 0, 4, 4, 3, 9,-1,-1,-1,-1,			// 0x60 - 0x6F
		0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1, 1, 1,		// 0x70 - 0x7F
		0, 0, 0, 0, 0, 0, 0, 0, 6,-1,-1,-1,-1,-1,-1,-1,		// 0x80 - 0x8F
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1,-1,-1,-1,-1,		// 0x90 - 0x9F
		0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,-1,-1,-1,-1,			// 0xA0 - 0xAF
		0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xB0 - 0xBF
		0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xC0 - 0xCF
		0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1, 2,-1,-1,-1,			// 0xD0 - 0xDF
		0, 0, 1, 0, 0, 0, 0, 0,-1,-1, 2,-1,-1,-1,-1, 0,		// 0xE0 - 0xEF
		0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1		// 0xF0 - 0xFF
	};
	
static RegisterDescriptor	viaPMURegisterDescriptor[]	=	{	CHIP_REGISTER(VIA_Chip,dataB,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,handshakeDataA,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataDirectionB,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataDirectionA,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1CounterLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1CounterHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1LatchLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer1LatchHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer2CounterLow,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,timer2CounterHigh,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,shift,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,auxillaryControl,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,peripheralControl,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,interruptFlag,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,interruptEnable,REG_NOFLAGS),
													CHIP_REGISTER(VIA_Chip,dataA,REG_NOFLAGS),
													LAST_REGISTER
												};

PMUDriver::PMUDriver(ADBHardware* adb,MachineDevice<VIA_Chip>* device):
	ADBHardwareDriver("PMU ADB Driver",adb),
	InterruptHandler(device->interrupts[0])
{
	via2 = via1 = device->logicalAddr = (VIA_Chip*)NKIOMap(device->physicalAddr,device->len,WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	// Only for 5300 and Duo2300
	//via2 = (VIA_Chip*)NKIOMap((Int8*)device->physicalAddr + sizeof(VIA_Chip),sizeof(VIA_Chip),WIMG_GUARDED | WIMG_CACHE_INHIBITED,PP_READ_WRITE);
	
	// For the Chip Debugger
	new Chip("VIA PMU 1",viaPMURegisterDescriptor,via1);
	new Chip("VIA PMU 2",viaPMURegisterDescriptor,via2);
	
	flags = 0;
	deviceList = 0;
	currCommand = nil;
}

void PMUDriver::initialize()
{
	PMURequest	theRequest;
	UInt8		mask = kPMUADBint;
	UInt8		interruptState[12];
	
	theRequest.pmCommand = kPMUmaskInts;
	theRequest.pmSLength1 = 1;
	theRequest.pmSBuffer1 = &mask;
	theRequest.pmSLength2 = 0;
	theRequest.pmSBuffer2 = nil;
	theRequest.pmRBuffer = nil;
	sendPMURequest(&theRequest);
	
	do
	{
		ackPMUInterrupt();
		theRequest.pmCommand = kPMUreadINT;
		theRequest.pmSLength1 = 0;
		theRequest.pmSBuffer1 = nil;
		theRequest.pmSLength2 = 0;
		theRequest.pmSLength2 = nil;
		theRequest.pmRBuffer = interruptState;
		sendPMURequest(&theRequest);
	}while(interruptState[0]);
	
	ready = false;
}

void PMUDriver::start()
{
	if( ready )
		return;
	
	PMURequest	theRequest;
	
	ackPMUInterrupt();
	theRequest.pmCommand = kPMUmaskInts;
	theRequest.pmSLength1 = 1;
	theRequest.pmSLength2 = 0;
	theRequest.pmMisc[0] = kPMUMD2Int | kPMUbrightnessInt | kPMUADBint;
	theRequest.pmSBuffer1 = &theRequest.pmMisc[0];
	theRequest.pmRBuffer = nil;
	sendPMURequest(&theRequest);
	
	ackPMUInterrupt();
	theRequest.pmCommand = kPMUreadINT;
	theRequest.pmSLength1 = 0;
	theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = &theRequest.pmMisc[0];
	sendPMURequest(&theRequest);
	
	enable();
	
	ready = true;
}

void PMUDriver::stop()
{
	if( !ready )
		return;
	ready = false;
	
	disable();
	
	PMURequest	theRequest;
	
	ackPMUInterrupt();
	theRequest.pmCommand = kPMUmaskInts;
	theRequest.pmSLength1 = 1;
	theRequest.pmSLength2 = 0;
	theRequest.pmMisc[0] = 0;
	theRequest.pmSBuffer1 = &theRequest.pmMisc[0];
	theRequest.pmRBuffer = nil;
	sendPMURequest(&theRequest);
}

void PMUDriver::startAsyncIO(IOCommand* cmd)
{
	if(cmd)
	{
		currCommand = static_cast<ADBIOCommand*>(cmd);
		PMURequest* theRequest = reinterpret_cast<PMURequest*>(currCommand->driver1);
		if(currCommand->command != kADBPseudoCmdSetDeviceList)
			sendPMURequest(theRequest);
		switch(currCommand->command)
		{
			case kADBPseudoCmdStartStopAutoPoll:
			case kADBPseudoCmdRestart:
				currCommand->result = kADBResultOK;
				currCommand->doneIO();
				delete theRequest;
				startAsyncIO(dequeue());
			break;
			case kADBPseudoCmdSetDeviceList:
				deviceList = (currCommand->data.info.arg[0] << 8) | currCommand->data.info.arg[1];
				currCommand->result = kADBResultOK;
				currCommand->doneIO();
				delete theRequest;
				startAsyncIO(dequeue());
			break;
			case kADBPseudoCmdPowerDown:
				if(theRequest->pmRBuffer[0] == 0x70)
					currCommand->result = kADBResultOK;
				else if(theRequest->pmRBuffer[0] == 0xAA)
					Panic("PMU PowerDown denied!\n");
				else
					Panic("PMU PowerDown unknwon response!\n");
				currCommand->doneIO();
				delete theRequest;
				startAsyncIO(dequeue());
			break;
		}
	}
}

void PMUDriver::sendPMURequest(PMURequest* request)
{
	UInt16	retries = 0;
	UInt8	firstChar = request->pmCommand;
	Int8		rspLength = rspLengthTable[firstChar];
	
	if(	(request->pmSLength1 && !request->pmSBuffer1) ||
		(request->pmSLength2 && !request->pmSBuffer2) ||
		((rspLength < 0 || rspLength > 0) && !request->pmRBuffer))
		Panic("Mismatched buffer/length/response in PMUDriver::sendPMURequest!\n");
	
	if(firstChar != 0xE1 && cmdLengthTable[firstChar] != -1 && cmdLengthTable[firstChar] != request->pmSLength1 + request->pmSLength2)
		Panic("Bad command length!\n");
	
	request->pmStatus = kPMUIOError;
	
	waitForAckHi();
	
	for(;retries < 512;retries++)
	{
		if(sendPMUByte(firstChar) == kPMUNoError)
			break;
		
		Wait_ms(32);
	}
	
	if(retries == 512)
		Panic("Failed to send PMU request after 512 retries!\n");
	
	if(cmdLengthTable[firstChar] < 0)
		sendPMUByte(request->pmSLength1 + request->pmSLength2);
	
	UInt8* p = request->pmSBuffer1;
	for(Int32 i=0;i<request->pmSLength1;i++)
		sendPMUByte(*p++);
	
	p = request->pmSBuffer2;
	for(Int32 i=0;i<request->pmSLength2;i++)
		sendPMUByte(*p++);
	
	/* charCountR ==	0:	no reply at all
					1:	only a reply byte will be sent by the PGE
					<0: a length byte and a reply will be sent
					>1: a reply will be sent, but no length byte
						 (length is charCount - 1)
	*/
	request->pmRLength = 0;
	if(rspLength)
	{
		if(rspLength == 1)
			request->pmRBuffer[0] = readPMUByte();
		else
		{
			if(rspLength < 0)
				rspLength = readPMUByte();
			else
				rspLength--;
			
			request->pmRLength = rspLength;
			p = request->pmRBuffer;
			while(rspLength--)
				*p++ = readPMUByte();
		}
	}
	
	request->pmStatus = kPMUNoError;
}

Boolean PMUDriver::sendCommand(ADBIOCommand* theCommand)
{
	PMURequest*	theRequest = new PMURequest;
	
	theCommand->driver1 = reinterpret_cast<UInt32>(theRequest);
	theCommand->driver2 = PMU_STATE_CMD_START;
	
	if(theCommand->isADBCommand)
	{
		theRequest->pmCommand = kPMUpMgrADB;
		theRequest->pmSBuffer1 = theRequest->pmMisc;
		theRequest->pmSLength1 = 3;
		
		theRequest->pmMisc[0] = theCommand->command;
		theRequest->pmMisc[1] = flags;
		if(theCommand->type == kBufferCommand)
			Panic("PMU sendCommand was a buffer command!\n");
		else
		{
			theRequest->pmMisc[2] = theRequest->pmSLength2 = theCommand->data.info.numArgs;
			theRequest->pmSBuffer2 = theCommand->data.info.arg;
		}
		theRequest->pmRBuffer = nil;
		
		theCommand->driver2 = PMU_STATE_RESPONSE_EXPECTED;
		enqueue(theCommand);
	}
	else
	{
		switch(theCommand->command)
		{
			case kADBPseudoCmdStartStopAutoPoll:
				if(theCommand->data.info.arg[0])
				{
					// Enable auto-polling
					theRequest->pmCommand = kPMUpMgrADB;
					theRequest->pmMisc[0] = 0;
					theRequest->pmMisc[1] = 0x86;
					theRequest->pmMisc[2] = ((deviceList >> 8) & 0x00FF);
					theRequest->pmMisc[3] = (deviceList & 0x00FF);
					theRequest->pmSLength1 = 4;
					theRequest->pmSBuffer1 = theRequest->pmMisc;
					flags = 0x02;
				}
				else
				{
					// Disable auto-polling
					theRequest->pmCommand = kPMUpMgrADBoff;
					theRequest->pmSLength1 = 0;
					theRequest->pmSBuffer1 = nil;
					flags = 0;
				}
				theRequest->pmSLength2 = 0;
				theRequest->pmSBuffer2 = theRequest->pmRBuffer = nil;
				enqueue(theCommand);
			break;
			case kADBPseudoCmdSetDeviceList:
				enqueue(theCommand);
			break;
			case kADBPseudoCmdSetAutoPollRate:
				// This is ignored by PMU
				theCommand->result = kADBResultOK;
				currCommand->doneIO();
			break;
			case kADBPseudoCmdPowerDown:
				theRequest->pmMisc[0] = 'M';
				theRequest->pmMisc[1] = 'A';
				theRequest->pmMisc[2] = 'T';
				theRequest->pmMisc[3] = 'T';
				theRequest->pmCommand = kPMUPmgrPWRoff;
				theRequest->pmSLength1 = 4;
				theRequest->pmSBuffer1 = theRequest->pmMisc;
				theRequest->pmSLength2 = 0;
				theRequest->pmSBuffer2 = nil;
				theRequest->pmRBuffer = theRequest->pmMisc;
				enqueue(theCommand);
			break;
			case kADBPseudoCmdRestart:
				theRequest->pmCommand = kPMUresetCPU;
				theRequest->pmSLength1 = theRequest->pmSLength2 = 0;
				theRequest->pmSBuffer1 = theRequest->pmSBuffer2 = nil;
				theRequest->pmRBuffer = nil;
				enqueue(theCommand);
			break;
			default:
				Panic("Unknown PMU pseudo-command!\n");
		}
	}
	
	return true;
}

void PMUDriver::checkInterrupt()
{
	if(isPendingInterrupt())
		handleInterrupt();
}

void PMUDriver::toggleInterruptMode(Boolean noInterruptMode)
{
	if( noInterruptMode )
		disable();
	else
		enable();
}

void PMUDriver::handleInterrupt()
{
	PMURequest	theRequest;
	UInt8		interruptState[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
	
	theRequest.pmCommand = kPMUreadINT;
	theRequest.pmSLength1 = theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = interruptState;
	
	sendPMURequest(&theRequest);
	
	if(theRequest.pmRLength < 1)
		Panic("Bad ReadINT length!\n");
	
	if(interruptState[0] & kPMUADBint)
	{
		if(interruptState[0] & kPMUMD2Int)
		{
			// A device is sending us a message
			Int8		deviceNum = (theRequest.pmRBuffer[1] >> 4);
			adb->DeviceMessage(deviceNum,theRequest.pmRLength-2,theRequest.pmRBuffer+2);
		}
		else
		{
			// This is a reply of some sort
			FatalAssert(theRequest.pmRLength >= 2);
			FatalAssert(currCommand->driver2 == PMU_STATE_RESPONSE_EXPECTED);
			FatalAssert(theRequest.pmRBuffer[1] == (UInt8)currCommand->command);
			
			ADBResultCode	result;
			
			if(theRequest.pmRLength > 2)
				result = kADBResultOK;
			else
				result = kADBResultTimeout;
			
			currCommand->result = result;
			currCommand->replyLen = theRequest.pmRLength-2;
			if( currCommand->replyLen != 0 )
			{
				currCommand->reply = new(kernelProcess) UInt8[currCommand->replyLen];
				MemCopy( theRequest.pmRBuffer+2, currCommand->reply, currCommand->replyLen );
			}
			
			PMURequest*	_temp = reinterpret_cast<PMURequest*>(currCommand->driver1);
			delete _temp;
			currCommand->doneIO();
			
			startAsyncIO(dequeue());
		}
	}
}

Boolean PMUDriver::waitForAckLo()
{
	Float64 endTime = GetTime_ns() + 32*1000*1000;
	while(GetTime_ns() < endTime)
	{
		if(!(via2->dataB & (1<<3)))
			return true;
	}
	
	dout << "waitForAckLo didn't complete succesfully!\n";
	
	return false;
}

Boolean PMUDriver::waitForAckHi()
{
	Float64 endTime = GetTime_ns() + 32*1000*1000;
	while(GetTime_ns() < endTime)
	{
		if(via2->dataB & (1<<3))
			return true;
	}
	
	dout << "waitForAckHi didn't complete succesfully!\n";
	
	return false;
}

Int32 PMUDriver::sendPMUByte(UInt8 byte)
{
	via1->auxillaryControl |= 0x1C;
	_eieio();
	
	via1->shift = byte;
	_eieio();
	
	via2->dataB &= ~(1<<4);
	_eieio();
	
	if(waitForAckLo())
	{
		via2->dataB |= (1<<4);
		_eieio();
		if(!waitForAckHi())
		{
			dout << "sendPMUByte missed ack hi!\n";
			return kPMUsendEndErr;
		}
	}
	else
	{
		via2->dataB |= (1<<4);
		_eieio();
		dout << "sendPMUByte missed ack low!\n";
		return kPMUsendStartErr;
	}
	
	return kPMUNoError;
}

UInt8 PMUDriver::readPMUByte()
{
	UInt8	byte;
	
	via1->auxillaryControl |= 0x0C;
	_eieio();
	
	via1->auxillaryControl &= ~0x10;
	_eieio();
	
	byte = via1->shift;
	_eieio();
	
	via2->dataB &= ~(1<<4);
	_eieio();
	
	if(waitForAckLo())
	{
		via2->dataB |= (1<<4);
		_eieio();
		
		if(waitForAckHi())
		{
			byte = via1->shift;
			_eieio();
		}
		else
			dout << "readPMUByte missed ack hi!\n";
	}
	else
	{
		via2->dataB |= (1<<4);
		_eieio();
		dout << "readPMUByte missed ack lo!\n";
	}
	
	return byte;
}