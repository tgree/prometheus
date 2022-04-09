#ifndef __L2_DIAGNOSTICS_PROCS__
#define __L2_DIAGNOSTICS_PROCS__

enum
{
	// For blockFlags
	cacheBlockTested	=	1,	// This block has been tested
	cacheBlockGood		=	2,	// This block passed testing
	cacheBlockHitMem	=	4	// This block hit memory in the cache load routine
};

// This callback is called for every cache block before testing.  Use it to report the progress
// of the cache test.  You can pass in a nil one if you don't want to report.
typedef void (*L2DiagnosticsCallbackProcPtr)(UInt32 currBlock,UInt32 numBlocks);

// This routine is used to perform the actual test.  You are called in supervisor mode with
// MSR[DR] = 0.  The L1 cache is in a state such that you can't damage anything in it.  Your
// routine should simply allocate its block in L1, perform it's store routine to L2 and
// return the expected block contents in r24 - r31.  Do NOT read from L2.  You have access
// to all GPR for your use.  You should save/restore SP and never actually access the stack.
typedef void (*L2DiagnosticsTestProcPtr)(register Ptr cacheBlockPhysical,register UInt32 blockNum);

// Some standard L2 tests that can be passed in to TestL2Cache
void L2BlockEnumerateTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Stamps each cache block with the block number
void L2BlockOnesTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);			// Fills the block with binary ones
void L2BlockZeroesTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);			// Fills the block with binary zeroes
void L2BlockAlternateTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Fills the block with binary 01's
void L2BlockWalkingOneTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Walks a 1 over all 32 bits of each word
void L2BlockWalkingZeroTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Walks a 0 over all 32 bits of each word, which is otherwise all 1's
void L2BlockTimeBaseTest(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Stamps each block with the timebase value
void L2BlockCount256Test(register Ptr cacheBlockPhysical,register UInt32 blockNum);		// Counts in each block using the low 8 bits of blockNum

// API for L2 Diagnostic Procs
void PrepareL2ForTesting(UInt32 testL2CR);
void TestL2Cache(Ptr expectedCacheContents,Ptr twoMBBlockPhysical,Ptr twoMBBlockLogical,UInt32 startBlock,UInt32 numBlocks,UInt8* blockFlags,L2DiagnosticsCallbackProcPtr callback,L2DiagnosticsTestProcPtr testProc);
void FinishL2Testing();

#endif /* __L2_DIAGNOSTICS_PROCS__ */