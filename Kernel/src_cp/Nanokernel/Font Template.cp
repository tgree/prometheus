/*
	Font Template.cp
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
*/
#include "NKFonts.h"
#include "Assembly.h"
#include "Memory Manager.h"

#define	FONT_NAME		blaBlaBla9
#define	FONT_INIT_FUNC	InitBlaBlaBla
#define	WIDTH			5
#define	HEIGHT			9
#define	HAS_LOWERCASE	0

#if HAS_LOWERCASE
#define LOWERCASE_ENTRY(a)
#else
#define LOWERCASE_ENTRY(a)	entry static a
#endif

#define	PIX1(x)	stb	r5,(x)(r3);
#define	PIX2(x)	stb	r5,(x)(r3);	stb	r5,((x)+1)(r3);
#define	PIX3(x)	stb	r5,(x)(r3);	stb	r5,((x)+1)(r3);	stb	r5,((x)+2)(r3);
#define	PIX4(x)	stb	r5,(x)(r3);	stb	r5,((x)+1)(r3);	stb	r5,((x)+2)(r3);	stb	r5,((x)+3)(r3);
#define	PIX5(x)	PIX4(x);	PIX1((x)+4);
#define	PIX6(x)	PIX4(x);	PIX2((x)+4);
#define	PIX7(x)	PIX4(x);	PIX3((x)+4);
#define	PIX8(x)	PIX4(x);	PIX4((x)+4);
#define	PIX9(x)	PIX8(x);	PIX1((x)+8);
#define	PIX10(x)	PIX8(x);	PIX2((x)+8);
#define	NL		add	r3,r3,r4;

void	FONT_INIT_FUNC(void);

static void _space_8(register char* addr,register long rowBytes,register unsigned long color);
static void _exclam_8(register char* addr,register long rowBytes,register unsigned long color);
static void _quote_8(register char* addr,register long rowBytes,register unsigned long color);
static void _pound_8(register char* addr,register long rowbytes,register unsigned long color);
static void _dollar_8(register char* addr,register long rowbytes,register unsigned long color);
static void _percent_8(register char* addr,register long rowbytes,register unsigned long color);
static void _ampersand_8(register char* addr,register long rowbytes,register unsigned long color);
static void _apost_8(register char* addr,register long rowBytes,register unsigned long color);
static void _open_parens_8(register char* addr,register long rowBytes,register unsigned long color);
static void _close_parens_8(register char* addr,register long rowBytes,register unsigned long color);
static void _star_8(register char* addr,register long rowBytes,register unsigned long color);
static void _plus_8(register char* addr,register long rowBytes,register unsigned long color);
static void _comma_8(register char* addr,register long rowBytes,register unsigned long color);
static void _minus_8(register char* addr,register long rowBytes,register unsigned long color);
static void _period_8(register char* addr,register long rowBytes,register unsigned long color);
static void _fs_8(register char* addr,register long rowbytes,register unsigned long color);
static void _0_8(register char* addr,register long rowBytes,register unsigned long color);
static void _1_8(register char* addr,register long rowBytes,register unsigned long color);
static void _2_8(register char* addr,register long rowBytes,register unsigned long color);
static void _3_8(register char* addr,register long rowBytes,register unsigned long color);
static void _4_8(register char* addr,register long rowBytes,register unsigned long color);
static void _5_8(register char* addr,register long rowBytes,register unsigned long color);
static void _6_8(register char* addr,register long rowBytes,register unsigned long color);
static void _7_8(register char* addr,register long rowBytes,register unsigned long color);
static void _8_8(register char* addr,register long rowBytes,register unsigned long color);
static void _9_8(register char* addr,register long rowBytes,register unsigned long color);
static void _colon_8(register char* addr,register long rowBytes,register unsigned long color);
static void _semicolon_8(register char* addr,register long rowbytes,register unsigned long color);
static void _lt_8(register char* addr,register long rowbytes,register unsigned long color);
static void _equals_8(register char* addr,register long rowBytes,register unsigned long color);
static void _gt_8(register char* addr,register long rowBytes,register unsigned long color);
static void _question_8(register char* addr,register long rowbytes,register unsigned long color);
static void _at_8(register char* addr,register long rowbytes,register unsigned long color);
static void _A_8(register char* addr,register long rowBytes,register unsigned long color);
static void _B_8(register char* addr,register long rowBytes,register unsigned long color);
static void _C_8(register char* addr,register long rowBytes,register unsigned long color);
static void _D_8(register char* addr,register long rowBytes,register unsigned long color);
static void _E_8(register char* addr,register long rowBytes,register unsigned long color);
static void _F_8(register char* addr,register long rowBytes,register unsigned long color);
static void _G_8(register char* addr,register long rowBytes,register unsigned long color);
static void _H_8(register char* addr,register long rowBytes,register unsigned long color);
static void _I_8(register char* addr,register long rowBytes,register unsigned long color);
static void _J_8(register char* addr,register long rowBytes,register unsigned long color);
static void _K_8(register char* addr,register long rowBytes,register unsigned long color);
static void _L_8(register char* addr,register long rowBytes,register unsigned long color);
static void _M_8(register char* addr,register long rowBytes,register unsigned long color);
static void _N_8(register char* addr,register long rowBytes,register unsigned long color);
static void _O_8(register char* addr,register long rowBytes,register unsigned long color);
static void _P_8(register char* addr,register long rowBytes,register unsigned long color);
static void _Q_8(register char* addr,register long rowBytes,register unsigned long color);
static void _R_8(register char* addr,register long rowBytes,register unsigned long color);
static void _S_8(register char* addr,register long rowBytes,register unsigned long color);
static void _T_8(register char* addr,register long rowBytes,register unsigned long color);
static void _U_8(register char* addr,register long rowBytes,register unsigned long color);
static void _V_8(register char* addr,register long rowBytes,register unsigned long color);
static void _W_8(register char* addr,register long rowBytes,register unsigned long color);
static void _X_8(register char* addr,register long rowBytes,register unsigned long color);
static void _Y_8(register char* addr,register long rowBytes,register unsigned long color);
static void _Z_8(register char* addr,register long rowBytes,register unsigned long color);
static void _open_bracket_8(register char* addr,register long rowBytes,register unsigned long color);
static void _bs_8(register char* addr,register long rowbytes,register unsigned long color);
static void _close_bracket_8(register char* addr,register long rowBytes,register unsigned long color);
static void _carat_8(register char* addr,register long rowbytes,register unsigned long color);
static void _underscore_8(register char* addr,register long rowBytes,register unsigned long color);
static void _accent_grave_8(register char* addr,register long rowbytes,register unsigned long color);
static void _a_8(register char* addr,register long rowBytes,register unsigned long color);
static void _b_8(register char* addr,register long rowBytes,register unsigned long color);
static void _c_8(register char* addr,register long rowBytes,register unsigned long color);
static void _d_8(register char* addr,register long rowBytes,register unsigned long color);
static void _e_8(register char* addr,register long rowBytes,register unsigned long color);
static void _f_8(register char* addr,register long rowBytes,register unsigned long color);
static void _g_8(register char* addr,register long rowBytes,register unsigned long color);
static void _h_8(register char* addr,register long rowBytes,register unsigned long color);
static void _i_8(register char* addr,register long rowBytes,register unsigned long color);
static void _j_8(register char* addr,register long rowBytes,register unsigned long color);
static void _k_8(register char* addr,register long rowBytes,register unsigned long color);
static void _l_8(register char* addr,register long rowBytes,register unsigned long color);
static void _m_8(register char* addr,register long rowBytes,register unsigned long color);
static void _n_8(register char* addr,register long rowBytes,register unsigned long color);
static void _o_8(register char* addr,register long rowBytes,register unsigned long color);
static void _p_8(register char* addr,register long rowBytes,register unsigned long color);
static void _q_8(register char* addr,register long rowBytes,register unsigned long color);
static void _r_8(register char* addr,register long rowBytes,register unsigned long color);
static void _s_8(register char* addr,register long rowBytes,register unsigned long color);
static void _t_8(register char* addr,register long rowBytes,register unsigned long color);
static void _u_8(register char* addr,register long rowBytes,register unsigned long color);
static void _v_8(register char* addr,register long rowBytes,register unsigned long color);
static void _w_8(register char* addr,register long rowBytes,register unsigned long color);
static void _x_8(register char* addr,register long rowBytes,register unsigned long color);
static void _y_8(register char* addr,register long rowBytes,register unsigned long color);
static void _z_8(register char* addr,register long rowBytes,register unsigned long color);
static void _open_brace_8(register char* addr,register long rowbytes,register unsigned long color);
static void _bar_8(register char* addr,register long rowBytes,register unsigned long color);
static void _close_brace_8(register char* addr,register long rowbytes,register unsigned long color);
static void _tilde_8(register char* addr,register long rowbytes,register unsigned long color);
static void _delete_8(register char* addr,register long rowbytes,register unsigned long color);
static void _undef_8(register char* addr,register long rowBytes,register unsigned long color);

static CharBlitterProcPtr funcList_8[128] = {	_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,
									_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,
									_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,
									_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,_undef_8,
									_space_8,_exclam_8,_quote_8,_pound_8,_dollar_8,_percent_8,_ampersand_8,_apost_8,
									_open_parens_8,_close_parens_8,_star_8,_plus_8,_comma_8,_minus_8,_period_8,_fs_8,
									_0_8,_1_8,_2_8,_3_8,_4_8,_5_8,_6_8,_7_8,
									_8_8,_9_8,_colon_8,_semicolon_8,_lt_8,_equals_8,_gt_8,_question_8,
									_at_8,_A_8,_B_8,_C_8,_D_8,_E_8,_F_8,_G_8,
									_H_8,_I_8,_J_8,_K_8,_L_8,_M_8,_N_8,_O_8,
									_P_8,_Q_8,_R_8,_S_8,_T_8,_U_8,_V_8,_W_8,
									_X_8,_Y_8,_Z_8,_open_bracket_8,_bs_8,_close_bracket_8,_carat_8,_underscore_8,
									_accent_grave_8,_a_8,_b_8,_c_8,_d_8,_e_8,_f_8,_g_8,
									_h_8,_i_8,_j_8,_k_8,_l_8,_m_8,_n_8,_o_8,
									_p_8,_q_8,_r_8,_s_8,_t_8,_u_8,_v_8,_w_8,
									_x_8,_y_8,_z_8,_open_brace_8,_bar_8,_close_brace_8,_tilde_8,_delete_8};
static unsigned long	memoryBlock[(sizeof(CompiledFont)+3)/4];
CompiledFont*		FONT_NAME;

void FONT_INIT_FUNC(void)
{
	// This is because operator new cannot be used until the MicroKernel initializes the memory manager.  Gonzales is used in the NanoKernel long before
	// the memory manager is initialized, so we preallocate a block of memory for this font in the globals area of the nanokernel.
	FONT_NAME = new((void*)memoryBlock) CompiledFont(WIDTH+1,HEIGHT+1,funcList_8,nil,nil);
}

static __asm__ void _space_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// That thing you put between words
	blr;
}

static __asm__ void _exclam_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// !
	blr;
}

static __asm__ void _quote_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// "
	blr;
}

static __asm__ void _pound_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// #
	blr;
}

static __asm__ void _dollar_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// $
	blr;
}

static __asm__ void _percent_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// %
	blr;
}

static __asm__ void _ampersand_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// &
	blr;
}

static __asm__ void _apost_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// '
	blr;
}

static __asm__ void _open_parens_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// (
	blr;
}

static __asm__ void _close_parens_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// )
	blr;
}

static __asm__ void _star_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// *
	blr;
}

static __asm__ void _plus_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// +
	blr;
}

static __asm__ void _comma_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ,
	blr;
}

static __asm__ void _minus_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// -
	blr;
}

static __asm__ void _period_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// .
	blr;
}

static __asm__ void _fs_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// /
	blr;
}

static __asm__ void _0_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 0
	blr;
}

static __asm__ void _1_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 1
	blr;
}

static __asm__ void _2_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 2
	blr;
}

static __asm__ void _3_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 3
	blr;
}

static __asm__ void _4_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 4
	blr;
}

static __asm__ void _5_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 5
	blr;
}

static __asm__ void _6_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 6
	blr;
}

static __asm__ void _7_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 7
	blr;
}

static __asm__ void _8_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 8
	blr;
}

static __asm__ void _9_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// 9
	blr;
}

static __asm__ void _colon_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// :
	blr;
}

static __asm__ void _semicolon_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ;
	blr;
}

static __asm__ void _lt_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// <
	blr;
}

static __asm__ void _equals_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// =
	blr;
}

static __asm__ void _gt_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// >
	blr;
}

static __asm__ void _question_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ?
	blr;
}

static __asm__ void _at_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// @
	blr;
}

static __asm__ void _A_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// A
	LOWERCASE_ENTRY(_a_8);
	blr;
}

static __asm__ void _B_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// B
	LOWERCASE_ENTRY(_b_8);
	blr;
}

static __asm__ void _C_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// C
	LOWERCASE_ENTRY(_c_8);
	blr;
}

static __asm__ void _D_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// D
	LOWERCASE_ENTRY(_d_8);
	blr;
}

static __asm__ void _E_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// E
	LOWERCASE_ENTRY(_e_8);
	blr;
}
static __asm__ void _F_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// F
	LOWERCASE_ENTRY(_f_8);
	blr;
}

static __asm__ void _G_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// G
	LOWERCASE_ENTRY(_g_8);
	blr;
}

static __asm__ void _H_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// H
	LOWERCASE_ENTRY(_h_8);
	blr;
}

static __asm__ void _I_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// I
	LOWERCASE_ENTRY(_i_8);
	blr;
}

static __asm__ void _J_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// J
	LOWERCASE_ENTRY(_j_8);
	blr;
}

static __asm__ void _K_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// K
	LOWERCASE_ENTRY(_k_8);
	blr;
}

static __asm__ void _L_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// L
	LOWERCASE_ENTRY(_l_8);
	blr;
}

static __asm__ void _M_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// M
	LOWERCASE_ENTRY(_m_8);
	blr;
}

static __asm__ void _N_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// N
	LOWERCASE_ENTRY(_n_8);
	blr;
}

static __asm__ void _O_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// O
	LOWERCASE_ENTRY(_o_8);
	blr;
}

static __asm__ void _P_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// P
	LOWERCASE_ENTRY(_p_8);
	blr;
}

static __asm__ void _Q_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// Q
	LOWERCASE_ENTRY(_q_8);
	blr;
}

static __asm__ void _R_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// R
	LOWERCASE_ENTRY(_r_8);
	blr;
}

static __asm__ void _S_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// S
	LOWERCASE_ENTRY(_s_8);
	blr;
}

static __asm__ void _T_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// T
	LOWERCASE_ENTRY(_t_8);
	blr;
}

static __asm__ void _U_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// U
	LOWERCASE_ENTRY(_u_8);
	blr;
}

static __asm__ void _V_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// V
	LOWERCASE_ENTRY(_v_8);
	blr;
}

static __asm__ void _W_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// W
	LOWERCASE_ENTRY(_w_8);
	blr;
}

static __asm__ void _X_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// X
	LOWERCASE_ENTRY(_x_8);
	blr;
}

static __asm__ void _Y_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// Y
	LOWERCASE_ENTRY(_y_8);
	blr;
}

static __asm__ void _Z_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// Z
	LOWERCASE_ENTRY(_z_8);
	blr;
}

static __asm__ void _open_bracket_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// [
	blr;
}

static __asm__ void _bs_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// \ 
	blr;
}

static __asm__ void _close_bracket_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ]
	blr;
}

static __asm__ void _carat_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ^
	blr;
}

static __asm__ void _underscore_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// _
	blr;
}

static __asm__ void _accent_grave_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// `
	blr;
}

static __asm__ void _open_brace_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// {
	blr;
}

static __asm__ void _bar_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// |
	blr;
}

static __asm__ void _close_brace_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// }
	blr;
}

static __asm__ void _tilde_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// ~
	blr;
}

static __asm__ void _delete_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// This character is guaranteed to be a solid box.
	blr;
}

static __asm__ void _undef_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// This gets printed whenever an undefined character tries to go out
	blr;
}

#if HAS_LOWERCASE

static __asm__ void _a_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// a
	blr;
}

static __asm__ void _b_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// b
	blr;
}

static __asm__ void _c_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// c
	blr;
}

static __asm__ void _d_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// d
	blr;
}

static __asm__ void _e_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// e
	blr;
}
static __asm__ void _f_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// f
	blr;
}

static __asm__ void _g_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// g
	blr;
}

static __asm__ void _h_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// h
	blr;
}

static __asm__ void _i_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// i
	blr;
}

static __asm__ void _j_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// j
	blr;
}

static __asm__ void _k_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// k
	blr;
}

static __asm__ void _l_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// l
	blr;
}

static __asm__ void _m_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// m
	blr;
}

static __asm__ void _n_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// n
	blr;
}

static __asm__ void _o_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// o
	blr;
}

static __asm__ void _p_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// p
	blr;
}

static __asm__ void _q_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// q
	blr;
}

static __asm__ void _r_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// r
	blr;
}

static __asm__ void _s_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// s
	blr;
}

static __asm__ void _t_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// t
	blr;
}

static __asm__ void _u_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// u
	blr;
}

static __asm__ void _v_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// v
	blr;
}

static __asm__ void _w_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// w
	blr;
}

static __asm__ void _x_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// x
	blr;
}

static __asm__ void _y_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// y
	blr;
}

static __asm__ void _z_8(register char* addr,register long rowBytes,register unsigned long color)
{
	// z
	blr;
}

#endif /* HAS_LOWERCASE */