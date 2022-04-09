#include "IDE Device.h"
#include "CMD PCI 646.h"
#include "Chip Debugger.h"
#include "PCI.h"

// For the Chip Debugger
static RegisterDescriptor	ideRegisterDescriptor[]	=	{	CHIP_REGISTER(CMD646GeneralRegs,data,REG_SIDE_EFFECTS),
												CHIP_REGISTER(CMD646GeneralRegs,error_read,REG_READ_ONLY),
												CHIP_REGISTER(CMD646GeneralRegs,features_write,REG_WRITE_ONLY),
												CHIP_REGISTER(CMD646GeneralRegs,sectorCount,REG_NOFLAGS),
												CHIP_REGISTER(CMD646GeneralRegs,sector,REG_NOFLAGS),
												CHIP_REGISTER(CMD646GeneralRegs,cylinderLow,REG_NOFLAGS),
												CHIP_REGISTER(CMD646GeneralRegs,cylinderHigh,REG_NOFLAGS),
												CHIP_REGISTER(CMD646GeneralRegs,head,REG_NOFLAGS),
												CHIP_REGISTER(CMD646GeneralRegs,status_read,REG_READ_ONLY),
												CHIP_REGISTER(CMD646GeneralRegs,command_write,REG_WRITE_ONLY),
												LAST_REGISTER
											};

static RegisterDescriptor	ideRegisterDescriptor2[]	=	{	CHIP_REGISTER(CMD646AlternateRegs,alternateStatus_read,REG_READ_ONLY),
												CHIP_REGISTER(CMD646AlternateRegs,deviceControl_write,REG_WRITE_ONLY),
												LAST_REGISTER
											};

static RegisterDescriptor	pci646Regs[]			=	{	CHIP_REGISTER(CMD646PCIRegs,bmidecr0,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,mrdmode,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,bmidesr0,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,udidetcr0,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,dtrp0,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,bmidecr1,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,bmidesr1,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,udidetcr1,REG_NOFLAGS),
												CHIP_REGISTER(CMD646PCIRegs,dtpr1,REG_NOFLAGS),
												LAST_REGISTER
											};

// Prober so we can find CMD 646 devices
static CMD646IDEProber	cmc646ideProber;

Boolean CMD646IDEProber::probe(PCIDevice* pciDevice)
{
	if(pciDevice->readVendorID() == 0x1095 && pciDevice->readDeviceID() == 0x0646)
	{
		CMD646IDEDriver*	driver = new CMD646IDEDriver(pciDevice);
		machine.driverList.enqueue(driver);
		new IDEBus(driver,pciDevice->slotID());
		return true;
	}
	
	return false;
}

CMD646IDEDriver::CMD646IDEDriver(PCIDevice* theDev):
	IDEDriver("CMD PCI 646",theDev->readInterruptLine())	// PCIDevice::probe() has already set the interruptLine to the correct interrupt controller number
{
	// Save the PCIDevice
	dev = theDev;
	
	// Map our registers
	regs = (CMD646GeneralRegs*)dev->mapBaseAddr(0);
	altRegs = (CMD646AlternateRegs*)dev->mapBaseAddr(1);
	pciRegs = (CMD646PCIRegs*)dev->mapBaseAddr(4);
	
	// For the Chip Debugger
	new Chip("CMD 646 IDE General",ideRegisterDescriptor,regs);
	new Chip("CMD 646 IDE Alternate",ideRegisterDescriptor2,altRegs);
	new Chip("CMD 646 IDE PCI",pci646Regs,pciRegs);
	
	// Enable interrupts (this is probably specific to this particular PCI device).  IDEDriver() will use InterruptHandler::enable() and InterruptHandler::disable()
	// to really enable/disable interrupts.  We can just turn them on here and leave them on forever.  Or, we could be nice citizens and sub-class start() and
	// stop() and enable/disable them there (and call IDEDriver::start/stop() at the same time), but that's not really necessary.
	UInt8	mrdmode = ReadUReg8(&pciRegs->mrdmode);
	mrdmode &= ~(MRDMODE_INT_DISABLE1 | MRDMODE_INT_DISABLE2);
	WriteUReg8(mrdmode,&pciRegs->mrdmode);
}

CMD646IDEDriver::~CMD646IDEDriver()
{
}

UInt8 CMD646IDEDriver::readError()
{
	return ReadUReg8(&regs->error_read);
}

UInt8 CMD646IDEDriver::readSectorCount()
{
	return ReadUReg8(&regs->sectorCount);
}

UInt8 CMD646IDEDriver::readSector()
{
	return ReadUReg8(&regs->sector);
}

UInt8 CMD646IDEDriver::readCylinderLow()
{
	return ReadUReg8(&regs->cylinderLow);
}

UInt8 CMD646IDEDriver::readCylinderHigh()
{
	return ReadUReg8(&regs->cylinderHigh);
}

UInt8 CMD646IDEDriver::readHead()
{
	return ReadUReg8(&regs->head);
}

UInt8 CMD646IDEDriver::readStatus()
{
	return ReadUReg8(&regs->status_read);
}

void CMD646IDEDriver::writeData16(UInt16 data)
{
	WriteUReg16BE(data,(UReg16BE*)&regs->data);
}

void CMD646IDEDriver::writeFeatures(UInt8 data)
{
	WriteUReg8(data,&regs->features_write);
}

void CMD646IDEDriver::writeSectorCount(UInt8 data)
{
	WriteUReg8(data,&regs->sectorCount);
}

void CMD646IDEDriver::writeSector(UInt8 data)
{
	WriteUReg8(data,&regs->sector);
}

void CMD646IDEDriver::writeCylinderLow(UInt8 data)
{
	WriteUReg8(data,&regs->cylinderLow);
}

void CMD646IDEDriver::writeCylinderHigh(UInt8 data)
{
	WriteUReg8(data,&regs->cylinderHigh);
}

void CMD646IDEDriver::writeHead(UInt8 data)
{
	WriteUReg8(data,&regs->head);
}

void CMD646IDEDriver::writeCommand(UInt8 data)
{
	WriteUReg8(data,&regs->command_write);
}

void CMD646IDEDriver::writeDeviceControl(UInt8 data)
{
	WriteUReg8(data,&altRegs->deviceControl_write);
}

void CMD646IDEDriver::slamRead512(Ptr destLogical)
{
	SlamSector32((UReg32BE*)&regs->data,destLogical);
}

void CMD646IDEDriver::slamRead512LE(Ptr destLogical)
{
	SlamSector32LE((UReg32LE*)&regs->data,destLogical);
}