#ifndef __NEW_CUDA_DRIVER__
#define __NEW_CUDA_DRIVER__

#include "ADBHardwareDriver.h"
#include "External Interrupt.h"
#include "VIA Chip.h"
#include "NKMachineInit.h"
#include "Time.h"

// Bit operations (WriteUReg8 and ReadUReg8 do eieio, so we don't do it here)

#define BIT_MASK(x)		(1 << (x))								// Get the bitmask for bit x (0=lsb)
#define SET_BIT(x,y)		WriteUReg8(ReadUReg8(&x) | BIT_MASK(y),&x)
															// Set bit y in variable x
#define CLR_BIT(x,y)		WriteUReg8(ReadUReg8(&x) & ~BIT_MASK(y),&x)
															// Clear bit y in variable x
#define TOGGLE_BIT(x,y)		WriteUReg8(ReadUReg8(&x) ^ BIT_MASK(y),&x)
															// Toggle bit y in variable x
#define SET_TWO_BITS(x,y,z)	WriteUReg8(ReadUReg8(&x) | BIT_MASK(y) | BIT_MASK(z),&x)
															// Set bits y and z in variable x
#define CLR_TWO_BITS(x,y,z)	WriteUReg8(ReadUReg8(&x) & (~BIT_MASK(y) & ~BIT_MASK(z)),&x)
															// Clear bits y and z in variable x
#define SET_CLR_BITS(x,y,z)	WriteUReg8((ReadUReg8(&x) & ~BIT_MASK(y)) | BIT_MASK(z)),&x)
															// Set bit y and clear bit z in variable x
#define SET_SET_CLR(a,x,y,z)	WriteUReg8((ReadUReg8(&a) | BIT_MASK(x) | BIT_MASK(y)) & ~BIT_MASK(z),&a)
#define IS_BIT_SET(x,y)		((ReadUReg8(&x) & BIT_MASK(y)) != 0)		// Is bit y in variable x set?
#define IS_BIT_CLR(x,y)		((ReadUReg8(&x) & BIT_MASK(y)) == 0)		// Is bit y in variable x cleared?
#define VIA_SET_BIT(x,y)	WriteUReg8(0x80 | BIT_MASK(y),&x)		// VIA way of setting bit y in variable x
#define VIA_CLR_BIT(x,y)	WriteUReg8(0x7F & BIT_MASK(y),&x)		// VIA way of setting bit y in variable x

// Special bits in CUDA registers

enum
{
	// Auxillary control register
	kCudaClockBit1 = 2,
	kCudaClockBit2 = 3,
	kCudaDirectionBit = 4,

	// Data/Dir B register (these bits are active low: 0 is on, 1 is off)
	kCudaTransferRequestBit = 3,		// In
	kCudaTransferAcknowledgeBit = 4,	// Out
	kCudaTransferInProgressBit = 5,	// Out

	// Interrupts
	kCudaInterruptBit = 2
};

// A CUDA packet. Currently, a packet can be no bigger than 256 bytes (most packets are barely 5 bytes big!)
enum { kPacketMaxSize = 256 };
typedef struct CUDAPacket
{
	UInt16		dataSize, dataDone;
	UInt8		data[kPacketMaxSize];
} CUDAPacket;

class CUDADriver : public ADBHardwareDriver,
				public InterruptHandler
{
protected:
	VIA_Chip				*theChip;

	// Data send/receive
	inline void				sendByte( UInt8 x )			{ WriteUReg8(x,&theChip->shift); }
	inline void				readByte( UInt8& x)			{ x = ReadUReg8(&theChip->shift); }
	inline void				clearInterrupt()			{ UInt8 val; readByte( val ); }

	// Status management
	inline void				setDataDirectionToInput()		{CLR_BIT(theChip->auxillaryControl,kCudaDirectionBit); }
	inline void				setDataDirectionToOutput()	{SET_BIT(theChip->auxillaryControl,kCudaDirectionBit); }
	inline void				transferInProgress()			{CLR_BIT(theChip->dataB,kCudaTransferInProgressBit); }
	inline void				transferNotInProgress()		{SET_BIT(theChip->dataB,kCudaTransferInProgressBit); }
	inline void				terminateTransaction()		{SET_TWO_BITS(theChip->dataB,kCudaTransferInProgressBit,kCudaTransferAcknowledgeBit); }
	inline void				acknowledgeTransfer()		{TOGGLE_BIT(theChip->dataB,kCudaTransferAcknowledgeBit); }
	inline void				setTransferAcknowledge()	{CLR_BIT(theChip->dataB,kCudaTransferAcknowledgeBit); }
	inline void				clearTransferAcknowledge()	{SET_BIT(theChip->dataB,kCudaTransferAcknowledgeBit); }
	inline void				synchronize()				{setTransferAcknowledge(); }
	inline void				desynchronize()			{clearTransferAcknowledge(); }
	inline void				enableInterrupts()			{VIA_SET_BIT(theChip->interruptEnable,kCudaInterruptBit); }
	inline void				disableInterrupts()			{VIA_CLR_BIT(theChip->interruptEnable,kCudaInterruptBit); }
	inline void				getInterruptStatus(UInt8& status )
												{ status = (~ReadUReg8(&theChip->dataB) & (BIT_MASK(kCudaTransferInProgressBit) | BIT_MASK(kCudaTransferRequestBit)))
													| (ReadUReg8(&theChip->auxillaryControl) & BIT_MASK(kCudaDirectionBit)); }

	// Special condition testing
	inline Boolean			isTransferBeingRequested()	{ return IS_BIT_CLR(theChip->dataB,kCudaTransferRequestBit); }
	inline Boolean			isTransferInProgress()		{ return IS_BIT_CLR(theChip->dataB,kCudaTransferInProgressBit); }
	inline Boolean			isPendingInterrupt()			{ return IS_BIT_SET(theChip->interruptFlag,kCudaInterruptBit); }
	inline Boolean			isInterruptEnabled()			{ return IS_BIT_SET(theChip->interruptEnable,kCudaInterruptBit); }

	// Wait for conditions (with timeout in ns). Return false if timeout is met but condition is not. Return true otherwise
	inline Boolean			waitForTransferRequest(UInt32 timeout_ns )
							{
								Float64	timeoutEnd = GetTime_ns() + timeout_ns;
								while(!isTransferBeingRequested() )
								{
									if(GetTime_ns() > timeoutEnd )
										return false;
								}
								return true;
							}
	inline Boolean			waitForNoTransferRequest(UInt32 timeout_ns )
							{
								Float64	timeoutEnd = GetTime_ns() + timeout_ns;
								while(isTransferBeingRequested() )
								{
									if(GetTime_ns() > timeoutEnd )
										return false;
								}
								return true;
							}
	inline Boolean			waitForInterrupt( UInt32 timeout_ns )
							{
								Float64	timeoutEnd = GetTime_ns() + timeout_ns;
								while(!isPendingInterrupt() )
								{
									if(GetTime_ns() > timeoutEnd )
										return false;
								}
								return true;
							}
	
        // Internal variables
        CUDAPacket*			devPacket;
        CUDAPacket*			replyPacket;
        ADBIOCommand*		currCommand;
        Int8					state;
        UInt8				intStatus;
        Boolean				isDevMsg;
        Int8					filler;
        UInt32				filler2, filler3;
        Int8					filler4, filler5;
        Boolean				ready;
        
	Int8					synchronizeWithCUDA();
	void					internalSend();
	void					sendSecondByte();
	void					sendNextByte();
	void					startReply();
	void					readNextByte();
	void					processResponse();
	void					startDeviceMessage();

public:
						CUDADriver( ADBHardware* adb,MachineDevice<VIA_Chip>* device );

	virtual Boolean			sendCommand( ADBIOCommand* theCommand );

	// For handling no interrupt mode
	virtual void			checkInterrupt();
	virtual void			toggleInterruptMode( Boolean noInterruptMode );

	virtual void			handleInterrupt();

	virtual void			startAsyncIO(IOCommand* cmd);

	// Stuff for Driver
	virtual		void		initialize();	// Initialize and turn off interrupts
	virtual		void		start();		// Generate interrupts and enable()
	virtual		void		stop();		// Turn off interrupts and disable()
};

#endif /* !__NEW_CUDA_DRIVER__ */