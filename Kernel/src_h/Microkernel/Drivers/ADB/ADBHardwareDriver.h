#ifndef __ADB_HARDWARE_DRIVER__
#define __ADB_HARDWARE_DRIVER__

#include "Driver.h"

enum /* Proper ADB commands */
{
	kADBCmdResetBus = 0x00,
	kADBCmdFlushADB = 0x01,
	kADBCmdWriteADB = 0x08,
	kADBCmdReadADB = 0x0C
};

enum /* Several pseudo commands */
{
	kADBNoCmd = 0,
	kADBPseudoCmdStartStopAutoPoll,
	kADBPseudoCmdSetDeviceList,
	kADBPseudoCmdSetAutoPollRate,
	kADBPseudoCmdPowerDown,
	kADBPseudoCmdRestart
};

enum ADBResultCode
{
	kADBResultOK = 0,
	kADBResultDeviceInUse,
	kADBResultDeviceNotPresent,
	kADBResultTimeout,
	kADBResultUnknown,
	kADBResultPacketRequestError,
	kADBResultBusError,
	kADBResultNotDone
};

enum ADBIOCommandType
{
	kArgCommand = 0,
	kBufferCommand
};

typedef struct
{
	UInt8			numArgs, arg[2];
} ADBIOCommandArgsInfo;

typedef struct
{
	UInt32		len;
	UInt8*		buffer;
} ADBIOCommandBufferInfo;

class ADBHardwareDriver;
class ADBHardware;

class ADBIOCommand : public IOCommand
{
public:
	friend class			ADBHardwareDriver;
	
	UInt16				command;
	Boolean				isADBCommand;
	ADBIOCommandType		type;
	union
	{
		ADBIOCommandArgsInfo	info;
		ADBIOCommandBufferInfo	buffer;
	} data;
	
	ADBResultCode			result;
	UInt8*				reply;
	UInt32				replyLen;
	
	UInt32				driver1, driver2;			// For hardware driver use only
	
						ADBIOCommand();
						
	void					MakeStartStopAutoPoll( Boolean startAutoPoll );
	void					MakeSetDeviceList( UInt16 deviceList );
	void					MakeSetAutoPollRate( UInt8 rate );
	void					MakePowerDown();
	void					MakeRestart();
	
public:
						ADBIOCommand( UInt16 command, UInt8 numArgs = 0,
							UInt8 arg1 = 0, UInt8 arg2 = 0 );
						ADBIOCommand( UInt16 command, UInt32 len, UInt8* buffer );
	virtual				~ADBIOCommand();
	
	Boolean				GetResult( UInt8& result );
	Boolean				GetReply( UInt8* reply, UInt32& replyLen );

	virtual UInt32			ioError();
};

class ADBHardwareDriver : public IOCommandDriver
{
protected:
	ADBHardware			*adb;
	
public:
						ADBHardwareDriver( ConstASCII8Str driverName, ADBHardware *adb );
	
	virtual Boolean			sendCommand( ADBIOCommand* theCommand ) = 0;
	
	// For handling no interrupt mode
	virtual void			checkInterrupt() = 0;
	virtual void			toggleInterruptMode( Boolean noInterruptMode ) = 0;
	
	virtual ADBIOCommand*	SetAutoPollState( Boolean on );
	virtual ADBIOCommand*	SetAutoPollRate( UInt8 rate );
	virtual ADBIOCommand*	SetDeviceList( UInt16 deviceList );
	virtual ADBIOCommand*	PowerDown();
	virtual ADBIOCommand*	Restart();
};

#endif /* __ADB_HARDWARE_DRIVER__ */