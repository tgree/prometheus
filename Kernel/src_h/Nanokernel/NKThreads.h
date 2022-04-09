/*
	NKThreads.h
	Copyright © 1998 by Terry Greeniaus

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
	Patrick Varilly		-	Sunday, 9 Jan 2000	-	Modified to use timer instead of decrementor (preparing for major changes)
	Patrick Varilly		-	Monday, 10 Jan 2000-	Major changes started:
											- blockQuerying() is gone (use a loop with blockForNS or, better yet, rewrite
											   to not busy-wait)
	Patrick Varilly		-	Tuesday, 11 Jan 2000-	Gave up modifying code.  Starting rewrite of scheduler
*/
#ifndef __NK_THREADS__
#define __NK_THREADS__

#include "Driver.h"
#include "NKTimers.h"

class ThreadTimer : public NKTimer
{
private:
	class Thread			*myThread;
public:
						ThreadTimer(Thread *inThread);
	virtual void			signal();
};

// Established priorities
// Normal priorities (i.e. 1 to 99) are chosen using lottery scheduling, higher priority giving more chance
// Real-time priorities (i.e. 100 to 199) are chosen by the thread with the highest priority, which must voluntarily
// relinquish control.  If there are several RT threads with the same priority, they are run in round-robin fashion,
// but they still must relinquish control voluntarily for the next equal-priority thread to execute.
enum
{
	kPriorityMin = 1,
	kPriorityMax = 199,
	kPriorityKernel = kPriorityMax,
	kPriorityDriver = kPriorityKernel-1,
	
	kPriorityLow = 1,
	kPriorityNormal = 10,
	kPriorityUrgent = 50,
	kPriorityRTLow = 100,
	kPriorityRTNormal = 110,
	kPriorityRTUrgent = 150
};

class Semaphore;

class Thread : public KernelObject
{
	// WARNING:  All threads MUST be allocated in the Kernel memory space.  Meaning you MUST allocate it
	// with operator new unless you are in the kernel!!!!!!!!!!!!!!!!!!!!!!!!!!
private:
	PPCRegisters		savedRegs;		// State of processor at last thread switch
	
	ASCII8			name[MAX_KSTR_LEN+1];// The thread's name
	Process*			process;			// A pointer to the process that spawned this thread
	Thread*			nextInProcess;		// A pointer to the next thread (this is a per-process queue)
	Thread			*next, *prev;		// Pointers to next and previous threads in current queue
	
	Float64			quantumTime;		// Time when thread's quantum expires (in ns)
	UInt32			priority;
	void*			stack;
	UInt8			state;
	Boolean			deleteWhenDone;
	
	class IOCommand*	ioCmd;
	ThreadTimer		sleepTimer, timeoutTimer;
	Boolean			timeout;
	Semaphore		*waitSem;
	Boolean			gotSemaphore;
	
	Thread();							// Only to be called by IdleThread::IdleThread()!!
	Thread(void* stack);				// Only to be called from NKInitThreads(), for the kernel thread!
	Thread(UInt32 stackLen,UInt32 priority,Process* parent,ConstASCII8Str name);
									// Only to be called when the kernel creates a new Process!!!
	
	void				init(UInt32 _stackLen,UInt32 _priority,Process* _parent,ConstASCII8Str _name,Boolean _deleteWhenDone);
	void				init(void* _stack,void* _stackBase,UInt32 _priority,Process* _parent,ConstASCII8Str _name,Boolean _deleteWhenDone);
	
	void				addToReady();
	void				removeFromReady();
	static Thread*		chooseReady();
	
	void				addToRT();
	void				removeFromRT();
	static Thread*		chooseRT();
	
	void				putInReady();
	
	void				makeReady();
	void				makeNotReady();
	static Thread*		chooseThread();
	
	void				cleanupIO();
	void				doneIO();
	
	void				cleanupSem();
	
public:
	Thread(UInt32 stackLen,UInt32 priority,ConstASCII8Str name,Boolean deleteWhenDone = 0);
	virtual ~Thread();
	
	virtual	void		threadLoop(void) = 0;// Overload this in your derived thread class
	
			void		resume(void);		// Starts a thread if it has been suspended
			void		suspend(void);		// Marks a thread ineligible to execute
	
			void		blockForIO(IOCommand* cmd,Float64 nsTimeout = 0);
									// Marks the thread as blocked until this IOCommand completes,
									// or the request times out (no timeout if nsTimeout == 0)
	
	// In the sleep functions, making periodic true starts counting the delay from the time of the last sleep
	// so that it's possible to make a loop such that the inside is called on a periodic basis (subject to availability of CPU
	// time and the thread's priority).  If the last sleep wasn't periodic, the sleep works as usual.
	// Example:
	//	while(1) {
	//		nkVideo << "1 second!\n";		// This will be called at regular 1s intervals
	//		sleepS(1,true);
	//	}
			void		sleepNS(Float64 ns, Boolean periodic = false);			// Blocks for ns nanoseconds
	inline	void		sleepUS(Float64 us, Boolean periodic = false) { sleepNS( us * 1000., periodic ); }
	inline	void		sleepMS(Float64 ms, Boolean periodic = false) { sleepNS( ms * 1000000., periodic ); }
	inline	void		sleepS(Float64 s, Boolean periodic = false) { sleepNS( s * 1000000000., periodic ); }
	inline	void		sleep(Float64 ms, Boolean periodic = false) { sleepMS( ms, periodic ); }
			void		wake(void);									// Wakes up if sleeping
	
	friend class KernelThread;
	friend class IdleThread;
	friend class Process;
	friend class CurrProcess;
	friend class ProcessWindow;
	friend void NKInitThreads(void);
	friend Thread* NKScheduleThread(void);
	friend static void ThreadStarter(Thread* thread);
	friend static void Yield(void);
	friend void NKDebuggerPrintThreads(void);
	friend void NKCheckSchedule( PPCRegisters* savedRegs );
	friend class ThreadTimer;
	friend class IOCommand;
	friend class Semaphore;
	friend class EventSemaphore;
};

class CurrThread
{
public:
	static	Thread*	thread();			// Returns a pointer to the current thread
	static	void		resume(void);		// Starts a thread if it has been suspended
	static	void		suspend(void);		// Marks a thread ineligible to execute
	
	static	void		blockForIO(IOCommand* cmd,Float64 nsTimeout=0);
									// Marks the thread as blocked until this IOCommand completes,
									// or the request times out
	
	static	void		sleepNS(Float64 ns);								// Blocks for ns nanoseconds
	inline static void	sleepUS(Float64 us)	{ sleepNS( us * 1000. ); }			// Microseconds
	inline static void	sleepMS(Float64 ms)	{ sleepNS( ms * 1000000. ); }		// Milliseconds
	inline static void	sleepS(Float64 s)	{ sleepNS( s * 1000000000. ); }	// Seconds
	inline static void	sleep(Float64 ms)	{ sleepMS( ms ); }				// Synonym for sleepMS
	static	void		wake(void);									// Wakes up if sleeping
	
	static	void		yield();			// Yields the current thread
};

class Semaphore
{
public:
					Semaphore( Int32 n );
	virtual			~Semaphore();
	
	virtual Boolean		down( Float64 nsTimeout = 0 );
	virtual void		up();
	
	friend class ThreadTimer;
	friend class Thread;
	
protected:
	virtual void		wait( Thread* thread, Float64 nsTimeout = 0 );
	virtual void		signal();
	virtual void		timedOut();
	
	Int32			_n;
	Thread			*waitQueue, *waitTail;
};

class MutexLock : public Semaphore
{
public:
	inline			MutexLock() : Semaphore(1) {}
	
	inline Boolean		lock( Float64 nsTimeout = 0 ) { return down(nsTimeout); }
	inline void			unlock() { up(); }
};

class EventSemaphore : public Semaphore
{
public:
	inline			EventSemaphore() : Semaphore(0) {}
	
protected:
	virtual void		signal();
	virtual void		timedOut();
};

class MutexLocker
{
public:
	inline			MutexLocker( MutexLock& inSem, Float64 nsTimeout = 0 ) : sem(inSem)
						{ myLock = false; myLock = sem.lock(nsTimeout); }
	inline			~MutexLocker() { if( myLock ) sem.unlock(); }
	
	inline Boolean		lock( Float64 nsTimeout = 0 ) { if(!myLock) myLock = sem.lock(nsTimeout); return myLock; }
	inline void			unlock() { if(myLock) { sem.unlock(); myLock = false; } }
	inline Boolean		locked() { return myLock; }
	
private:
	MutexLock&		sem;
	Boolean			myLock;
};

void NKInitThreads(void);
void NKCheckSchedule( PPCRegisters* savedRegs );
void NKGaugeThreads( void );

#endif
