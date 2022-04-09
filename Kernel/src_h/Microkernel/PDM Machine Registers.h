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
	Other sources			Project				Author			Notes
	===========			======				=====			====
	powermac_pdm.h		Mach DR2.1 update 6		???				Basically a copy of this file, with some modifications
															for Prometheus.
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright licenses to file.
*/
#ifndef __PDM__MACHINE__REGISTERS__
#define __PDM__MACHINE__REGISTERS__

/* Master Addresses */

#define PDM_IO_BASE_ADDR		0x50F00000
#define PDM_IO_SIZE			0x42000		/* 264K */

/* To get the physical address of an io device */
#define PDM_IO_PHYS_ADDR(addr)	(PDM_IO_BASE_ADDR + (addr))

/* All values from here on are offsets from PDM_IO_BASE_ADDR */

/* SCC registers (serial line) - physical addr is for probe */
#define PDM_SCC_BASE			(0x04000)

#define PDM_DMA_CTRL_BASE		(0x31000)

/* SCSI DMA control registers */
#define PDM_SCSI_DMA_CTRL_BASE	(PDM_DMA_CTRL_BASE+0x1000)
#define PDM_MACE_DMA_CTRL_BASE	(PDM_DMA_CTRL_BASE + 0xC20)

#define PDM_SCSI_DMA_CTRL_DIR_BIT   6
#define PDM_SCSI_DMA_CTRL_FLUSH_BIT 4
#define PDM_SCSI_DMA_CTRL_MODE1_BIT 3
#define PDM_SCSI_DMA_CTRL_MODE0_BIT 2
#define PDM_SCSI_DMA_CTRL_RUN_BIT   1
#define PDM_SCSI_DMA_CTRL_RESET_BIT 0

/* ASC registers(SCSI controller) */
#define PDM_ASC_BASE			(0x10000)	// Slow (5 MB/sec), external on 2 bus machine, internal/external on 1 bus machine, ASC controller
#define PDM_ASC2_BASE			(0x11000)	// Fast (10 MB/sec), internal on 2 bus machine, ASC controller

/* Interrupt control register */
#define PDM_ICR				(0x2A000)

/* Cuda registers */
#define PDM_CUDA_BASE			(0)

/* Interrupt controlling VIAs */
#define PDM_VIA1_BASE			(0)
#define PDM_VIA1_IER			(0x01C00)
#define PDM_VIA1_IFR			(0x01A00)
#define PDM_VIA1_PCR			(0x01800)
#define PDM_VIA1_AUXCONTROL	(0x01600)
#define PDM_VIA1_T1COUNTERLOW	(0x00800)
#define PDM_VIA1_T1COUNTERHIGH	(0x00A00)
#define PDM_VIA1_T1LATCHLOW	(0x00C00)
#define PDM_VIA1_T1LATCHHIGH	(0x00E00)

#define PDM_VIA2_BASE			(0x26000)
#define PDM_VIA2_IER			(0x26013)
#define PDM_VIA2_IFR			(0x26003)
#define PDM_VIA2_SLOT_IER		(0x26012)
#define PDM_VIA2_SLOT_IFR		(0x26002)

#define PDM_DMA_IFR			(0x2A008)

/* Am79c940 Media access controller for Ethernet - phys addr for autoconf */
#define PDM_MACE_BASE			(0x0A000)

#endif /* !__PDM__MACHINE__REGISTERS__ */