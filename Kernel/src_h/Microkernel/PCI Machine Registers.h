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
	powermac_pci.h		Mach DR2.1 update 6		???				Basically a direct copy (also based on older
															DR2.1 release)
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added copyright notice to file.
*/
#ifndef __PCI__MACHINE__REGISTERS__
#define __PCI__MACHINE__REGISTERS__

/* Master Values */

#define PCI_IO_BASE_ADDR		0xF3000000
#define NEWWORLD_IO_BASE_ADDR	0x80800000
#define PCI_IO_SIZE				0x20000		// 128K

/* To get the physical address of an io device */
#define PCI_IO_PHYS_ADDR(addr)		(PCI_IO_BASE_ADDR + (addr))
#define NEWWORLD_IO_PHYS_ADDR(addr)	(NEWWORLD_IO_BASE_ADDR + (addr))

/* All values from here on are offsets from PCI_IO_BASE_ADDR */

/* DMA */

#define PCI_DMA_BASE			(0x8000)

/* Interrupts */
#define PCI_INTERRUPT_EVENTS	(0x00020)
#define PCI_INTERRUPT_MASK		(0x00024)
#define PCI_INTERRUPT_CLEAR		(0x00028)
#define PCI_INTERRUPT_LEVELS	(0x0002C)

/* SCC registers (serial line) */
#define PCI_SCC_BASE			(0x12000)

/* ASC registers (external scsi) */
#define PCI_ASC_BASE			(0x10000)

/* MESH (internal scsi) controller */
#define PCI_MESH_BASE			(0x18000)
#define PB_PCI_MESH_BASE		(0x10000)

/* IDE controllers */
#define PB_PCI_IDE0_BASE		(0x20000)
#define PB_PCI_IDE1_BASE		(0x21000)

/* audio controller */
#define PCI_AUDIO_BASE			(0x14000)

/* floppy controller */
#define PCI_FLOPPY_BASE		(0x15000)

/* Ethernet controller */
#define PCI_ETHERNET_BASE		(0x11000)
#define PCI_ETHERNET_ADDR		(0x19000)

/* VIA controls, misc stuff (including CUDA) */
#define PCI_VIA_BASE			(0x16000)
#define PCI_CUDA_BASE			PCI_VIA_BASE
#define PB_PCI_VIA_BASE		PCI_VIA_BASE
#define PB_PCI_PMU_BASE		PCI_VIA_BASE

#define PCI_VIA1_AUXCONTROL	(PCI_VIA_BASE+0x01600)
#define PCI_VIA1_T1COUNTERLOW	(PCI_VIA_BASE+0x00800)
#define PCI_VIA1_T1COUNTERHIGH	(PCI_VIA_BASE+0x00A00)
#define PCI_VIA1_T1LATCHLOW	(PCI_VIA_BASE+0x00C00)
#define PCI_VIA1_T1LATCHHIGH	(PCI_VIA_BASE+0x00E00)

#define PCI_VIA1_IER			(PCI_VIA_BASE_PHYS+0x01C00)
#define PCI_VIA1_IFR			(PCI_VIA_BASE_PHYS+0x01A00)
#define PCI_VIA1_PCR			(PCI_VIA_BASE_PHYS+0x01800)

/* Am79c940 Media access controller for Ethernet */
#define PCI_MACE_BASE			(0x11000)

/* Interrupt controller */
#define PCI_INTERRUPT_BASE		(PCI_INTERRUPT_EVENTS)

#endif /* !__PCI__MACHINE__REGISTERS__ */