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
	mesh.h				Mach DR2.1 update 6		???				Basically a direct copy, with some modifications
															for Prometheus
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright notice to file.
*/
#ifndef __MK_MESH__
#define __MK_MESH__

/*
 * MESH Commands
 */

#define	MESH_CMD_NOP				0x00
#define	MESH_CMD_ARBITRATE			0x01
#define	MESH_CMD_SELECT				0x02
#define	MESH_CMD_COMMAND			0x03
#define	MESH_CMD_STATUS				0x04
#define	MESH_CMD_DATAOUT			0x05
#define	MESH_CMD_DATAIN				0x06
#define	MESH_CMD_MSGOUT				0x07
#define	MESH_CMD_MSGIN				0x08
#define	MESH_CMD_BUSFREE			0x09
#define	MESH_CMD_ENABLE_PARITY		0x0A
#define	MESH_CMD_DISABLE_PARITY		0x0B
#define	MESH_CMD_ENABLE_RESELECT		0x0C
#define	MESH_CMD_DISABLE_RESELECT	0x0D
#define	MESH_CMD_RESET_MESH			0x0E
#define	MESH_CMD_FLUSH_FIFO			0x0F
#define	MESH_SEQ_DMA				0x80
#define	MESH_SEQ_TARGET				0x40
#define	MESH_SEQ_ATN				0x20
#define	MESH_SEQ_ACT_NEG			0x10

#define	MESH_BUS1_STATUS_SEL		0x20
#define	MESH_BUS1_STATUS_BUSY		0x40
#define	MESH_BUS1_STATUS_RESET		0x80

/* 
 * Exception Status bits 
 */

#define	MESH_EXCPT_SELTO		0x01
#define	MESH_EXCPT_PHASE	0x02
#define	MESH_EXCPT_ARBLOST	0x04
#define	MESH_EXCPT_RESEL		0x08
#define	MESH_EXCPT_SEL		0x10
#define	MESH_EXCPT_SEL_WATN	0x20

/*
 * Error Status Register
 */

#define	MESH_ERR_PARITY0		0x01
#define	MESH_ERR_PARITY1		0x02
#define	MESH_ERR_PARITY2		0x04
#define	MESH_ERR_PARITY3		0x08
#define	MESH_ERR_SEQERR		0x10
#define	MESH_ERR_SCSI_RESET	0x20
#define	MESH_ERR_DISCONNECT	0x40

/* 
 * Interrupt Status Register
 */

#define	MESH_INTR_DONE		0x01
#define	MESH_INTR_EXCPT		0x02
#define	MESH_INTR_ERROR		0x04

/* 
 * Various Bus Status 0 values
 */
#define	SCSI_IO		0x01		/* Input/Output */
#define	SCSI_CD		0x02		/* Command/Data */

#define	MESH_PHASE_IO		0x01
#define	MESH_PHASE_CD		0x02
#define	MESH_PHASE_MSG		0x04
#define	MESH_PHASE_ATN		0x08
#define	MESH_PHASE_ACK		0x10
#define	MESH_PHASE_REQ		0x20
#define	MESH_PHASE_ACK32	0x40	/* For SCSI WIDE implementations */
#define	MESH_PHASE_REQ32	0x80

#define	MESH_PHASE_MASK(phase)	(phase & 0x7)
#define	MESH_PHASE_DATO			0x00
#define	MESH_PHASE_DATI			MESH_PHASE_IO
#define	MESH_PHASE_CMD			MESH_PHASE_CD
#define	MESH_PHASE_STATUS		(MESH_PHASE_CD|SCSI_IO)
#define	MESH_PHASE_MSGOUT		(MESH_PHASE_MSG|SCSI_CD)
#define	MESH_PHASE_MSGIN 		(MESH_PHASE_MSG|SCSI_CD|SCSI_IO)

/*
 * This structure lets us access the Mesh registers.
 */

struct mesh_regmap
{
	UReg8				r_count0;
	const UInt8			pad00[0x0F];
	UReg8				r_count1;
	const UInt8			pad01[0x0F];
	UReg8				r_fifo;
	const UInt8			pad02[0x0F];
	UReg8				r_cmd;
	const UInt8			pad03[0x0F];
	UReg8				r_bus0status;
	const UInt8			pad04[0x0F];
	UReg8				r_bus1status;
	const UInt8			pad05[0x0F];
	UReg8				r_fifo_cnt;
	const UInt8			pad06[0x0F];
	UReg8				r_excpt;
	const UInt8			pad07[0x0F];
	UReg8				r_error;
	const UInt8			pad08[0x0F];
	UReg8				r_intmask;
	const UInt8			pad09[0x0F];
	UReg8				r_interrupt;
	const UInt8			pad10[0x0F];
	UReg8				r_sourceid;
	const UInt8			pad11[0x0F];
	UReg8				r_destid;
	const UInt8			pad12[0x0F];
	UReg8				r_sync;
	const UInt8			pad13[0x0F];
	UReg8				r_meshid;
	const UInt8			pad14[0x0F];
	UReg8				r_sel_timeout;
};

#define	MESH_SET_XFER(regs, count) { regs->r_count0 = (count & 0xff); regs->r_count1 = (count >> 8) & 0xff; _eieio(); }
#define	MESH_GET_XFER(regs) ((regs->r_count1 << 8) | regs->r_count0)

void InitMesh(void);

#endif
