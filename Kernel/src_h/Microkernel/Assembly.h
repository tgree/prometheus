/*
	Assembly.h
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
	Other sources			Project				Author		Notes
	===========			======				=====		====
	none
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#ifndef __ASSEMBLY__
#define __ASSEMBLY__

#include "Compiler.h"

// Useful macros
#define	UPPER_HW(n)	( ( -(( ( (n) >> 16) & 0x0000FFFF) >= 0x00008000)*(1 + ((~(n)) >> 16) & 0x0000FFFF) ) | ((( ( (n) >> 16) & 0x0000FFFF) < 0x00008000)*( ((n) >> 16) & 0x0000FFFF) ) )
#define	LOWER_HW(n)	( -(( (n) & 0x0000FFFF) >= 0x00008000)*(1 + ~((n) & 0x0000FFFF) ) | ( ( (n) & 0x0000FFFF) < 0x00008000)*( (n) & 0x0000FFFF) )
#define LOAD_IMM(r,n)	lis	r,UPPER_HW(n);	ori	r,r,LOWER_HW(n);

// Extended mnemonics
#define	srwi(rt,ra,n)			rlwinm	rt,ra,32-n,n,31
#define	srwi_(rt,ra,n)			rlwinm.	rt,ra,32-n,n,31
#define	slwi(rt,ra,n)			rlwinm	rt,ra,n,0,31-n
#define	slwi_(rt,ra,n)			rlwinm.	rt,ra,n,0,31-n
#define	rrwinm(rt,ra,n,m1,m2)	rlwinm	rt,ra,32-n,m1,m2
#define	rrwinm_(rt,ra,n,m1,m2)	rlwinm.	rt,ra,32-n,m1,m2

// The size of the PowerOpen ABI "Red Zone"
#define	RED_ZONE_SIZE		224

// Macros for manipulating the condition register fields
#define	cr0_LT	0
#define	cr0_GT	1
#define	cr0_EQ	2
#define	cr0_SO	3
#define	cr1_LT	4
#define	cr1_GT	5
#define	cr1_EQ	6
#define	cr1_SO	7
#define	cr2_LT	8
#define	cr2_GT	9
#define	cr2_EQ	10
#define	cr2_SO	11
#define	cr3_LT	12
#define	cr3_GT	13
#define	cr3_EQ	14
#define	cr3_SO	15
#define	cr4_LT	16
#define	cr4_GT	17
#define	cr4_EQ	18
#define	cr4_SO	19
#define	cr5_LT	20
#define	cr5_GT	21
#define	cr5_EQ	22
#define	cr5_SO	23
#define	cr6_LT	24
#define	cr6_GT	25
#define	cr6_EQ	26
#define	cr6_SO	27
#define	cr7_LT	28
#define	cr7_GT	29
#define	cr7_EQ	30
#define	cr7_SO	31

// SPR registers
#define	RTCU_R		4
#define	RTCL_R		5
#define	DSISR		18
#define	DAR			19
#define	RTCU_W		20
#define	RTCL_W		21
#define	DEC			22
#define	SDR1		25
#define	SRR0		26
#define	SRR1		27
#define	TBL_R		268
#define	TBU_R		269
#define	SPRG0		272
#define	SPRG1		273
#define	SPRG2		274
#define	SPRG3		275
#define	EAR			282
#define	TBL_W		284
#define	TBU_W		285
#define	PVR			287
#define	IBAT0U		528
#define	IBAT0L		529
#define	IBAT1U		530
#define	IBAT1L		531
#define	IBAT2U		532
#define	IBAT2L		533
#define	IBAT3U		534
#define	IBAT3L		535
#define	DBAT0U		536
#define	DBAT0L		537
#define	DBAT1U		538
#define	DBAT1L		539
#define	DBAT2U		540
#define	DBAT2L		541
#define	DBAT3U		542
#define	DBAT3L		543
#define	HID0			1008
#define	HID1			1009
#define	HID2			1010
#define	DABR		1013	// DABR is also HID5

// 603 specific registers
#define	DMISS		976
#define	DCMP		977
#define	HASH1		978
#define	HASH2		979
#define	IMISS		980
#define	ICMP			981
#define	RPA			982

// 750, 7400 registers
#define	L2CR			1017
#define	IABR			1010
// 7400 registers
#define	MSSCR0		1014
#define	MSSCR1		1015

// Some assembly opcodes we use when generating the interrupt vector table
#define SET_FLD(bit1,bit2,val)		( ((UInt32)((UInt32)val) << (31UL - (bit2))) & ( (UInt32)((1UL << (32 - ((UInt32)bit1))) - 1UL) & ~( (1UL << (31UL - ((UInt32)bit2))) - 1UL)) )
#define ADDI(regt,regs,val)		(SET_FLD(0,5,14) | SET_FLD(6,10,regt) | SET_FLD(11,15,regs) | SET_FLD(16,31,val))
#define ADDIS(regd,rega,val)		(SET_FLD(0,5,15) | SET_FLD(6,10,regd) | SET_FLD(11,15,rega) | SET_FLD(16,31,val))
#define BLRL					(SET_FLD(0,5,19) | SET_FLD(6,10,20) | SET_FLD(21,30,16) | SET_FLD(31,31,1))
#define LIS(regd,val)			ADDIS(regd,0,val)
#define LWZ(regd,offset,rega)		(SET_FLD(0,5,32) | SET_FLD(6,10,regd) | SET_FLD(11,15,rega) | SET_FLD(16,31,offset))
#define MFSPR(reg,sprg)			(SET_FLD(0,5,31) | SET_FLD(6,10,reg) | SET_FLD(11,15,(sprg & 0x1F)) | SET_FLD(16,20,((sprg >> 5) & 0x1F)) | SET_FLD(21,30,339))
#define MTSPR(sprg,reg)			(SET_FLD(0,5,31) | SET_FLD(6,10,reg) | SET_FLD(11,15,(sprg & 0x1F)) | SET_FLD(16,20,((sprg >> 5) & 0x1F)) | SET_FLD(21,30,467))
#define MFLR(reg)				MFSPR(reg,8)
#define MFMSR(reg)				(SET_FLD(0,5,31) | SET_FLD(6,10,reg) | SET_FLD(21,30,83))
#define MTLR(reg)				MTSPR(8,reg)
#define MTMSR(reg)				(SET_FLD(0,5,31) | SET_FLD(6,10,reg) | SET_FLD(21,30,146))
#define ORI(regt,regs,val)		(SET_FLD(0,5,24) | SET_FLD(6,10,regt) | SET_FLD(11,15,regs) | SET_FLD(16,31,val))
#define OR(rega,regs,regb)		(SET_FLD(0,5,31) | SET_FLD(6,10,regs) | SET_FLD(11,15,rega) | SET_FLD(16,20,regb) | SET_FLD(21,30,444))
#define RFI					(SET_FLD(0,5,19) | SET_FLD(21,30,50))
#define STWU(regs,offset,regt)	(SET_FLD(0,5,37) | SET_FLD(6,10,regs) | SET_FLD(11,15,regt) | SET_FLD(16,31,offset))
#define SYNC					(SET_FLD(0,5,31) | SET_FLD(21,30,598))
#define BA(addr)				(SET_FLD(0,5,18) | SET_FLD(30,30,1) | addr)
#define BAL(addr)				(BA(addr) | 1)
#define NOP					OR(0,0,0)
#define RLWINM(ra,rs,sh,mb,me)	(SET_FLD(0,5,21) | SET_FLD(6,10,rs) | SET_FLD(11,15,ra) | SET_FLD(16,20,sh) | SET_FLD(21,25,mb) | SET_FLD(26,30,me))
#define MULLI(rd,ra,simm)		(SET_FLD(0,5,7) | SET_FLD(6,10,rd) | SET_FLD(11,15,ra) | SET_FLD(16,31,simm))
#define AND(ra,rs,rb)			(SET_FLD(0,5,31) | SET_FLD(6,10,rs) | SET_FLD(11,15,ra) | SET_FLD(16,20,rb) | SET_FLD(21,30,28))
#define ANDC(ra,rs,rb)			(SET_FLD(0,5,31) | SET_FLD(6,10,rs) | SET_FLD(11,15,ra) | SET_FLD(16,20,rb) | SET_FLD(21,30,60))

// Some useful #defines
#define	mfpvr(rt)			mfspr	rt,PVR
#define	mtdbatu(bat,ra)	mtspr	(DBAT0U+2*bat),ra
#define	mfdbatu(rt,bat)		mfspr	rt,(DBAT0U+2*bat)
#define	mtdbatl(bat,ra)		mtspr	(DBAT0L+2*bat),ra
#define	mfdbatl(rt,bat)		mfspr	rt,(DBAT0L+2*bat)
#define	mtibatu(bat,ra)		mtspr	(IBAT0U+2*bat),ra
#define	mfibatu(rt,bat)		mfspr	rt,(IBAT0U+2*bat)
#define	mtibatl(bat,ra)		mtspr	(IBAT0L+2*bat),ra
#define	mfibatl(rt,bat)		mfspr	rt,(IBAT0L+2*bat)
#define	mfsdr1(rt)		mfspr	rt,SDR1
#define	mtsdr1(ra)		mtspr	SDR1,ra
#define	mtsprg(sprg,ra)	mtspr	(SPRG0+sprg),ra
#define	mfsprg(rt,sprg)	mfspr	rt,(SPRG0+sprg)
#define	mtcr(ra)			mtcrf	0xFF,ra
#define	mfdsisr(rt)		mfspr	rt,DSISR
#define	mfdar(rt)			mfspr	rt,DAR
#define	mfsrr0(rt)		mfspr	rt,SRR0
#define	mfsrr1(rt)		mfspr	rt,SRR1
#define	mtsrr0(ra)		mtspr	SRR0,ra
#define	mtsrr1(ra)		mtspr	SRR1,ra
#define	mtdec(ra)			mtspr	DEC,ra
#define	mfdec(rt)			mfspr	rt,DEC
#define	mtdabr(ra)		mtspr	DABR,ra
#define	mfl2cr(rt)		mfspr	rt,L2CR
#define	mtl2cr(ra)		mtspr	L2CR,ra
#define	trapNow(imm)		twi		0x1F,r0,imm
#define	mfhid0(rt)		mfspr	rt,HID0
#define	mthid0(ra)		mtspr	HID0,ra
#define	swapRegs(ra,rb)	xor ra,ra,rb; xor rb,ra,rb; xor ra,ra,rb;

__inline__ __asm__	UInt32		EnableDR(void)	{mfmsr r3; ori r4,r3,0x0010; sync; mtmsr r4; sync; blr;}		// Enables Data Relocation
__inline__ __asm__	UInt32		DisableDR(void)	{mfmsr r3; rlwinm r4,r3,0,28,26; sync; mtmsr r4; sync; blr;}	// Disables Data Relocation

__inline__ __asm__ UInt32		EnableIR(void)	{mfmsr r3; ori r4,r3,0x0020; sync; mtmsr r4; sync; blr;}		// Enables Instruction Relocation
__inline__ __asm__ UInt32		DisableIR(void)	{mfmsr r3; rlwinm r4,r3,0,27,25; sync; mtmsr r4; sync; blr;}	// Disables Data Relocation

__inline__ __asm__ void			SetMSR(register UInt32 msr)	{sync; mtmsr r3; isync; blr;}
__inline__ __asm__ UInt32		GetMSR(void)					{mfmsr r3; blr;}
__inline__ __asm__	UInt32		DisableInterrupts(void)			{mfmsr r3; rlwinm r4,r3,0,17,15; sync; mtmsr r4; isync; blr;}
__inline__ __asm__ void			EnableInterrupts(void)			{mfmsr r3; ori r3,r3,0x8000; sync; mtmsr r3; isync; blr;}

__inline__ __asm__ UInt32		_getSR(register UInt32 n) {slwi(r3,r3,28); mfsrin r3,r3; blr;}
__inline__ __asm__ void			_setSR(register UInt32 n,register UInt32 val) {slwi(r3,r3,28); isync; mtsrin r4,r3; isync; blr;}
__inline__ __asm__ void			_setSR0(register UInt32 val) {sync; mtsr 0,r3; sync; blr;}
__inline__ __asm__ UInt32		_getSDR1(void) {mfsdr1(r3); blr;}
__inline__ __asm__ void			_setSDR1(register UInt32 n) {sync; mtsdr1(r3); sync; blr;}
__inline__ __asm__ UInt32		_getHID0(void) {mfspr r3,HID0; blr;}
__inline__ __asm__ void			_setHID0(register UInt32 val) {sync; mtspr HID0,r3; sync; blr;}
__inline__ __asm__	UInt32		_getHID1(void) {mfspr r3,HID1; blr;}

__inline__ __asm__ void			_dcbi(register void* addr)
{
	sync;
	dcbi	r0,r3;
	sync;
	blr;
}

__inline__ __asm__ void			_dcbf(register void* addr)
{
	sync;
	dcbf r0,r3;
	sync;
	blr;
}

__inline__ __asm__ void			_icbi(register void* addr)
{
	sync;
	icbi	r0,r3;
	sync;
	isync;
	blr;
}


__inline__ __asm__ void			_setIBAT0L(register UInt32 n) {mtibatl(0,r3); blr;}
__inline__ __asm__ void			_setIBAT0U(register UInt32 n) {sync; mtibatu(0,r3); sync; blr;}
__inline__ __asm__ UInt32		_getIBAT0L(void) {mfibatl(r3,0); blr;}
__inline__ __asm__ UInt32		_getIBAT0U(void) {mfibatu(r3,0); blr;}
__inline__ __asm__ void			_setIBAT1L(register UInt32 n) {mtibatl(1,r3); blr;}
__inline__ __asm__ void			_setIBAT1U(register UInt32 n) {sync; mtibatu(1,r3); sync; blr;}
__inline__ __asm__ UInt32		_getIBAT1L(void) {mfibatl(r3,1); blr;}
__inline__ __asm__ UInt32		_getIBAT1U(void) {mfibatu(r3,1); blr;}
__inline__ __asm__ void			_setIBAT2L(register UInt32 n) {mtibatl(2,r3); blr;}
__inline__ __asm__ void			_setIBAT2U(register UInt32 n) {sync; mtibatu(2,r3); sync; blr;}
__inline__ __asm__ UInt32		_getIBAT2L(void) {mfibatl(r3,2); blr;}
__inline__ __asm__ UInt32		_getIBAT2U(void) {mfibatu(r3,2); blr;}
__inline__ __asm__ void			_setIBAT3L(register UInt32 n) {mtibatl(3,r3); blr;}
__inline__ __asm__ void			_setIBAT3U(register UInt32 n) {sync; mtibatu(3,r3); sync; blr;}
__inline__ __asm__ UInt32		_getIBAT3L(void) {mfibatl(r3,3); blr;}
__inline__ __asm__ UInt32		_getIBAT3U(void) {mfibatu(r3,3); blr;}
__inline__ __asm__ void			_setDBAT0L(register UInt32 n) {mtdbatl(0,r3); blr;}
__inline__ __asm__ void			_setDBAT0U(register UInt32 n) {sync; mtdbatu(0,r3); sync; blr;}
__inline__ __asm__ UInt32		_getDBAT0L(void) {mfdbatl(r3,0); blr;}
__inline__ __asm__ UInt32		_getDBAT0U(void) {mfdbatu(r3,0); blr;}
__inline__ __asm__ void			_setDBAT1L(register UInt32 n) {mtdbatl(1,r3); blr;}
__inline__ __asm__ void			_setDBAT1U(register UInt32 n) {sync; mtdbatu(1,r3); sync; blr;}
__inline__ __asm__ UInt32		_getDBAT1L(void) {mfdbatl(r3,1); blr;}
__inline__ __asm__ UInt32		_getDBAT1U(void) {mfdbatu(r3,1); blr;}
__inline__ __asm__ void			_setDBAT2L(register UInt32 n) {mtdbatl(2,r3); blr;}
__inline__ __asm__ void			_setDBAT2U(register UInt32 n) {sync; mtdbatu(2,r3); sync; blr;}
__inline__ __asm__ UInt32		_getDBAT2L(void) {mfdbatl(r3,2); blr;}
__inline__ __asm__ UInt32		_getDBAT2U(void) {mfdbatu(r3,2); blr;}
__inline__ __asm__ void			_setDBAT3L(register UInt32 n) {mtdbatl(3,r3); blr;}
__inline__ __asm__ void			_setDBAT3U(register UInt32 n) {sync; mtdbatu(3,r3); sync; blr;}
__inline__ __asm__ UInt32		_getDBAT3L(void) {mfdbatl(r3,3); blr;}
__inline__ __asm__ UInt32		_getDBAT3U(void) {mfdbatu(r3,3); blr;}
__inline__ __asm__ UInt32		_getPVR(void) {mfpvr(r3); blr;}
__inline__ __asm__ UInt32		_getDSISR(void) {mfdsisr(r3); blr;}
__inline__ __asm__ UInt32		_getDAR(void) {mfdar(r3); blr;}
__inline__ __asm__ UInt32		_getSRR0(void) {mfsrr0(r3); blr;}
__inline__ __asm__ UInt32		_getSRR1(void) {mfsrr1(r3); blr;}
__inline__ __asm__ void			_setSRR1(register UInt32) {mtsrr1(r3); blr;}
__inline__ __asm__	UInt32		_getSPRG0(void) {mfsprg(r3,0); blr;}
__inline__ __asm__	UInt32		_getSPRG1(void) {mfsprg(r3,1); blr;}
__inline__ __asm__	UInt32		_getSPRG2(void) {mfsprg(r3,2); blr;}
__inline__ __asm__	UInt32		_getSPRG3(void) {mfsprg(r3,3); blr;}
__inline__ __asm__ void			_setSPRG0(register UInt32) {mtsprg(0,r3); blr;}
__inline__ __asm__	void			_setSPRG1(register UInt32) {mtsprg(1,r3); blr;}
__inline__ __asm__	void			_setSPRG2(register UInt32) {mtsprg(2,r3); blr;}
__inline__ __asm__	void			_setSPRG3(register UInt32) {mtsprg(3,r3); blr;}
__inline__ __asm__ void			_setDEC(register UInt32) {mtdec(r3); blr;}
__inline__ __asm__ void			_setDABR(register UInt32) {mtdabr(r3); blr;}
__inline__ __asm__	UInt32		_getRTOC(void) {mr r3,RTOC; blr;}
__inline__ __asm__	UInt32		_getSP(void) {mr r3,sp; blr;}
__inline__ __asm__	UInt8		_lbz(register volatile UInt8*) {lbz r3,0(r3); blr;}
__inline__ __asm__	void			_sc(void) {sc; blr;}
#define	_l2le(addr)			__lhbrx((void*)addr,0)
#define	_st2le(val,addr)		__sthbrx(val,(void*)addr,0)
#define	_l4le(addr)			__lwbrx((void*)addr,0)
#define	_st4le(val,addr)		__stwbrx(val,(void*)addr,0)
__inline__ __asm__ UInt16		_l2be(register void*) {lhz r3,0(r3); blr;}
__inline__ __asm__ void			_st2be(register UInt16,register void*) {sth r3,0(r4); blr;}
__inline__ __asm__ UInt32		_l4be(register void*) {lwz r3,0(r3); blr;}
__inline__ __asm__ void			_st4be(register UInt32,register void*) {stw r3,0(r4); blr;}
__inline__ __asm__ UInt32		_getL2CR() {mfspr r3,L2CR; blr;}
__inline__ __asm__ void			_setL2CR(register val) {sync; mtspr L2CR,r3; sync; blr;}
				UInt32		_getLR();
__inline__ __asm__ void*		_findFirstStackFrame(void)	// Find the highest address contained in this stack!
{
	mr		r4,sp;
loop:
	lwz		r4,0(r4);
	cmpwi	r4,0;
	beq		@out;
	mr		r3,r4;
	b		loop;
@out:
	blr;
}

void*				_getPC(void);			// Returns the Program Counter (the LR for this function call, actually)

__inline__ __asm__ void			_tlbie(register UInt32 logAddr)
{
#if __MWERKS__
	machine 603;
#endif
	sync;
	tlbie r3;
	sync;
	tlbsync;
	sync;
	
	blr;
}

__inline__ __asm__ void			_tlbsync()
{
#if __MWERKS__
	machine 603;
#endif
	tlbsync;
	blr;
}

__inline__ __asm__ UInt64		_getClock(void)
{
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,1;
	beq-		@RTCTime;
@TBTime:
	mfspr	r3,TBU_R;
	mfspr	r4,TBL_R;
	mfspr	r5,TBU_R;
	cmpw	r3,r5;
	bne		@TBTime;
	blr;
@RTCTime:
	mfspr	r3,RTCU_R;
	mfspr	r4,RTCL_R;
	mfspr	r5,RTCU_R;
	cmpw	r3,r5;
	bne		@RTCTime;
	blr;
}

__inline__ __asm__ void			_zapClock(void)
{
	li		r4,0
	mfpvr(r3);
	srwi(r3,r3,16);
	cmpwi	r3,1;
	beq-		@RTCTime;
@TBTime:
	mtspr	TBL_W,r4;
	mtspr	TBU_W,r4;
	mtspr	TBL_W,r4;
	blr;
@RTCTime:
	mtspr	RTCL_W,r4;
	mtspr	RTCU_W,r4;
	mtspr	RTCL_W,r4;
	blr;
}

#ifdef __MWERKS__
#define	_eieio		__eieio
#define	_sync		__sync
#define	_isync	__isync
#else
__inline__ __asm__ void _eieio(void) {eieio; blr;}
__inline__ __asm__ void _sync(void) {sync; blr;}
__inline__ __asm__ void _isync(void) {isync; blr;}
#endif

#endif /* __ASSEMBLY__ */
