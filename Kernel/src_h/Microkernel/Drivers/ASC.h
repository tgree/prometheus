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
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	scsi_53C94.h			Mach DR2.1 update 6		Alessandro Forin	Basically the same file, with some small modifications
															for Prometheus.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright notices to file
*/
#ifndef __SCSI_53C94__
#define __SCSI_53C94__

enum
{
	fastASC	=	0,
	slowASC	=	1
};

void InitASC(void);	// Returns true if there is an ASC chip

typedef struct asc_curio_regmap{
	UReg8			asc_tc_lsb;	/* rw: Transfer Counter LSB */
	const UInt8		pad0[15];
	UReg8			asc_tc_msb;	/* rw: Transfer Counter MSB */
	const UInt8		pad1[15];
	UReg8			asc_fifo;		/* rw: FIFO top */
	const UInt8		pad2[15];
	UReg8			asc_cmd;		/* rw: Command */
	const UInt8		pad3[15];
	union
	{
		UReg8	asc_csr;		/* r:  Status w: SCSI Dest ID*/
		UReg8	asc_dest_id;
	};
	const UInt8		pad4[15];
	union
	{
		UReg8	asc_intr;		/* r:  Interrupt w: SCSI Timeout Register */
		UReg8	asc_timeout;
	};
	const UInt8		pad5[15];
	union
	{
		UReg8	asc_ss;		/* r:  Sequence Step w: Sync Transfer Period */
		UReg8	asc_sync_tp;
	};
	const UInt8		pad6[15];
	union
	{
		UReg8	asc_flags;		/* r:  FIFO flags + seq step w: Sync Offset Register */
		UReg8	asc_sync_offset;
	};
	const UInt8		pad7[15];
	UReg8			asc_cnfg1;	/* rw: Configuration 1 */
	const UInt8		pad8[15];
	UReg8			asc_ccf;		/* w:  Clock Conv. Factor */
	const UInt8		pad9[15];
	UReg8			asc_test;		/* w:  Test Mode */
	const UInt8		pad10[15];
	UReg8			asc_cnfg2;	/* rw: Configuration 2 */
	const UInt8		pad11[15];
	UReg8			asc_cnfg3;	/* rw: Configuration 3 */
	const UInt8		pad12[15];
	UReg8		 	asc_cnfg4;	/* rw: Configuration 4 */
	const UInt8		pad13[15];
	UReg8			fas_tc_hi;		/* rw: high register count (only present in F chips)*/
} asc_curio_regmap;

/*
 * Transfer Count: access macros
 * That a NOP is required after loading the dma counter
 * I learned on the NCR test code. Sic.
 */

#define	ASC_TC_MAX	0x10000
#define	FAS_TC_MAX	0x1000000
#define	FSC_TC_MAX	0x1000000

#define ASC_TC_GET(asc,val)                             \
        val = !((asc)->chipType == ASC_NCR_53CF94) ? \
        (asc)->regs->asc_tc_lsb|((asc)->regs->asc_tc_msb<<8) :      \
        (asc)->regs->asc_tc_lsb|((asc)->regs->asc_tc_msb<<8)|((asc)->regs->fas_tc_hi<<16)

#define ASC_TC_PUT(asc,val)                             \
        (asc)->regs->asc_tc_lsb=(val);                        \
        (asc)->regs->asc_tc_msb=(val)>>8;                     \
        if((asc)->chipType == ASC_NCR_53CF94) \
                (asc)->regs->fas_tc_hi = (val)>>16;//           \
        (asc)->regs->asc_cmd = ASC_CMD_NOP|ASC_CMD_DMA;

/*
 * Family Code and Revision Level: access macros
 *
 * Only valid when the following conditions are true:
 *
 *	- After power-up or chip reset
 *	- Before the fas_tc_hi register is loaded
 *	- The FAS_CNFG2_FEATURES bit is set
 *	- A DMA NOP command has been issued
 */

#define	FAMILY_MASK		0xf8
#define REV_MASK		0x07

#define FAS_FAMILY_CODE		0x02		/* Emulex FAS Controller */
#define FSC_FAMILY_CODE		0x14		/* NCR FSC Controller    */

#define	FAMILY(code)					\
	(((code)&FAMILY_MASK)>>3)

#define REV(level)					\
	((level)&REV_MASK)

#define	ASC_MIN_PERIOD_20	5
#define	ASC_MIN_PERIOD_40	4

/*
 * FIFO register
 */

#define ASC_FIFO_DEEP		16

/*
 * Command register (command bit masks)
 */

#define ASC_CMD_DMA		0x80
#define ASC_CMD_MODE_MASK	0x70
#define ASC_CMD_DISC_MODE	0x40
#define ASC_CMD_T_MODE		0x20
#define ASC_CMD_I_MODE		0x10
#define ASC_CMD_MASK		0x0f

/*
 * Command register (command codes)
 */
					/* Miscellaneous */
#define ASC_CMD_NOP		0x00
#define ASC_CMD_FLUSH		0x01
#define ASC_CMD_RESET		0x02
#define ASC_CMD_BUS_RESET	0x03
					/* Initiator state */
#define ASC_CMD_XFER_INFO	0x10
#define ASC_CMD_I_COMPLETE	0x11
#define ASC_CMD_MSG_ACPT	0x12
#define ASC_CMD_XFER_PAD	0x18
#define ASC_CMD_SET_ATN		0x1a
#define ASC_CMD_CLR_ATN		0x1b
					/* Target state */
#define ASC_CMD_SND_MSG		0x20
#define ASC_CMD_SND_STATUS	0x21
#define ASC_CMD_SND_DATA	0x22
#define ASC_CMD_DISC_SEQ	0x23
#define ASC_CMD_TERM		0x24
#define ASC_CMD_T_COMPLETE	0x25
#define ASC_CMD_DISC		0x27
#define ASC_CMD_RCV_MSG		0x28
#define ASC_CMD_RCV_CDB		0x29
#define ASC_CMD_RCV_DATA	0x2a
#define ASC_CMD_RCV_CMD		0x2b
#define ASC_CMD_ABRT_DMA	0x04
					/* Disconnected state */
#define ASC_CMD_RESELECT	0x40
#define ASC_CMD_SEL		0x41
#define ASC_CMD_SEL_ATN		0x42
#define ASC_CMD_SEL_ATN_STOP	0x43
#define ASC_CMD_ENABLE_SEL	0x44
#define ASC_CMD_DISABLE_SEL	0x45
#define ASC_CMD_SEL_ATN3	0x46

/* this is approximate (no ATN3) but good enough */
#define	asc_isa_select(cmd)	(((cmd)&0x7c)==0x40)

/*
 * Status register, and phase encoding
 */

#define ASC_CSR_INT		0x80
#define ASC_CSR_GE		0x40
#define ASC_CSR_PE		0x20
#define ASC_CSR_TC		0x10
#define ASC_CSR_VGC		0x08
#define ASC_CSR_MSG		0x04
#define ASC_CSR_CD		0x02
#define ASC_CSR_IO		0x01

#define	ASC_PHASE(csr)		SCSI_PHASE(csr)

/*
 * Destination Bus ID
 */

#define ASC_DEST_ID_MASK	0x07


/*
 * Interrupt register
 */

#define ASC_INT_RESET		0x80
#define ASC_INT_ILL		0x40
#define ASC_INT_DISC		0x20
#define ASC_INT_BS		0x10
#define ASC_INT_FC		0x08
#define ASC_INT_RESEL		0x04
#define ASC_INT_SEL_ATN		0x02
#define ASC_INT_SEL		0x01


/*
 * Timeout register:
 *
 *	The formula in the NCR specification does not yeild an accurate timeout
 *	The following formula is correct:
 *
 *	val = (timeout * CLK_freq) / (7682 * CCF);
 */

#define	asc_timeout_250(clk,ccf)	((31*clk)/ccf)

/* 250 msecs at the max clock frequency in each range (see CCF register ) */
#define	ASC_TIMEOUT_250		0xa3

/*
 * Sequence Step register
 */

#define ASC_SS_XXXX		0xf0
#define FAS_SS_XXXX		0xf8
#define ASC_SS_SOM		0x08
#define ASC_SS_MASK		0x07
#define	ASC_SS(ss)		((ss)&ASC_SS_MASK)

/*
 * Synchronous Transfer Period
 */

#define ASC_STP_MASK		0x1f
#define ASC_STP_MIN		0x05		/*  5 clk per byte */
#define ASC_STP_MAX		0x03		/* after ovfl, 35 clk/byte */
#define FAS_STP_MIN		0x04		/*  4 clk per byte */
#define FAS_STP_MAX		0x1f		/* 35 clk per byte */

/*
 * FIFO flags
 */

#define ASC_FLAGS_SEQ_STEP	0xe0
#define FAS_FLAGS_SEQ_CNT	0x20
#define ASC_FLAGS_FIFO_CNT	0x1f

/*
 * Synchronous offset
 */

#define ASC_SYNO_MASK		0x0f		/* 0 -> asyn */
#define	FAS_ASSERT_MASK		0x30		/* REQ/ACK assertion control */
#define	FAS_DEASSERT_MASK	0xc0		/* REQ/ACK deassertion control */

/*
 * Configuration 1
 */

#define ASC_CNFG1_SLOW		0x80
#define ASC_CNFG1_SRD		0x40
#define ASC_CNFG1_P_TEST	0x20
#define ASC_CNFG1_P_CHECK	0x10
#define ASC_CNFG1_TEST		0x08
#define ASC_CNFG1_MY_BUS_ID	0x07

/*
 * CCF register
 */

#define ASC_CCF_10MHz		0x2	/* 10 MHz */
#define ASC_CCF_15MHz		0x3	/* 10.01 MHz to 15 MHz */
#define ASC_CCF_20MHz		0x4	/* 15.01 MHz to 20 MHz */
#define ASC_CCF_25MHz		0x5	/* 20.01 MHz to 25 MHz */
#define FAS_CCF_30MHz		0x6	/* 25.01 MHz to 30 MHz */
#define FAS_CCF_35MHz		0x7	/* 30.01 MHz to 35 MHz */
#define FAS_CCF_40MHz		0x8	/* 35.01 MHz to 40 MHz, 8 clocks */	// *** TG: This used to be 0x0.  The CCF register gets 3 bits, but this value is also
															// used in a divide, so you get a divide by zero exception!!!  That's NO GOOD!  Now we
															// & this with 0x07 whenever writing to the CCF register

#define	mhz_to_ccf(x)		(((x-1)/5)+1)	/* see specs for limits */

/*
 * Test register
 */

#define ASC_TEST_XXXX		0xf8
#define ASC_TEST_HI_Z		0x04
#define ASC_TEST_I		0x02
#define ASC_TEST_T		0x01

/*
 * Configuration 2
 */

#define ASC_CNFG2_RFB		0x80
#define ASC_CNFG2_EPL		0x40
#define FAS_CNFG2_FEATURES	0x40
#define ASC_CNFG2_EBC		0x20
#define ASC_CNFG2_DREQ_HIZ	0x10
#define ASC_CNFG2_SCSI2		0x08
#define ASC_CNFG2_BPA		0x04
#define ASC_CNFG2_RPE		0x02
#define ASC_CNFG2_DPE		0x01

/*
 * Configuration 3
 */

#define FAS_CNFG3_IDRESCHK	0x80
#define	FAS_CNFG3_QUENB		0x40
#define	FAS_CNFG3_CDB10		0x20
#define	FAS_CNFG3_FASTSCSI	0x10
#define FAS_CNFG3_FASTCLK	0x08
#define ASC_CNFG3_XXXX		0xf8
#define ASC_CNFG3_SRB		0x04
#define ASC_CNFG3_ALT_DMA	0x02
#define ASC_CNFG3_T8		0x01

/*
 * FSC Configuration 4
 */

#define FSC_CNFG4_BBTE		0x01
#define FSC_CNFG4_TEST		0x02
#define FSC_CNFG4_EAN		0x04

#define	ASC_NCR_53C94		0	/* Regular 53C94 */
#define	ASC_NCR_53CF94		1	/* Fast-SCSI version of 53C94 */

#define	ASC_STATE_IDLE		0	/* idle state */
#define	ASC_STATE_BUSY		1	/* selecting or currently connected */
#define ASC_STATE_TARGET	2	/* currently selected as target */
#define ASC_STATE_RESEL		3	/* currently waiting for reselect */

#endif /* __SCSI_53C94__ */