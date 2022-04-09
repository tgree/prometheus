#include "SOS.h"
#include "Kernel Console.h"
#include "Command Line.h"
#include "SOSMouse.h"
#include "Login.h"
#include "NKProcesses.h"

User*	stupidOSUser;
Process*	stupidOSProcess;

void main(User* theUser);

void main(User* theUser)
{
	stupidOSUser = theUser;
	stupidOSProcess = CurrProcess::process();
	
	stupidOSUser->console << "Initializing StupidOS services..." << newLine << newLine;
	if(stupidOSUser->loginLocation == kDesktopLogin)	// Mouse only available on a desktop login
		InitMouseHandler();
	
	stupidOSUser->console << "Welcome to the Stupid OS command line, " << stupidOSUser->name() << "!" << newLine << newLine;
	InitCommandLine();
}
