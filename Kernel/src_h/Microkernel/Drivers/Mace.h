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
 * Copyright 1996 1995 by Apple Computer, Inc. 1997 1996 1995 1994 1993 1992 1991  
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
 * MkLinux
 */
/* 
 * PMach Operating System
 * Copyright (c) 1995 Santa Clara University
 * All Rights Reserved.
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
 *	File:	if_3c501.h
 *	Author: Philippe Bernadat
 *	Date:	1989
 * 	Copyright (c) 1989 OSF Research Institute 
 *
 * 	3COM Etherlink 3C501 Mach Ethernet drvier
 */
/*
  Copyright 1990 by Open Software Foundation,
Cambridge, MA.

		All Rights Reserved

  Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies and
that both the copyright notice and this permission notice appear in
supporting documentation, and that the name of OSF or Open Software
Foundation not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

  OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	mace.h				Mach DR2.1 update 6		Philippe Bernadat	Basically a direct copy, with some modifcations
															for Prometheus.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright notices to file.
*/
#ifndef __MACE__
#define __MACE__

#define ENETPAD(n)      const UInt8 n[15] 

typedef struct mace_board
{
	UReg8	rcvfifo;  /* 00  receive fifo */
	ENETPAD(epad0);
	UReg8	xmtfifo;  /* 01  transmit fifo */
	ENETPAD(epad1);
	UReg8	xmtfc;    /* 02  transmit frame control */
	ENETPAD(epad2);
	UReg8	xmtfs;    /* 03  transmit frame status */
	ENETPAD(epad3);
	UReg8	xmtrc;    /* 04  transmit retry count */
	ENETPAD(epad4);
	UReg8	rcvfc;    /* 05  receive frame control -- 4 bytes */
	ENETPAD(epad5); 
	UReg8	rcvfs;    /* 06  receive frame status */
	ENETPAD(epad6);
	UReg8	fifofc;   /* 07  fifo frame count */
	ENETPAD(epad7);
	UReg8	ir;       /* 08  interrupt */
	ENETPAD(epad8);
	UReg8	imr;      /* 09  interrupt mask */
	ENETPAD(epad9);
	UReg8	pr;       /* 10  poll */
	ENETPAD(epad10);
	UReg8	biucc;    /* 11  bus interface unit configuration control */
	ENETPAD(epad11);
	UReg8	fifocc;   /* 12  fifo configuration control */
	ENETPAD(epad12);
	UReg8	maccc;    /* 13  media access control configuration control */
	ENETPAD(epad13);
	UReg8	plscc;    /* 14  physical layer signalling configuration control */
	ENETPAD(epad14);
	UReg8	phycc;    /* 15  physical layer configuration control */
	ENETPAD(epad15);
	UReg8	chipid1;  /* 16  chip identification LSB */
	ENETPAD(epad16);
	UReg8	chipid2;  /* 17  chip identification MSB */
	ENETPAD(epad17);
	UReg8	iac;      /* 18  internal address configuration */
	ENETPAD(epad18);
     	UReg8	res1;     /* 19  */
	ENETPAD(epad19);
	UReg8	ladrf;    /* 20  logical address filter -- 8 bytes */
	ENETPAD(epad20);
	UReg8	padr;     /* 21  physical address -- 6 bytes */
	ENETPAD(epad21);
	UReg8	res2;     /* 22  */
	ENETPAD(epad22);
	UReg8	res3;     /* 23  */
	ENETPAD(epad23);
	UReg8	mpc;      /* 24  missed packet count */
	ENETPAD(epad24);
	UReg8	res4;     /* 25  */
	ENETPAD(epad25);
	UReg8	rntpc;    /* 26  runt packet count */
	ENETPAD(epad26);
	UReg8	rcvcc;    /* 27  receive collision count */
	ENETPAD(epad27);
	UReg8	res5;     /* 28  */
	ENETPAD(epad28);
	UReg8	utr;      /* 29  user test */
	ENETPAD(epad29);
	UReg8	res6;     /* 30  */
	ENETPAD(epad30);
	UReg8	res7;     /* 31  */
}mace_board;

/*
 * Chip Revisions..
 */

#define	MACE_REVISION_B0	0x0940
#define	MACE_REVISION_A2	0x0941

/* xmtfc */
#define XMTFC_DRTRY       0X80
#define XMTFC_DXMTFCS     0x08
#define XMTFC_APADXNT     0x01

/* xmtfs */
#define XMTFS_XNTSV  	0x80
#define XMTFS_XMTFS  	0x40
#define XMTFS_LCOL   	0x20
#define XMTFS_MORE   	0x10
#define XMTFS_ONE    	0x08
#define XMTFS_DEFER  	0x04
#define XMTFS_LCAR   	0x02
#define XMTFS_RTRY   	0x01

/* xmtrc */
#define XMTRC_EXDEF  0x80

/* rcvfc */
#define RCVFC_LLRCV       0x08
#define RCVFC_M_R         0x04
#define RCVFC_ASTRPRCV    0x01

/* rcvfs */
#define RCVFS_OFLO   	0x80
#define RCVFS_CLSN   	0x40
#define RCVFS_FRAM   	0x20
#define RCVFS_FCS    	0x10
#define RCVFS_REVCNT 	0x0f

/* fifofc */
#define	FIFOCC_XFW_8	0x00 
#define	FIFOCC_XFW_16	0x40 
#define	FIFOCC_XFW_32	0x80 
#define	FIFOCC_XFW_XX	0xc0 
#define	FIFOCC_RFW_16	0x00 
#define	FIFOCC_RFW_32	0x10 
#define	FIFOCC_RFW_64	0x20 
#define	FIFOCC_RFW_XX	0x30 
#define FIFOCC_XFWU	0x08	
#define FIFOCC_RFWU	0x04	
#define FIFOCC_XBRST	0x02	
#define FIFOCC_RBRST	0x01	


/* ir */
#define IR_JAB    	0x80
#define IR_BABL   	0x40
#define IR_CERR   	0x20
#define IR_RCVCCO 	0x10
#define IR_RNTPCO 	0x08
#define IR_MPCO   	0x04
#define IR_RCVINT 	0x02
#define IR_XMTINT 	0x01

/* imr */
#define IMR_MJAB    	0x80
#define IMR_MBABL   	0x40
#define IMR_MCERR   	0x20
#define IMR_MRCVCCO 	0x10
#define IMR_MRNTPCO 	0x08
#define IMR_MMPCO   	0x04
#define IMR_MRCVINT 	0x02
#define IMR_MXMTINT 	0x01

/* pr */
#define PR_XMTSV  	0x80
#define PR_TDTREQ 	0x40
#define PR_RDTREQ 	0x20

/* biucc */
#define BIUCC_BSWP        0x40
#define BIUCC_XMTSP04     0x00
#define BIUCC_XMTSP16     0x10
#define BIUCC_XMTSP64     0x20
#define BIUCC_XMTSP112    0x30
#define BIUCC_SWRST       0x01

/* fifocc */
#define FIFOCC_XMTFW08W    0x00
#define FIFOCC_XMTFW16W    0x40
#define FIFOCC_XMTFW32W    0x80

#define FIFOCC_RCVFW16     0x00     
#define FIFOCC_RCVFW32     0x10
#define FIFOCC_RCVFW64     0x20

#define FIFOCC_XMTFWU      0x08
#define FIFOCC_RCVFWU      0x04
#define FIFOCC_XMTBRST     0x02
#define FIFOCC_RCVBRST     0x01

/* maccc */
#define MACCC_PROM        0x80
#define MACCC_DXMT2PD     0x40
#define MACCC_EMBA        0x20
#define MACCC_DRCVPA      0x08
#define MACCC_DRCVBC      0x04
#define MACCC_ENXMT       0x02
#define MACCC_ENRCV       0x01

/* plscc */
#define PLSCC_XMTSEL      0x08
#define PLSCC_AUI         0x00
#define PLSCC_TENBASE     0x02
#define PLSCC_DAI         0x04
#define PLSCC_GPSI        0x06
#define PLSCC_ENPLSIO     0x01

/* phycc */
#define PHYCC_LNKFL       0x80
#define PHYCC_DLNKTST     0x40
#define PHYCC_REVPOL      0x20
#define PHYCC_DAPC        0x10
#define PHYCC_LRT         0x08
#define PHYCC_ASEL        0x04
#define PHYCC_RWAKE       0x02
#define PHYCC_AWAKE       0x01

/* iac */
#define IAC_ADDRCHG     0x80
#define IAC_PHYADDR     0x04
#define IAC_LOGADDR     0x02

/* utr */
#define UTR_RTRE        0x80
#define UTR_RTRD        0x40
#define UTR_RPA         0x20
#define UTR_FCOLL       0x10
#define UTR_RCVFCSE     0x08

#define UTR_NOLOOP      0x00
#define UTR_EXTLOOP     0x02
#define UTR_INLOOP      0x04
#define UTR_INLOOP_M    0x06

#define ENET_PHYADDR_LEN	6
#define ENET_HEADER         14

#define BFRSIZ		2048
#define ETHER_ADD_SIZE	6	/* size of a MAC address */
#define	DSF_LOCK	1
#define DSF_RUNNING	2
#define MOD_ENAL 1
#define MOD_PROM 2

void InitMace(void);
void Sniff(void);	// Sniffs a packet out to cout
void DumpRegs(void);	// Dumps the MACE registers

#endif /* __MACE__ */