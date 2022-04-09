/*
	SCSI Commands.h
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
	scsi.h				Mach DR2.1 update 6		Alessandro Forin	#defines at the end of this file are taken from here.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 19 June 98	-	Made SCSIInquiryData a full 256 bytes!!!!!
*/
#ifndef __SCSI_COMMANDS__
#define __SCSI_COMMANDS__

#include "Driver.h"

#pragma options align=mac68k
typedef struct SCSI6Descriptor
{
	UInt32	opcode	:	8;
	UInt32	lun		:	3;
	UInt32	logAddr	:	21;
	UInt8	len;
	UInt8	control;
}SCSI6Descriptor;

typedef struct SCSI10Descriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	5;
	UInt32	logAddr;
	UInt32	rsrv2	:	8;
	UInt32	len		:	16;
	UInt32	control	:	8;
}SCSI10Descriptor;

typedef struct SCSI12Descriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	5;
	UInt32	logAddr;
	UInt32	len;
	UInt8	rsrv2;
	UInt8	control;
}SCSI12Descriptor;

typedef struct SCSIChangeDefDescriptor
{
	UInt8	opcode;
	UInt8	lun			:	3;
	UInt8	rsrv1		:	5;
	UInt8	rsrv2		:	7;
	UInt8	save			:	1;
	UInt8	rsrv3		:	1;
	UInt8	defParam		:	7;
	UInt8	rsrv4[4];
	UInt8	paramDataLen;
	UInt8	control;
}SCSIChangeDefDescriptor;

typedef struct SCSIInquiryDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	4;
	UInt8	evpd		:	1;
	UInt8	pageCode;
	UInt8	rsrv2;
	UInt8	allocationLen;
	UInt8	control;
}SCSIInquiryDescriptor;

typedef struct SCSITestUnitReadyDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	5;
	UInt8	rsrv2[3];
	UInt8	control;
}SCSITestUnitReadyDescriptor;

typedef struct SCSIRequestSenseDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	5;
	UInt8	rsrv2[2];
	UInt8	allocationLen;
	UInt8	control;
}SCSIRequestSenseDescriptor;

typedef struct SCSIStartStopDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	4;
	UInt8	immed	:	1;
	UInt8	rsrv2[2];
	UInt8	rsrv3	:	6;
	UInt8	LoEj		:	1;
	UInt8	start		:	1;
	UInt8	control;
}SCSIStartStopDescriptor;

typedef struct SCSIReadCapacityDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	4;
	UInt8	relAdr	:	1;
	UInt32	logAddr;
	UInt8	rsrv2[2];
	UInt8	rsrv3	:	7;
	UInt8	pmi		:	1;
	UInt8	control;
}SCSIReadCapacityDescriptor;

typedef struct SCSIPreventAllowRemovalDescriptor
{
	UInt8	opcode;
	UInt8	lun		:	3;
	UInt8	rsrv1	:	5;
	UInt8	rsrv2[2];
	UInt8	rsrv3	:	7;
	UInt8	prevent	:	1;
	UInt8	control;
}SCSIPreventAllowRemovalDescriptor;

typedef struct SCSIInquiryData
{
	UInt8	peripheralQualifier	:	3;
	UInt8	peripheralDevType	:	5;
	
	UInt8	rmb				:	1;
	UInt8	devTypeModifier	:	7;
	
	UInt8	isoVersion			:	2;
	UInt8	ecmaVersion		:	3;
	UInt8	ansiVersion		:	3;
	
	UInt8	aenc				:	1;
	UInt8	trmpIOP			:	1;
	UInt8	rsrv1			:	2;
	UInt8	responseDataFormat	:	4;
	
	UInt8	additionalLen;
	UInt8	rsrv2[2];
	
	UInt8	relAdr			:	1;
	UInt8	wbus32			:	1;
	UInt8	wbus16			:	1;
	UInt8	sync				:	1;
	UInt8	linked			:	1;
	UInt8	rsrv3			:	1;
	UInt8	cmdQueue			:	1;
	UInt8	sftReset			:	1;
	
	ASCII8	vendorID[8];
	ASCII8	productID[16];
	ASCII8	revisionLevel[4];
	UInt8	vendorSpecific1[20];
	UInt8	rsrv4[40];
	UInt8	vendorSpecific2[160];
}SCSIInquiryData;

typedef struct scsi_sense_data
{
	UInt8	valid			:	1;
	UInt8	error_code	:	7;
	
	UInt8	segment_number;
	
	UInt8	fm			:	1;
	UInt8	eom			:	1;
	UInt8	ili			:	1;
	UInt8	rsrv			:	1;
	UInt8	sense_key		:	4;
	
	UInt8	info[4];
	
	UInt8	add_len;
	
	UInt8	command_info[4];
	
	UInt8	asc;
	
	UInt8	ascq;
	
	UInt32	fruc			:	8;
	UInt32	sksv			:	1;
	UInt32	key_specific	:	23;
	
	UInt8	data[238];
} scsi_sense_data;
#pragma options align=reset

enum
{
	// Peripheral device types for SCSIInquiryData
	directAccessDev	=	0,
	sequentialAccessDev	=	1,
	printerDev		=	2,
	processorDev		=	3,
	writeOnceDev		=	4,
	CDROMDev		=	5,
	scannerDev		=	6,
	opticalMemDev		=	7,
	mediumChangerDev	=	8,
	communicationDev	=	9,
	unknownDev		=	31
};

typedef union SCSIGenericDescriptor
{
	SCSI6Descriptor					scsi6;
	SCSI10Descriptor					scsi10;
	SCSI12Descriptor					scsi12;
	SCSIChangeDefDescriptor				scsiChangeDefinition;
	SCSIInquiryDescriptor				scsiInquiry;
	SCSITestUnitReadyDescriptor			scsiTestUnitReady;
	SCSIRequestSenseDescriptor			scsiRequestSense;
	SCSIStartStopDescriptor				scsiStartStop;
	SCSIReadCapacityDescriptor			scsiReadCapacity;
	SCSI6Descriptor					scsiRead6;
	SCSI10Descriptor					scsiRead10;
	SCSIPreventAllowRemovalDescriptor	scsiPreventAllowRemoval;
}SCSIGenericDescriptor;

struct SCSICommand	:	public IOCommand
{
	SCSIGenericDescriptor	cmd;
	UInt32				cmdLen;			// The length of the SCSI command
	void*				dataPtr;			// A pointer to the data for any DMA commands
	UInt32				dataLen;			// The length of data to transfer for any DMA commands
	volatile UInt32			remainData;		// The number of bytes remaining to be transferred for DMA commands
	volatile UInt32			requestData;		// The number of bytes in the current transfer request for DMA commands
	volatile UInt8			msg[16];			// Memory for any messages returned on the SCSI bus
	UInt32				msgLen;			// The length of the message in msg[]
	volatile UInt32			remainMsg;		// The number of bytes remaining to be transferred for this message
	UInt32				targetID;			// The ID of the target device
	volatile UInt32			transactionFlags;	// For SCSIDriver/derived classes use only
	volatile UInt32			error;			// Error code returned after command executes
	volatile UInt8			result[256];		// Memory for result data.
	
	virtual	UInt32		ioError();
};

#define	CPU_SCSI_ID					7		// SCSI ID of the CPU
#define	DMA_MAX_TRANSFER			65536

#define	SCSI_CMD_CHANGE_DEF		0x40
#define	SCSI_CMD_INQUIRY			0x12	/* E all (2M all) */
#define	SCSI_CMD_TEST_UNIT_READY	0x00	/* O all 2M all */
#define	SCSI_CMD_READ_6			0x08	/* M disk tape O worm rom */
#define	SCSI_CMD_READ_10			0x28
#define	SCSI_CMD_READ_CAPACITY	0x25	/* E disk worm rom */
#define	SCSI_CMD_REQUEST_SENSE	0x03	/* M all */
#define	SCSI_CMD_PREVENT_ALLOW_REMOVAL	0x1e	/* O disk tape worm rom */

#define	SCSI_RET_SUCCESS	0x01
#define	SCSI_RET_RETRY		0x02
#define	SCSI_RET_NEED_SENSE	0x04
#define	SCSI_RET_DEVICE_DOWN	0x10

#define	SCSI_SYNC_XFER_REQUEST		0x01	/* IT */
#define	SCSI_MESSAGE_REJECT			0x07	/* M IT  */
#define	SCSI_IFY_ENABLE_DISCONNECT		0x40	/* I  */
#define	SCSI_IDENTIFY					0x80	/* IT */
#define	SCSI_EXTENDED_MESSAGE		0x01	/*   IT  */
#define	SCSI_NOP						0x08	/* M I   */
#define	SCSI_SAVE_DATA_POINTER		0x02	/* O T   */
#define	SCSI_RESTORE_POINTERS			0x03	/* O T   */
#define	SCSI_DISCONNECT				0x04	/* O T   */
#define	SCSI_COMMAND_COMPLETE		0x00	/* M T   */

#define	DEV_NO_DISCONNECT			0x001	/* Do not preform disconnect */

#define	STATE_DISCONNECTED			0x001	/* currently disconnected from bus */
#define	STATE_DMA_IN				0x004	/* reading from SCSI device */
#define	STATE_DMA_RUNNING			0x002	/* data DMA started */
#define	STATE_DMA_FIFO				0x100	/* using FIFO to transfer data */

#define	SCSI_ST_GOOD					0x00		/* scsi_status_code values */
#define	SCSI_ST_CHECK_CONDITION		0x01
#define	SCSI_ST_CONDITION_MET			0x02
#define	SCSI_ST_BUSY					0x04
#define	SCSI_ST_INT_GOOD				0x08
#define	SCSI_ST_INT_MET				0x0A
#define	SCSI_ST_RES_CONFLICT			0x0C

#define	SCSI_CMD_START_STOP_UNIT		0x1b	/* O disk prin worm rom */
#define	SCSI_CMD_LOAD_UNLOAD			0x1b	/* O tape */
#define	SCSI_CMD_SS_IMMED			0x01
#define	SCSI_CMD_SS_START			0x01
#define	SCSI_CMD_SS_RETEN			0x02
#define	SCSI_CMD_SS_RETAIN			0x01
#define	SCSI_CMD_SS_EJECT			0x02

#define	SCSI_SNS_NOSENSE		0x00
#define	SCSI_SNS_RECOVERED	0x01
#define	SCSI_SNS_NOTREADY	0x02
#define	SCSI_SNS_MEDIUM_ERR	0x03
#define	SCSI_SNS_HW_ERR		0x04
#define	SCSI_SNS_ILLEGAL_REQ	0x05
#define	SCSI_SNS_UNIT_ATN	0x06
#define	SCSI_SNS_PROTECT		0x07
#define	SCSI_SNS_BLANK_CHK	0x08
#define	SCSI_SNS_VUQE		0x09
#define	SCSI_SNS_COPY_ABRT	0x0A
#define	SCSI_SNS_ABORTED		0x0B
#define	SCSI_SNS_EQUAL		0x0C
#define	SCSI_SNS_VOLUME_OVFL	0x0D
#define	SCSI_SNS_MISCOMPARE	0x0E
#define	SCSI_SNS_RESERVED	0x0F

#endif	/* __SCSI_COMMANDS__ */
