/*
	NKThreads.cp
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
	Terry Greeniaus	-	Friday, 4 Sept. 98	-	Made threads use the ProcessorInfo structure for multi-processing purposes
	Patrick Varilly		-	Sunday, 9 Jan 2000	-	Modified to use timer instead of decrementor (preparing for major changes)
	Patrick Varilly		-	Monday, 10 Jan 2000-	Major changes started:
											- blockQuerying() is gone (use a loop with blockForNS or, better yet, rewrite
											   to not busy-wait)
	Patrick Varilly		-	Tuesday, 11 Jan 2000-	Gave up modifying code.  Starting rewrite of scheduler
	Patrick Varilly		-	Thu, 13 Jan 2000	-	Finished rewrite, changed I/O handling in lots of places, now debugging (hard!)
	Patrick Varilly		-	Mon, 17 Jan 2000	-	WOOHOO!!! Complete, with real semaphores, and *stable* at last!!!  Must reduce
											thread switching time ASAP, though, it's rather high (unnecessary copying of
											whole CPU state): about 11us in my iMac.  I believe we can get it to go under 8us.
	Patrick Varilly		-	Mon, 31 Jan 2000	-	Fixed silly bug in Semaphore::signal().  Now we *always* wake up the first thread
											in the wait-queue if there are any threads waiting (as it should be).
*/
#include "NKThreads.h"
#include "NKMachineInit.h"
#include "NKProcessors.h"
#include "Macros.h"
#include "ANSI.h"
#include "Streams.h"

#define DEBUG_THREADS			0
#if DEBUG_THREADS
// Thread debugging facilities.  Set the appropiate values to 1 to see the corresponding event.  DEBUG_THREADS acts as a global switch
#define DEBUG_SLOW			1		// Use a slow thread quantum (usually one seconds, see below)
#define DEBUG_CREATE			1		// Messages related to thread creation
#define DEBUG_DELETE			1		// Messages related to thread deletion
#define DEBUG_PREEMPT			1		// Message when a thread switch occurs
#define DEBUG_QUANTUM			1		// Message when quantum expires and quantum doesn't switch threads (e.g. only 1 thread)
#define DEBUG_RESUMESUSPEND	1		// Message on resume/suspend
#define DEBUG_READY			1		// Message when a thread becomes ready, possibly interrupting a CPU
#define DEBUG_IO				1		// Messages when blocked for I/O, when I/O is done and when I/O timeout occurs
#define DEBUG_SLEEP			1		// Messages when sleep is requested and sleep finishes
#define DEBUG_SEM				1		// Messages on semaphore acquisition, release, wait and timeout
#else
#define DEBUG_SLOW			0
#define DEBUG_CREATE			0
#define DEBUG_DELETE			0
#define DEBUG_PREEMPT			0
#define DEBUG_QUANTUM			0
#define DEBUG_RESUMESUSPEND	0
#define DEBUG_READY			0
#define DEBUG_IO				0
#define DEBUG_SLEEP			0
#define DEBUG_SEM				0
#endif

#if DEBUG_SLOW
#define THREAD_QUANTUM		ONE_SEC_NANOSECONDS
#else
#define THREAD_QUANTUM		ONE_MS_NANOSECONDS
#endif

// This is the MSR all new threads start with
const UInt32					kNewThreadMSR = 0x00009030;	// exceptions on, priviledged, IR/DR enabled, FPU disabled,
														// machine check enabled

// Possible thread states
enum
{
	threadRunning = 0,			// This is the current thread on one processor
	threadReady = 1,			// Thread is not current, but is otherwise elegible for CPU time
	threadBlocked = 2,			// Thread is blocked for I/O
	threadWaiting = 3,			// Thread is waiting for a semaphore to signal
	threadSuspended = 4,		// Thread is not elegible for CPU time, and must be reactivated manually
	threadSleeping = 5,			// Thread will be moved into ready state after a set amount of time has passed
	
	threadPendingDelete = 6,		// Thread is done and should be deleted ASAP
	threadDone = 7				// Thread has finished and cannot be resumed
};

struct KernelThread	:	public Thread
{
	KernelThread(void* _stack);
	virtual	void	threadLoop(void);
};

struct IdleThread	:	public Thread
{
	IdleThread();
	virtual	void	threadLoop(void);
};

static void		NKStartThreads();
static void		ThreadStarter(Thread* thread);
static void		Yield(void);
static void		DoYield(void);
static void		NKMicrokernelResume(void);
Thread*			NKScheduleThread(void);
static void		clearReservation();
static UInt32		ThreadRand();

// A note on lottery scheduling:  Right now, the kernel keeps a doubly-linked list of ready threads (not including the running ones).  To choose
// a thread, it picks a random number between 1 and totalPriority and then sequentially scans the queue until the cumulative priority
// is equal to or greater than the random number.  Obviously, this is ok for a small amount of ready threads (e.g. 20).  It simplifies putting
// threads into the ready queue and it simplifies choosing a thread.  However, when a large number of threads is elegible for execution,
// scheduling overhead could become prohibitive.  If this turns out to be an issue, we should something smarter than a sequential queue.
// Maybe a binary tree that keeps a sum of the priorities below each branch?  I dunno, but I've tried to separate the code for dealing with
// the ready queue into the three functions addToReady, removeFromReady and chooseReady.

// Also, keep in mind that most of the time, most threads are inelegible for execution, and this applies to all systems, not just Prometheus.
// They could be waiting for a timer to expire or, most likely, waiting for user input.  For example, my local ISP's Linux system usually
// has around 70 or so processes running, of which only 3 or 4 are elegible for execution at any one time (obviously this number
// increases temporarily to about 6 or 7 when something special happens, e.g. a user checks his mail, or a web page is requested)

// Finally [if you've read this far along], real-time threads are also kept in a linear queue, but this is much less of an issue.  The only
// possibly time-consuming task is adding a thread to the queue, as it must be kept sorted in descending order, with the inserted thread
// being the last of the threads with equal priority (to schedule equal priority threads in a round-robin fashion).  Otherwise, removal
// is simply unlinking (which is easy since the queue is doubly-linked) and only the first thread (i.e. the highest priority thread) is ever
// consulted.  However, I've also separated the code for this queue in addToRT, removeFromRT and chooseRT.

static SpinLock		scheduleLock;
static Thread		*rtQueue = nil;		// Queue of available (ready but not current) real time threads, sorted in descending priority
static Thread 		*readyQueue = nil;	// Queue of available (ready but not current) normal thread (priority < 100)
static UInt32		totalPriority = 0;	// Sum of priorities of threads in normalQueue, for lottery scheduling,
								// (on worst case, you need about 43.3 million threads going for this to overflow; even if each
								// thread took up only one byte [it takes up about 500], a 32-bit machine could not address enough
								// physical memory to hold that many threads.  And any case, even in 64-bit machines,
								// 43.3 million threads is an unlikely load =)
static Thread		*deleteQueue = nil;	// Queue of threads awaiting automatic incineration =)
static UInt32		threadRandSeed = 0;	// Always start on 0, gets random as hardware event timings change the order of startup process

// Stuff for clearing any reservations when switching processes (so a lwarx of one thread isn't paired to the stwcx. of another one)
static UInt32		reservationGranule[8];	// Get an entire cache line for the reservation
static UInt32*		reservationScratch;

// Special stuff for resuming to the kernel when threads have started up
static void*		startThreadsPC;
static void*		startThreadsSP;
static void*		startThreadsLR;
static UInt32		startNonVolatileGPRS[19];
static UInt32		startCR;
static UInt32		threadsReady = 0;

#define NUM_GAUGE_LOOPS	40000

void NKGaugeThreads(void)
{
	// Gauge the speed of GetTime_ns
	Float64				start,end,diff,avg;
	UInt32				i,n;
	
for( n = 0; n < 2; n++ )
{
	cout << "Gauging GetTime_ns: ";
	start = GetTime_ns();
	for( i = 0; i < NUM_GAUGE_LOOPS; i++ )
		GetTime_ns();
	end = GetTime_ns();
	diff = end - start;
	avg = diff/(NUM_GAUGE_LOOPS+1.);
	
	// We did NUM_GAUGE_LOOPS+1 "calls" to GetTime_ns().  The +1 accounts for the half between getting the time and returning
	// for start and then calculating the time for end (half + half = 1)
	
	cout << "Avg: " << (UInt32)avg << "ns\n";
	
	// Gauge the speed of thread switches
	cout << "Gauging thread switch time: ";
	start = GetTime_ns();
	for( i = 0; i < NUM_GAUGE_LOOPS; i++ )
		CurrThread::sleepNS(0);
	end = GetTime_ns();
	diff = end - start - avg;		// Account for two calls to GetTime_ns()
	cout << "Avg: " << (UInt32)(diff/(NUM_GAUGE_LOOPS+1.)) << "ns\n";
}
}

static void TestMe(void);

void NKInitThreads(void)
{
	// "Start" kernel thread
	Thread*	kernelThread = new KernelThread((void*)(_getSP() - 64));
	kernelThread->resume();
	
	// Create idle threads
	UInt32					n;
	for( n = 0; n < NKGetNumProcessors(); n++ )
	{
		ProcessorInfo			*info = NKGetProcessorInfo(n);
		info->idleThread = new IdleThread;
		//info->idleThread->resume();
	}
	
	// Man I had some bad arithmetic in the old threads stuff!
	// Or maybe I thought cachelines were only 16 bytes...  -TG
	reservationScratch = (UInt32*)( ((UInt32)reservationGranule + 31) & 0xFFFFFFF0 );	// reservationScratch points to the start of a scratch cache line for clearing reservations
	
	//_setDEC(0x7FFFFFFF);
	//startThreadsPC = (void*)FUNC_ADDR(TestMe);
	//threadsReady = 1;
	//_setDEC(0x80000000);
	
	NKStartThreads();
}

KernelThread::KernelThread(void* _stack):
	Thread(_stack)
{
}

static void TestMe(void)
{
	nkVideo << "I'm here!!!";
	nkVideo << "\n";
	for(;;)
		;
}

__asm__ void KernelThread::threadLoop(void)
{
	// This used to be in a separate (static) function, though I don't see the need for it -- Pat
	// Because C++ assembly functions are evil!!!! They have extra invisible parameters.  Also, to access
	// the objects members, you have to do funky pointer arithmetic to get at the right parts sometimes...
	// And also disassemble this function and see what you get!  Not quite what you expect! ;-P
	// Glue code that takes us back to NKStartThreads()
	lwz		r3,startThreadsPC(rtoc);
	mtlr		r3;
	blr;
}

static __asm__ void NKStartThreads()
{
	// Set up the kernel thread
	stw		sp,startThreadsSP(rtoc);
	mflr		r3;
	stw		r3,startThreadsLR(rtoc);
	bl		@getStartPC;
	mflr		r3;
	stw		r3,startThreadsPC(rtoc);
	lwz		r3,startNonVolatileGPRS(rtoc);
#if USE_MW_INST
	stmw	r13,0(r3);
#else
	stw		r13,0(r3)
	stw		r14,4(r3)
	stw		r15,8(r3)
	stw		r16,12(r3)
	stw		r17,16(r3)
	stw		r18,20(r3)
	stw		r19,24(r3)
	stw		r20,28(r3)
	stw		r21,32(r3)
	stw		r22,36(r3)
	stw		r23,40(r3)
	stw		r24,44(r3)
	stw		r25,48(r3)
	stw		r26,52(r3)
	stw		r27,56(r3)
	stw		r28,60(r3)
	stw		r29,64(r3)
	stw		r30,68(r3)
	stw		r31,72(r3)
#endif
	mfcr		r3;
	stw		r3,startCR(rtoc);
	
	// Ready to start
	li		r3,1
	stw		r3,threadsReady(rtoc);
	
	// Cause a decrementer exception now.  If a dec occurs between the previous store and the mtdec, the second dec will not occur
	lis		r3,0x7FFF;
	mtdec(r3);
	lis		r3,0x8000;			// Just flipping the high bit from 0 to 1 does the trick
	mtdec(r3);
@forEver:
	b		@forEver;	// Wait for the decrementer exception and the kernel thread to start up.
@getStartPC:
	blrl;
	
	// We resume executing here when the kernel thread starts up
	lwz		sp,startThreadsSP(rtoc);
	lwz		r3,startThreadsLR(rtoc);
	mtlr		r3;
	lwz		r3,startNonVolatileGPRS(rtoc);
#if USE_MW_INST
	lmw		r13,0(r3);
#else
	lwz		r13,0(r3);
	lwz		r14,4(r3);
	lwz		r15,8(r3);
	lwz		r16,12(r3);
	lwz		r17,16(r3);
	lwz		r18,20(r3);
	lwz		r19,24(r3);
	lwz		r20,28(r3);
	lwz		r21,32(r3);
	lwz		r22,36(r3);
	lwz		r23,40(r3);
	lwz		r24,44(r3);
	lwz		r25,48(r3);
	lwz		r26,52(r3);
	lwz		r27,56(r3);
	lwz		r28,60(r3);
	lwz		r29,64(r3);
	lwz		r30,68(r3);
	lwz		r31,72(r3);
#endif
	lwz		r3,startCR(rtoc);
	mtcr(r3);
	
	blr;
}

IdleThread::IdleThread():
	Thread()
{
}

void IdleThread::threadLoop(void)
{
	// This is turned by the compiler to "loop: b loop" (literally, not even a blr afterwards), which means we could simply save the
	// pc, msr and sr's of the idle thread, as no register info needs to be kept (a removal of 32 word accesses and 32 double-word
	// accesses, quite a lot).  We could also maintain just one global idle thread, not one idle thread per processor.  This is a possible
	// optimization, if we can verify that all platforms turn this into a cyclic branch which needs no register info.  I'll see how much
	// overhead the scheduler takes up, and examine if it's worth it to complicate the thread switch for this
	for(;;)
		;
}

// Threads *always* start in the function ThreadStarter, which gets passed a pointer to the thread.  Hence, the basic setup is:
// - Create a stack (or, for the initial kernel thread, just get a stack pointer)
// - If we created the stack, put a nil back-pointer on it and set sp to the stack pointer - 64 (as if we had started from limbo)
// - If we got passed a stack pointer, just place it unchanged into r1
// - Set r2 (rtoc) to the rtoc of ThreadStarter
// - Set r3 (1st parameter) to this, so ThreadStarter receives ur pointer
// - Set the pc to the address of ThreadStarter
// - Set the lr to nil
// - Copy the current MSR to the new thread (in new processes we should *at least* turn off the PR bit, so they run in User Mode)
// - Segment registers: this should really change:  for the kernel and idle threads, the current SRs are copied.  For normal threads,
//	only SR0 (for the segment from 0Mb to 256Mb) is copied, so that the kernel is always visible to any process (this should change when
//	we processes to interface the kernel via system calls). SR1 is also special.  It is the same for all processes below a process created
//	by the kernel (i.e. an operating system), so that an OS's processes all see segment 1 (256Mb to 512Mb) of their OS, but different
//	OS's have different SR1.  This should also change to link to the OS via a more general mechanism, such as shared libs, which doesn't
//	rely on programs having position-independent code (I'm thinking about ELF binaries here, which are linked at a static address).  All other
//	SRs are set up for user and supervisor access with execute priviledges, and the virtual segment ID set up properly

// From there on, the setup is more or less straightforward: The priority, the state and the current process of the thread are set up and
// the current process is informed of the new thread (so that the thread is added to its list of threads)

void
Thread::init(UInt32 _stackLen,UInt32 _priority,Process* _parent,ConstASCII8Str _name,Boolean _deleteWhenDone)
{
	Int8		*_stack, *_stackBase;
	_stack = new Int8[_stackLen];
	FatalAssert( _stack != nil );
	
	// Create a nil back frame
	_stackBase = _stack + _stackLen - 64;
	*((UInt32*)_stackBase) = nil;
	
	init(_stack,_stackBase,_priority,_parent,_name,_deleteWhenDone);
}

void
Thread::init(void* _stack,void* _stackBase,UInt32 _priority,Process* _parent,ConstASCII8Str _name,Boolean _deleteWhenDone)
{
	// Set up registers (see description above)
	savedRegs.r[1] = (UInt32)_stackBase;
	savedRegs.r[2] = FUNC_RTOC(ThreadStarter);
	savedRegs.r[3] = reinterpret_cast<UInt32>(this);
	savedRegs.srr0 /*pc*/ = FUNC_ADDR(ThreadStarter);
	if( threadsReady )
		savedRegs.srr1 = (kNewThreadMSR & ~0x00000040) | (GetMSR() & 0x00000040);	// Set MSR[IP] correctly
	else
		savedRegs.srr1 /*msr*/ = GetMSR();	// FIXME: Check if parent is kernel; clear PR bit if not
	savedRegs.lr = nil;
	
	UInt32				processID = _parent->processID();
	savedRegs.sr[0] = _getSR(0);
	savedRegs.sr[1] = (_parent->parent ? _getSR(1) : (0x60100000 | processID));
	for(Int32 i=2;i<16;i++)
		savedRegs.sr[i] = 0x60000000 | 0x00100000*i | processID;
	
	// Set up thread state
	stack = _stack;
	state = threadSuspended;
	timeout = false;
	FatalAssert( _priority <= kPriorityMax );
	priority = _priority;
	process = _parent;
	nextInProcess = nil;
	next = prev = nil;
	deleteWhenDone = _deleteWhenDone;
	strncpy(name,_name,MAX_KSTR_LEN);
	
	// Inform parent process
#if DEBUG_CREATE
{
	// Acquire lock so screen doesn't mess up
	CriticalSection				critical(scheduleLock);
	nkVideo << "Created thread " << greenMsg << name << whiteMsg << " (" << (UInt32)this << ")\n";
}
#endif
	_parent->threadCreated(this);
}

Thread::Thread()
	: sleepTimer(this), timeoutTimer(this)
{
	// This is only called for the idle thread, a thread which runs when all other threads are blocked
	// Stack could be as low as 148 bytes, but add a little cushion
	init(256,0,kernelProcess,"Idle Thread",false);
}

Thread::Thread(void* _stack)
	: sleepTimer(this), timeoutTimer(this)
{
	// This is only called for the kernel thread.
	init(_stack,_stack,kPriorityKernel,kernelProcess,"Kernel Thread",true);
}

Thread::Thread(UInt32 stackLen,UInt32 _priority,ConstASCII8Str _name,Boolean _deleteWhenDone)
	: sleepTimer(this), timeoutTimer(this)
{
	// This is called for generic threads.
	init(stackLen,_priority,CurrProcess::process(),_name,_deleteWhenDone);
}

Thread::Thread(UInt32 stackLen,UInt32 _priority,Process* parent,ConstASCII8Str _name)
	: sleepTimer(this), timeoutTimer(this)
{
	// This is called for process starter threads.
	Int8			*_stack, *_stackBase;
	{
		ProcessWindow	window(parent);
		_stack = new(parent) Int8[stackLen];
		_stackBase = _stack + stackLen - 64;
		*((UInt32*)_stackBase) = nil;	// nil back frame
	}
	
	init(_stack,_stackBase,_priority,parent,_name,true);
}

Thread::~Thread()
{
	// WARNING: You should *only* call delete *once* for any thread.  Trying to delete it twice is like sticking a fork into your toaster.
	// The kernel interface, whatever it may become, should prevent a process from deleting a thread twice (by returning an error the
	// second time), so that a faulty program (or deliberate tactic?) doesn't crash the machine.
	
	// WARNING #2: *NEVER* delete the current thread from a critical section.  If the current thread deletes itself, it *needs* to be
	// preempted *before* operator delete even returns.  Failure to do so will cause a silent crash!!!!!
	ProcessorInfo					*info;
	UInt32						n;
	UInt8						saveState;
	
	if( state != threadPendingDelete )
	{
		// No thread switch will occur on *any* processor as long as we have this lock (allows us to walk through all processors)
		CriticalSection				critical(scheduleLock);
		saveState = state;
		state = threadPendingDelete;	// Marking it as such prevents the scheduling of this thread by any other processor
								// after we release the lock
#if DEBUG_DELETE
		nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") scheduled for deletion\n";
#endif
		
		// Turn timers off (it's harmless if they get called, but try to avoid this)
		sleepTimer.remove();
		timeoutTimer.remove();
			
		// Make sure other processors don't find this in the ready queues
		if( saveState == threadReady )
			// If the thread is elegible for execution, make it inelegible
			makeNotReady();
		else if( state == threadBlocked )
			// If we're waiting for I/O, make sure the command doesn't answer to us
			cleanupIO();
			// Some other thread, or the process kill function, will have to free the I/O command memory
		else if( state == threadWaiting )
			// If we're waiting for a semaphore, remove ourselves from the semaphore's list of waiting threads
			cleanupSem();
		
		// Nothing need be done if we were suspended.  Yay.
		
		// Find out which processor this thread is still running on, if any
		if( saveState == threadRunning )
		{
			for( n = 0; n < NKGetNumProcessors(); n++ )
			{
				info = NKGetProcessorInfo(n);
				if( info->thread == this )
					break;
			}
			if( n == NKGetNumProcessors() )
				Panic("Thread::~Thread(): Thread marked as running but not current on any processor!");
			
			// If necessary, tell it to preempt the thread ASAP (we busy wait below, after releasing the schedule lock, for the
			// thread to be inactive)
			if( NKGetThisProcessorInfo()->thread != this )
				Panic("Thread::~Thread(): Don't know how to send IPIs!!!");
		}
		else
			n = NKGetNumProcessors();	// Rogue value, meaning "not on any processor"
		
		// Dirty Trick #1:  A thread never actually gets to delete its own thread object.  Rather, it's just scheduled for deletion
		// and preempted.  The scheduler then sees that the thread is in the delete queue and calls delete on it *again*.  The trick
		// is that it's actually safe to "abort" a delete here and "continue" it in the scheduler.  As I've found out through
		// disassembly (maybe this is part of Standard C++?), the destructor first adjusts the virtual table to that of the class for
		// which it is destructor.  Thus, a subclass of thread, when being deleted, will get its destructor called, get the thread preempted
		// and *then* class Thread's original destructor will do proper work.
		
		// If it's the current thread, just schedule it for deletion and yield
		if( NKGetThisProcessorInfo()->thread == this )
		{
			// Well, only if it's on this processor
			// Schedule this for deletion (no processor will actually delete this until the lock above is released)
			prev = nil;		// Should be nil already
			next = deleteQueue;
			deleteQueue = this;
			if( next )
				next->prev = deleteQueue;
		
			Yield();
		}
	}
	else
		n = NKGetThisProcessorInfo()->number;
	
	// If the current thread deleted itself, we should not be here.  If we deleted the current thread on another processor, we must wait for
	// the thread to be preempted.  If we otherwise deleted a non-current thread, we can simply proceed to finalize the deletion
	while( NKGetProcessorInfo(n)->thread == this )
		;
	
	// Do actual deletion work
	if(stack)
		delete [] (Int8*)stack;
	
	process->threadDeleted(this);
}

// *** Queue handling ***
// All these functions assume you are in a critical section and have the scheduling lock
// Also, you should really only use makeReady and makeNotReady.  They put the thread in the appropiate ready queues, but don't
// change the state.  Interrupting normal threads by RT threads being made available is your duty.
// Finally, chooseThread will return a thread which has been selected to run.  It also remove it from the appropiate ready queue
void
Thread::addToReady()
{
	FatalAssert( next == nil && prev == nil );
	next = readyQueue;
	readyQueue = this;
	if( next )
		next->prev = this;
	totalPriority += priority;
}

void
Thread::removeFromReady()
{
	totalPriority -= priority;
	if( next )
		next->prev = prev;
	if( prev )
		prev->next = next;
	if( readyQueue == this )
		readyQueue = next;
	next = prev = nil;
}

Thread*
Thread::chooseReady()
{
	// Check for no available threads
	if( !readyQueue )
		return nil;
	
	// Get a random number between 1 and totalPriority
	UInt32			random;
	random = ThreadRand();
	random = (random % totalPriority)+1;
	
	// Go through the ready queue until our random number is less than or equal to the cumulative priority
	UInt32			cum = 0;
	Thread			*result = readyQueue;
	while( result )
	{
		cum += result->priority;
		if( random <= cum )
			break;
		result = result->next;
	}
	
	if( !result )
	{
		nkVideo << "\nReady->";
		result = readyQueue;
		while( result )
		{
			nkVideo << result->name << " (" << result->priority << ")->";
			result = result->next;
		}
		nkVideo << "Done.  Total = " << totalPriority << ", Random = " << random << "\n";
		Panic("Thread::chooseReady:  totalPriority seems to be larger than the sum of priorities of threads in the ready queue!!!");
	}
	
	result->removeFromReady();
	return result;
}

void
Thread::addToRT()
{
	FatalAssert( (next == nil) && (prev == nil) );
	
	// Find the spot to put it into (sorted in descending priority, with this thread being added as far back as possible, e.g. if this had
	// priority 130, and the queue looked like 165->160->130->130->120->100, the thread would be inserted just before the 120
	// (not after the 160), so that equal priority RT threads do round-robin when they relinquish the processor.
	Thread				*curT = rtQueue, *prevT = nil;
	while( curT )
	{
		if( curT->priority < priority )
			break;
		prevT = curT;
		curT = curT->next;
	}
	
	// Insert it after prevT
	if( !prevT )
	{
		next = rtQueue;
		rtQueue = this;
	}
	else
	{
		prev = prevT;
		next = prevT->next;
	}
	
	if( next )
		next->prev = this;
	if( prev )
		prev->next = this;
}

void
Thread::removeFromRT()
{
	if( next )
		next->prev = prev;
	if( prev )
		prev->next = next;
	if( rtQueue == this )
		rtQueue = next;
	next = prev = nil;
}

Thread*
Thread::chooseRT()
{
	// Just return the first thread in the queue, which should be the highest priority thread
	if( !rtQueue )
		return nil;
	else
	{
		Thread				*result = rtQueue;
		result->removeFromRT();	// Is it worth expanding this here?  I don't think so
		return result;
	}
}

void
Thread::putInReady()
{
	if( priority >= 100 )
		addToRT();
	else if( priority != 0 )		// Idle thread never goes into ready queue
		addToReady();
}

void
Thread::makeReady()
{
#if DEBUG_READY
	nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") moving to ready state\n";
#endif

	putInReady();
	state = threadReady;
	
#if DEBUG_READY
	nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") moved to ready state";
#endif
	
	// See if this should preempt any other thread
	// Look for the lowest priority thread in all processors
	UInt32			n, minProc = 0, minPri = kPriorityMax+1;
	ProcessorInfo		*minInfo, *myInfo;
	myInfo = NKGetThisProcessorInfo();
	for( n = 0; n < NKGetNumProcessors(); n++ )
	{
		ProcessorInfo	*info = NKGetProcessorInfo(n);
		if( info->thread && info->thread != info->idleThread )
		{
			if( info->thread->priority < minPri )
			{
				minPri = info->thread->priority;
				minProc = n;
				minInfo = info;
			}
		}
		else
		{
			minPri = 0;
			minProc = n;
			minInfo = info;
			break;
		}
	}
	
	// See if this thread should preempt that processor
	if( minPri == 0 || (priority >= 100 && minPri < priority) )
	{
#if DEBUG_READY
		nkVideo << ", Interrupting CPU " << minProc;
#endif
		// Yep!
		if( minInfo->number != myInfo->number )
			// Send an IPI to that processor to preempt (number is in minProc == minInfo->number)
			Panic("Thread::makeReady(): Don't know how to preempt other processors (no IPIs)!");
		else
			// Just preempt ourselves (remember, it doesn't preempt until the critical section goes out of scope)
			Yield();
	}
#if DEBUG_READY
	nkVideo << "\n";
#endif
}

void
Thread::makeNotReady()
{
	if( priority >= 100 )
		removeFromRT();
	else
		removeFromReady();
}

Thread*
Thread::chooseThread()
{
	Thread*			result;
	result = chooseRT();
	if( !result )
		result = chooseReady();
	return result;
}

// Takes care of detaching the IO command from the thread, if necessary.  Assumes you're in a critical section with the scheduling lock locked.
void
Thread::cleanupIO()
{
	if( ioCmd )
	{
		// Already have the ioLock
		if( ioCmd->waitingThread != this )
			Panic("Discrepancy between blocked thread and it's I/O command!");
		ioCmd->waitingThread = nil;
		ioCmd = nil;
		timeout = false;
	}
}

void
Thread::doneIO()
{
	CriticalSection		critical(scheduleLock);
	
	// Check timeout hasn't expired
	if( state == threadBlocked )
	{
#if DEBUG_IO
		nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") completed I/O command " << (UInt32)ioCmd << "\n";
#endif
		
		cleanupIO();
		
		// Turn timer off (harmless if no timeout)
		timeoutTimer.remove();
		
		// Make thread elegible for execution again
		makeReady();
	}
}

// Takes care of detaching the semaphore from the thread, if necessary.  Assumes you're in a critical section with the scheduling lock locked.
void
Thread::cleanupSem()
{
	if( waitSem )
	{
		if( prev )
			prev->next = next;
		else
			waitSem->waitQueue = next;
		if( next )
			next->prev = prev;
		else
			waitSem->waitTail = prev;
		prev = next = nil;
		waitSem->timedOut();
		gotSemaphore = false;
		timeout = false;
	}
}

void Thread::resume(void)
{
	CriticalSection		critical(scheduleLock);
	
	// Must be suspended to resume
	if( state == threadSuspended )
	{
#if DEBUG_RESUMESUSPEND
		nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") resumed\n";
#endif
		makeReady();
	}
}

void Thread::suspend(void)
{
	CriticalSection		critical(scheduleLock);
	
	// Must be ready or current to suspend
	if( state == threadReady )
		makeNotReady();
	else if( state != threadRunning )
		return;
	
#if DEBUG_RESUMESUSPEND
	nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") suspended\n";
#endif
	
	if(state == threadRunning)
		Yield();				// We're in a critical section, yields on return, not before
	
	state = threadSuspended;
}

void Thread::blockForIO(IOCommand* cmd, Float64 nsTimeout)
{
	CriticalSection		critical(scheduleLock);
	SpinLocker		critical2(cmd->ioLock);
	
	if( state != threadReady && state != threadRunning )
		return;			// Only when current or ready
	
	if(!cmd->hasDoneIO)
	{
		FatalAssert(cmd->waitingThread == nil);
		ioCmd = cmd;
		ioCmd->waitingThread = this;
		
#if DEBUG_IO
		nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") blocked for I/O command " << (UInt32)cmd << "\n";
#endif
		
		if(state == threadRunning)
			Yield();		// Yields until out of critical
		if( state == threadReady )
			makeNotReady();
		state = threadBlocked;
		
		// Turn on timeout timer
		if( nsTimeout )
		{
			timeoutTimer.add(nsTimeout);
			timeout = true;
		}
		else
		{
			timeoutTimer.remove();
			timeout = false;
		}
	}
}

void Thread::sleepNS(Float64 ns,Boolean periodic)
{
	CriticalSection		critical(scheduleLock);
	
	// Only ready or current threads can sleep
	if( state == threadReady )
		makeNotReady();
	else if( state != threadRunning )
		return;
	
#if DEBUG_SLEEP
	nkVideo << greenMsg << name << whiteMsg << " (" << (UInt32)this << ") requests sleep of " << (UInt32)ns << " ns\n";
#endif
	// Set up timeout timer
	if( periodic )
		sleepTimer.addPeriodic( ns );
	else
		sleepTimer.add( ns );
	timeoutTimer.remove();
	timeout = false;
	
	// Yield if necessary
	if( state == threadRunning )
		Yield();
	
	state = threadSleeping;
}

void
Thread::wake(void)
{
	// Only if sleeping, though
	CriticalSection		critical(scheduleLock);
	
	// Must be suspended to resume
	if( state == threadSleeping )
	{
		timeoutTimer.remove();		// Turn this off
		makeReady();
	}
}

static void ThreadStarter(Thread* thread)
{
	thread->threadLoop();
	
#if DEBUG_DELETE
{
	// Acquire lock so screen doesn't mess up
	CriticalSection				critical(scheduleLock);
	nkVideo << greenMsg << thread->name << whiteMsg << " (" << (UInt32)thread << ") finished naturally\n";
}
#endif
	if(thread->deleteWhenDone)	// After this, our thread won't be scheduled anymore
		delete thread;
	else
		thread->state = threadDone;
	
	// Yield if we haven't been preempted yet.
	Yield();
}

// *** From here on starts the scheduler proper, the thing that gets called regularly when thread switches are needed
ThreadTimer::ThreadTimer(Thread *inThread)
	: myThread(inThread), NKTimer()
{
}

void
ThreadTimer::signal()
{
	CriticalSection				critical(scheduleLock);
	Boolean					ready = false;
	
	// Switch depending on the state
	switch( myThread->state )
	{
		case threadSleeping:
			// Time to wake up
#if DEBUG_SLEEP
			nkVideo << greenMsg << myThread->name << whiteMsg << " (" << (UInt32)myThread << ") finished sleeping\n";
#endif
			break;
		case threadBlocked:
			if( myThread->timeout )
			{
				// OOPS!  I/O command timed out
#if DEBUG_IO
				nkVideo << greenMsg << myThread->name << whiteMsg << " (" << (UInt32)myThread << ") timed out on I/O\n";
#endif
				myThread->cleanupIO();
				break;
			}
			// *ELSE FALLTHROUGH*
		case threadWaiting:
			if( myThread->timeout )
			{
				// OOPS!  Wait on semaphore timed out
#if DEBUG_SEM
				nkVideo << greenMsg << myThread->name << whiteMsg << " (" << (UInt32)myThread << ") timed out on semaphore\n";
#endif
				myThread->cleanupSem();
				break;
			}
			// *ELSE FALLTHROUGH*
		default:
			// This timer can also be signalled when the state is threadRunning, but no special handling is done here.
			// A check for quantum expiration is done in NKCheckSchedule()
#if DEBUG_QUANTUM
			nkVideo << greenMsg << myThread->name << whiteMsg << " (" << (UInt32)myThread << ") expired quantum\n";
#endif
			return;
	}
	
	// Make ready if necessary (should have gone out by now if not)
	myThread->makeReady();
}

void NKCheckSchedule( PPCRegisters* savedRegs )
{
	if( !threadsReady )
		return;
	
	CriticalSection				critical(scheduleLock);
	ProcessorInfo				*info = NKGetThisProcessorInfo();
	Thread					*current = info->thread;
	Boolean					needSwitch = true;
	
	clearReservation();
	
	// First time around, there's *no* current thread
	if( current )
	{
		if( current->state == threadRunning && current != info->idleThread
		&& ((current->quantumTime > GetTime_ns()) || (current->priority >= 100)) )	// RT threads never "run out" of quantum
			needSwitch = false;
	}
	
	if( needSwitch )
	{
		if( current )
		{
			if( current != info->idleThread && current->state == threadRunning )
			{
				current->putInReady();
				current->timeoutTimer.remove();
			}
		}
		
		Thread				*newThread;
		newThread = Thread::chooseThread();
		if( !newThread )
			newThread = info->idleThread;
		
		if( current != newThread )
		{
			// Save & restore
#if DEBUG_PREEMPT
			nkVideo << redMsg;
			if( current )
				nkVideo << "Preempted " << greenMsg << current->name << redMsg << " (" << (UInt32)current << "): PC = " << current->savedRegs.srr0;
#endif
			if( current )
				current->savedRegs = *savedRegs;
#if DEBUG_PREEMPT
			else
				nkVideo << "Preempted into " << greenMsg << newThread->name << redMsg << " (" << (UInt32)newThread << "), pc = " << newThread->savedRegs.srr0 << "\n";
#endif
			*savedRegs = newThread->savedRegs;
#if DEBUG_PREEMPT
			if( current )
				nkVideo << " for " << greenMsg << newThread->name << redMsg << " (" << (UInt32)newThread << "), pc = " << newThread->savedRegs.srr0 << "\n";
			nkVideo << whiteMsg;
#endif
#if DEBUG_QUANTUM
		}
		else
		{
			nkVideo << redMsg << "Quantum, " << greenMsg << current->name << redMsg << " (" << (UInt32)current << ") remains\n"
				<< whiteMsg;
#endif
		}
		
		// Move to running state on this processor
		newThread->state = threadRunning;
		info->thread = newThread;
		
		// Quantum only applies to normal threads, not idle or real-time threads
		if( (newThread != info->idleThread) && (newThread->priority < 100) )
		{
			if( newThread->timeout )
			{
				nkVideo << "NKCheckSchedule(): timeout was true on quantum?";
				newThread->timeout = false;
			}
			newThread->quantumTime = GetTime_ns() + THREAD_QUANTUM;
			newThread->timeoutTimer.add( THREAD_QUANTUM );
		}
		
		*machine.videoParams.logicalAddr = (newThread == info->idleThread ? red8bit : white8bit);
	}
	
	// Delete everything in the delete queue (need to move this somewhere where it doesn't use up thread quantum)
	// Why not put it into the idle thread?  -TG
	Thread					*toDelete = deleteQueue, *nextDelete;
	while( toDelete )
	{
		nextDelete = toDelete->next;
		toDelete->next = toDelete->prev = nil;
#if DEBUG_DELETE
		nkVideo << greenMsg << toDelete->name << whiteMsg << " (" << (UInt32)toDelete << ")";
#endif
		delete toDelete;
#if DEBUG_DELETE
		nkVideo << " incinerated\n";
#endif
		toDelete = nextDelete;
	}
	deleteQueue = nil;
}

static void Yield(void)
{
	// Assume we're in a critical section with the appropiate lock
	ProcessorInfo			*info = NKGetThisProcessorInfo();
	if( info->thread )
		info->thread->quantumTime = 0;
	DoYield();
}

static __asm__ void DoYield(void)
{
	// Force decrementor high bit from 0 to 1
	lis		r3,0x7FFF;
	mtdec(r3);
	lis		r3,0x8000;
	mtdec(r3);
	blr;
}

static __asm__ void clearReservation()
{
	lwz		r0,reservationScratch(rtoc);
	stwcx.	r0,r0,r0;	// Clear any pending reservations
	blr;
}

Thread* CurrThread::thread()
{
	NKCriticalSection	critical;
	return NKGetThisProcessorInfo()->thread;
}

void
CurrThread::resume(void)
{
	thread()->resume();
}

void
CurrThread::suspend(void)
{
	thread()->suspend();
}

void
CurrThread::blockForIO(IOCommand* cmd,Float64 nsTimeout)
{
	thread()->blockForIO(cmd,nsTimeout);
}

void
CurrThread::sleepNS(Float64 ns)
{
	thread()->sleepNS(ns);
}

void
CurrThread::wake(void)
{
	thread()->wake();
}

void
CurrThread::yield()
{
	CriticalSection				critical(scheduleLock);
	Yield();
}

UInt32
ThreadRand()
{
	threadRandSeed = threadRandSeed*1103515245 + 12345;
	return threadRandSeed;
}

// A semaphore is an object with a small counter.  A thread which wants the semaphore decrements that counter.  If that counter
// remains positive, the thread continues.  Otherwise, it waits until the counter becomes positive.  Other threads which already
// have the semaphore can release it by incrementing the counter.  If this results in the counter becoming positive and there are
// threads waiting for the semaphore, one of those threads is picked at random and resumed, so that it now posseses the semaphore.

// Depending on the initial value of the counter, a Semaphore can do different things.  If it's 1, it could control access to a resource,
// letting only one thread access it at a time (e.g. a linked list).  It could also be used to control a messaging queue.  When a message
// is placed in the queue, the counter is incremented.  When a message is requested, the counter is decremented.  In this way, the
// counter implicitly encodes the number of messages in the queue, and will only let someone retrieve a message if the counter is
// greater than 0.  It will otherwise block and wait for a message to be placed in the queue.  An inverse semaphore could control
// that not too many messages are placed in a queue without being received.  All in all, a semaphore is a very powerful tool.
Semaphore::Semaphore( Int32 n )
{
	_n = n;
	waitQueue = waitTail = nil;
}

Semaphore::~Semaphore()
{
	CriticalSection		critical(scheduleLock);
	Thread			*cur = waitQueue, *next;
	while( cur )
	{
		cur->prev = nil;
		next = cur->next;
		cur->next = nil;
		cur->timeoutTimer.remove();
		cur->makeReady();
		cur = next;
	}
}

Boolean
Semaphore::down( Float64 nsTimeout )
{
	Boolean				blocked = false;
	Thread				*thread;
	
	{
		CriticalSection		critical(scheduleLock);
		
		thread = NKGetThisProcessorInfo()->thread;
		wait( thread, nsTimeout );
	}
	
	// If we blocked, should preempt here
	return thread->gotSemaphore;
}

void
Semaphore::up()
{
	CriticalSection		critical(scheduleLock);
	
	signal();
}

void
Semaphore::wait( Thread* thread, Float64 nsTimeout )
{
#if DEBUG_SEM
	nkVideo << "Semaphore wait " << (UInt32)this << " (" << _n << ")\n";
#endif
	if( --_n < 0 )
	{
#if DEBUG_SEM
		nkVideo << greenMsg << thread->name << whiteMsg << " (" << (UInt32)thread << ") waiting on semaphore " << (UInt32)this << "\n";
#endif
		// Put in wait queue and block (should preempt soon)
		if( waitTail )
		{
			waitTail->next = thread;
			thread->prev = waitTail;
			thread->next = nil;
			waitTail = thread;
		}
		else
		{
			thread->prev = thread->next = nil;
			waitQueue = waitTail = thread;
		}
		
		Yield();
		
		thread->state = threadWaiting;
		thread->waitSem = this;
		
		if( nsTimeout )
		{
			thread->timeoutTimer.add( nsTimeout );
			thread->timeout = true;
		}
		else
		{
			thread->timeoutTimer.remove();
			thread->timeout = false;
		}
	}
	else
	{
		thread->gotSemaphore = true;
#if DEBUG_SEM
		nkVideo << greenMsg << thread->name << whiteMsg << " (" << (UInt32)thread << ") acquired semaphore " << (UInt32)this << "\n";
#endif
	}
}

void
Semaphore::signal()
{
#if DEBUG_SEM
	nkVideo << "Semaphore signal " << (UInt32)this << " (" << _n << ")\n";
#endif
	++_n;
	if( waitQueue )
	{
		// Remove from wait queue and switch back on
		Thread		*thread = waitQueue;
		waitQueue = waitQueue->next;
		thread->next = nil;
		if( waitQueue )
			waitQueue->prev = nil;
		else
			waitTail = nil;
		thread->timeoutTimer.remove();
		thread->gotSemaphore = true;
#if DEBUG_SEM
		nkVideo << greenMsg << thread->name << whiteMsg << " (" << (UInt32)thread << ") acquired semaphore " << (UInt32)this << "\n";
#endif
		FatalAssert( thread->state == threadWaiting );
		thread->makeReady();
	}
}

void
Semaphore::timedOut()
{
	// Just signal (remember, this decremented the counter before locking)
	signal();
}

// An event semaphore is one in which every thread that does a down is made to wait, but when another thread does an up, all waiting threads
// are made ready.  Thus, down waits for an event and up signifies that event
void
EventSemaphore::signal()
{
#if DEBUG_SEM
	nkVideo << "Event semaphore signal " << (UInt32)this << "\n";
#endif
	while( waitQueue )
	{
		// Remove from wait queue and switch back on
		Thread		*thread = waitQueue;
		waitQueue = waitQueue->next;
		thread->next = nil;
		if( waitQueue )
			waitQueue->prev = nil;
		thread->timeout = false;
		thread->timeoutTimer.remove();
		thread->gotSemaphore = true;
		thread->makeReady();
	}
	waitQueue = waitTail = 0;
	_n = 0;
}

void
EventSemaphore::timedOut()
{
	// Don't signal!  A timeout on an event semaphore just means the signal didn't come.
}