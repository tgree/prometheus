/*
	debugger_parser.cp
	Copyright © 1998 by Patrick Varilly

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
	???
	
	Version History
	============
	Terry Greeniaus	-	Monday, 8 June 98	-	Added GNU license to file
*/
#include "Types.h"
#include "debugger_parser.h"
#include "debugger_support.h"
#include "NKDebuggerInternal.h"
#include "NKDebugger.h"
#include "NKThreads.h"

Boolean				err;

#define PARSE(val,proc)	do { val = proc(); if(err) return 0; } while (0)
#define ERROR						do { err = true; return 0; } while(0)

Int16				curToken;

typedef Int16 firstSymTable[19];

firstSymTable			first[6] =
	{ { 'g', tC_IL, tC_IP, 's', 't', tC_ST, tC_HELP, '?', tC_RS, tC_SB, tC_SH,
		tC_SW, tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, tSEGREGISTER, tC_DM },
		{ tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, tSEGREGISTER },
		{ '@', tCONSTANT, tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, '~', '-', '(' },
		{ '@', tCONSTANT, tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, '~', '-', '(' },
		{ '@', tCONSTANT, tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, '~', '-', '(' },
		{ tCONSTANT, tREGISTER, tR_PC, tR_LR, tR_SP, tR_RTOC, '~', '-', '(' } };

Int32 program( void );
Int32 statement( void );
Int32 aRegister( void );
Int32 expr( void );
Int32 term( void );
Int32 factor( void );
Int32 value( void );
Boolean canBe( Int8 nonTerminal );

Boolean parseDebug(void)
{
	err = false;
	
	curToken = getToken();
	program();
	
	if( err || (curToken != 0) )
		return false;
	else
		return true;
}

Int32 program( void )
{
	Int32			temp;
	
	PARSE(temp, statement);
	while( curToken == ';' )
	{
		curToken = getToken();
		PARSE(temp, statement);
	}
	
	return 0;
	Panic("program() fell though!");	// PAT this function needs a return value
}

Int32 statement( void )
{
	Int32			exprValue, otherValue;
	
	switch( curToken )
	{
		case 'g':
			curToken = getToken();
			if( canBe( ntEXPR ) )
				PARSE(exprValue,expr);
			else
				exprValue = getRegValue( REG_PC );
			
			goFrom( exprValue );
			break;
		case tC_IL:
			curToken = getToken();
			if( canBe( ntEXPR ) )
				PARSE(exprValue,expr);
			else
				exprValue = getRegValue( REG_PC );
			
			disassembleAt( exprValue, false );
			break;
		case tC_DM:
			curToken = getToken();
			if( canBe( ntEXPR ) )
				PARSE(exprValue,expr);
			else
				exprValue = getRegValue( REG_PC );
			
			displayMemory( exprValue );
			break;
		case tC_IP:
			curToken = getToken();
			if( canBe( ntEXPR ) )
				PARSE(exprValue,expr);
			else
				exprValue = getRegValue( REG_PC );
			
			disassembleAt( exprValue, true );
			break;
		case 's':
			curToken = getToken();
			
			step( false );
			break;
		case 't':
			curToken = getToken();
			
			step( false );
			break;
		case tC_ST:
			curToken = getToken();
			
			stackTrace();
			break;
		case tC_HELP:
		case '?':
			curToken = getToken();
			
			printHelp();
			break;
		case tC_RS:
			curToken = getToken();
			
			restart();
			break;
		case tC_SB:
			curToken = getToken();
			PARSE( exprValue, expr );
			if( curToken == ',' )
				curToken = getToken();
			else
				ERROR;
			PARSE( otherValue, expr );
			
			// Do something here
			break;
		case tC_SH:
			curToken = getToken();
			PARSE( exprValue, expr );
			if( curToken == ',' )
				curToken = getToken();
			else
				ERROR;
			PARSE( otherValue, expr );
			
			// Do something here
			break;
		case tC_SW:
			curToken = getToken();
			PARSE( exprValue, expr );
			if( curToken == ',' )
				curToken = getToken();
			else
				ERROR;
			PARSE( otherValue, expr );
			
			// Do something here
			break;
		case tC_TL:
			curToken = getToken();
			
			NKDebuggerPrintThreads();
			break;
		default:
			// Handle register = expr and register
			if( canBe( ntREGISTER ) )
			{
				PARSE( exprValue, aRegister );
				if( curToken == '=' )
				{
					curToken = getToken();
					PARSE( otherValue, expr );
					
					setRegValue( exprValue, otherValue );
				}
				else
					printNum( getRegValue( exprValue ) );
			}
			else
				ERROR;
			break;
	}
	
	return 0;
	Panic("statement() fell though");	// PAT this needs a return value
}

Int32 aRegister( void )
{
	switch( curToken )
	{
		case tREGISTER:
			curToken = getToken();
			
			if( (yylval < 0) || (yylval > 31) )
			{
				print( "Invalid register!\n" );
				ERROR;
			}
			else
				return yylval;
			break;
		case tSEGREGISTER:
			curToken = getToken();
			
			if( (yylval < 0) || (yylval > 15) )
			{
				print( "Invalid segment register!\n" );
				ERROR;
			}
			else
				return (yylval | 0x80000000);
			break;
		break;
		case tR_PC:
			curToken = getToken();
			return REG_PC;
		case tR_SP:
			curToken = getToken();
			return REG_SP;
		case tR_LR:
			curToken = getToken();
			return REG_LR;
		case tR_RTOC:
			curToken = getToken();
			return REG_RTOC;
		default:
			ERROR;
	}
}

Int32 expr( void )
{
	Int32		termValue, total;
	Int16		tok;
	
	if( canBe( ntTERM ) )
	{
		PARSE( termValue = total, term );
		
		while( (curToken == '+') || (curToken == '-') )
		{
			tok = curToken;
			curToken = getToken();
			if( canBe( ntTERM ) )
				PARSE( termValue, term );
			else
				ERROR;
			
			if( tok == '+' )
				total += termValue;
			else
				total -= termValue;
		}
		
		return total;
	}
	else
		ERROR;
}

Int32 term( void )
{
	Int32		factorValue, otherValue;
	Int16		tok;
	
	if( canBe( ntFACTOR ) )
		PARSE( factorValue, factor );
	else
		ERROR;
	
	if( (curToken != '|') && (curToken != '&') && (curToken != '*') && (curToken != '/') && 
		(curToken != '%') && (curToken != tSHIFTLEFT) && (curToken != tSHIFTRIGHT) )
		return factorValue;
	
	tok = curToken;
	curToken = getToken();
	
	if( canBe( ntFACTOR ) )
		PARSE( otherValue, factor );
	else
		ERROR;
	
	switch( tok )
	{
		case '|':		return factorValue | otherValue;
		case '&':		return factorValue & otherValue;
		case '*':		return factorValue * otherValue;
		case '/':		return factorValue / otherValue;
		case '%':		return factorValue % otherValue;
		case tSHIFTLEFT:		return factorValue << otherValue;
		case tSHIFTRIGHT:		return factorValue >> otherValue;
		default:		ERROR;
	}
}

Int32 factor( void )
{
	Int32			theValue;
	
	if( curToken == '@' )
	{
		curToken = getToken();
		if( canBe( ntVALUE ) )
			PARSE( theValue, value );
		else
			ERROR;
		
		// Do something more appropiate here
		return theValue;
	}
	else if( canBe( ntVALUE ) )
	{
		PARSE( theValue, value );
		if( curToken == '^' )
		{
			curToken = getToken();
			// Do something more useful here
		}
		
		return theValue;
	}
	else
		ERROR;
}

Int32 value( void )
{
	Int32		theValue;
	
	if( curToken == tCONSTANT )
	{
		theValue = yylval;
		
		curToken = getToken();
		return theValue;
	}
	else if( canBe( ntREGISTER ) )
	{
		PARSE( theValue, aRegister );
		theValue = getRegValue( theValue );
		return theValue;
	}
	else switch( curToken )
	{
		case '~':
			curToken = getToken();
			if( canBe( ntVALUE ) )
				PARSE( theValue, value );
			else
				ERROR;
			
			theValue = ~theValue;
			return theValue;
		case '-':
			curToken = getToken();
			if( canBe( ntVALUE ) )
				PARSE( theValue, value );
			else
				ERROR;
			
			theValue = -theValue;
			return theValue;
		case '(':
			curToken = getToken();
			if( canBe( ntEXPR ) )
				PARSE( theValue, expr );
			else
				ERROR;
			
			if( curToken == ')' )
				curToken = getToken();
			else
				ERROR;
			return theValue;
	}
	
	return 0;
	Panic("value() fell through!\n");	// PAT this needs a return value!
}

Boolean canBe( Int8 nonTerminal )
{
	if( curToken == 0 )
		return false;
	
	for( Int8 i = 0; i < sizeof(firstSymTable)/sizeof(Int16) ; i++ )
	{
		if( curToken == first[nonTerminal][i] )
			return true;
	}
	
	return false;
}