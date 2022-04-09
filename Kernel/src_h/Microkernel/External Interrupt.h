/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	???
	
	Version History
	============
	Terry Greeniaus		December 1997			Original creation of file
	Patrick Varilly			January, 1998			Ported code to PDM machines
	Patrick Varilly			Tue, 20 Jan 98			Original history tagging of file
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Terry Greeniaus	-	Friday, 26 June 98	-	Changed interrupt numbers to OpenFirmware ones
*/
#ifndef __EXTERNAL__INTERRUPT__
#define __EXTERNAL__INTERRUPT__

/*
	Hardware-independent interrupt types. These are generally OpenFirmware interrupt numbers
*/

enum {
	// No interrupt
	PMAC_DEV_NO_INT			= 64,
	
	// DMA Interrupts - these are also DMA channel numbers
	PMAC_DMA_SCSI0			= 0,		// External SCSI Bus
	PMAC_DMA_FLOPPY			= 1,
	PMAC_DMA_ETHERNET_TX	= 2,
	PMAC_DMA_IDE0			= 2,
	PMAC_DMA_ETHERNET_RX	= 3,
	PMAC_DMA_IDE1			= 3,
	PMAC_DMA_SCC_A_TX		= 4,		// Modem port (transmit?)
	PMAC_DMA_SCC_A_RX		= 5,		// Modem port (receive?)
	PMAC_DMA_SCC_B_TX		= 6,		// Printer port (transmit?)
	PMAC_DMA_SCC_B_RX		= 7,		// Printer port (receive?)
	PMAC_DMA_AUDIO_OUT		= 8,
	PMAC_DMA_AUDIO_IN		= 9,
	PMAC_DMA_SCSI1			= 10,	// Internal SCSI Bus
	
	/* Device interrupts */
	PMAC_DEV_SCSI0			= 12,	// Slow SCSI Bus, on both PCI and Nubus (this is internal and external on single bus machines)
	PMAC_DEV_SCSI1			= 13,	// Fast SCSI Bus, on both PCI and Nubus (with 2 busses)
	PMAC_DEV_IDE0			= 13,	// On PCI PowerBooks, IDE0 == SCSI1 interrupt
	PMAC_DEV_ETHERNET		= 14,
	PMAC_DEV_IDE1			= 14,	// On PCI PowerBooks, IDE1 == ETHERNET interrupt
	PMAC_DEV_SCC_A			= 15,	// Modem Port
	PMAC_DEV_SCC_B			= 16,	// Printer Port
	PMAC_DEV_AUDIO			= 17,
	PMAC_DEV_VIA			= 18,
	PMAC_DEV_FLOPPY			= 19,
	
	/* Add-on cards */
	PMAC_DEV_CARD0			= 20,
	PMAC_DEV_CARD1			= 21,
	PMAC_DEV_CARD2			= 22,
	PMAC_DEV_CARD3			= 23,
	PMAC_DEV_CARD4			= 24,
	PMAC_DEV_CARD5			= 25,
	PMAC_DEV_CARD6			= 26,	// This is the "control" video controller on my 7500
	PMAC_DEV_CARD7			= 27,
	PMAC_DEV_CARD8			= 28,	// This is the "planb" video controller on my 7500
	PMAC_DEV_CARD9			= 29,
	PMAC_DEV_CARD10			= 30,
	
	/* VIA interrupts - negate these to get their bit offset in the VIA interrupt register */
	PMAC_DEV_HZTICK			= -1,	// Never used in Mach - what is it?
	PMAC_DEV_CUDA			= -2,	// CUDA (ADB and others)
	PMAC_DEV_PMU			= -4,
	PMAC_DEV_TIMER2			= -5,	// Never used in Mach - what is it?
	PMAC_DEV_TIMER1			= -6,	// Timer attached to the VIA device
	
	/* Miscellaneous */
	PMAC_DEV_PDS			= -7,			// Processor Direct Slot: NuBus 601s only
	PMAC_DEV_NMI			= -8,			// NMI: Nubus only
	PMAC_DEV_VBL			= -9,			// Vertical Blanking Interrupt - not available on PCI????!?!
	
	/* A useful constant */
	PMAC_INT_MAX			= 41
};

/* Some NuBus and PDM aliases.. */
#define	PMAC_DEV_SCC		PMAC_DEV_SCC_A	// Older SCC chip on PDM machines

#define	PMAC_DEV_NUBUS0		PMAC_DEV_CARD0
#define	PMAC_DEV_NUBUS1		PMAC_DEV_CARD1
#define	PMAC_DEV_NUBUS2		PMAC_DEV_CARD2
#define	PMAC_DEV_NUBUS3		PMAC_DEV_CARD3

/*
	A class to register and handle interrupts.
	
	Usage: Create a sub-class of InterruptHandler which overrides the handleInterrupt method.
	The constructor of the subclass should pass the interrupt type (from the list above) to the InterruptHandler
	constructor. You must enable the interrupt for it to work. From then on, you can enable and/or disable it
	at will.
*/
class InterruptHandler
{
protected:
	InterruptHandler(UInt32 type0,UInt32 type1 = PMAC_DEV_NO_INT);
	virtual	~InterruptHandler();
	
public:
	UInt32	interruptType[2];	// Some guys get two interrupts
	
	virtual	void	handleInterrupt(void) = 0;
			void	enable();
			void	disable();
};

/*
	Prototypes
*/

// This function gets very early in the boot process to enable external interrupts and initialize private
// data structures
void	InitExternalInterrupt(void);

#endif /* !__EXTERNAL__INTERRUPT__ */
