#include "NKUnmangler.h"
#include "ANSI.h"
#include "Kernel Types.h"

Int32 decodeSymbol( ASCII8Str source, ASCII8Str dest );
void decodeArguments( ASCII8Str source, ASCII8Str dest );
Int32 decodeArgument( ASCII8Str source, ASCII8Str dest );

#define isNum(x)		(((x) >= '0') && ((x) <= '9'))
#define isUpper(x)		(((x) >= 'A') && ((x) <= 'Z'))

ASCII8Str Unmangle( ASCII8Str source, ASCII8Str dest )
{
	Int32		i;
	ASCII8Str		origDest = dest;
	
	if( *source == '.' )
		source++;
	
	ASCII8				functionName[100] = {0};
	
	// Handle special cases (like __ct__)
	if( strncmp( source, "__ct__", 5 ) == 0 )
	{
		i = decodeSymbol( source, dest );
		strcpy(functionName, dest);
		dest += strlen(dest);
		source += i;
		*dest++ = ':';
		*dest++ = ':';
		strcpy( dest, functionName );
		dest += strlen(dest);
		goto decodeArgs;
	}
	else if( strncmp( source, "__dt__", 5 ) == 0 )
	{
		i = decodeSymbol( source, dest );
		strcpy(functionName, dest);
		dest += strlen(dest);
		source += i;
		*dest++ = ':';
		*dest++ = ':';
		*dest++ = '~';
		strcpy( dest, functionName );
		dest += strlen(dest);
		goto decodeArgs;
	}
	
	i = 0;
	while( source[0] != 0 )
	{
		if( (source[0] == '_') && (source[1] == '_') )
			break;
		functionName[i++] = *source++;
	}
	functionName[i] = 0;
	
	if( *source == 0 )
	{
		// C function
		strcpy(dest,functionName);
		return origDest;
	}
	
	// Skip the underscores
	source += 2;
	if( *source == 'F' )		// C++ class-less function
	{
		strcpy( dest, functionName );
		dest += strlen(functionName);
	}
	else
	{					// C++ "correct" function
		i = decodeSymbol( source, dest );
		dest += strlen(dest);
		source += i;
		*dest++ = ':';
		*dest++ = ':';
		strcpy( dest, functionName );
		dest += strlen(dest);
	}
	
decodeArgs:
	*dest++ = '(';
	*dest++ = ' ';
	decodeArguments( ++source, dest );
	dest += strlen(dest);
	*dest++ = ' ';
	*dest++ = ')';
	*dest = 0;
	return origDest;
}

Int32 decodeSymbol( ASCII8Str source, ASCII8Str dest )
{
	Int32				symbolLen,i;
	
	symbolLen = 0;
	i = 0;
	while( isNum(*source) )
	{
		symbolLen *= 10;
		symbolLen += *source - '0';
		source++;
		i++;
	}
	i += symbolLen;
	
	while( symbolLen > 0 )
	{
		Int32			theLen;
		
		*dest++ = *source;
		if( *source++ == '<' )
		{
			theLen = decodeSymbol( source, dest );
			source += theLen;
			symbolLen -= theLen;
		}
		--symbolLen;
	}
	
	*dest = 0;
	
	return i;
}

void decodeArguments( ASCII8Str source, ASCII8Str dest )
{
	Boolean		isReference, isConst, isUnsigned;
	Int32		numPointers, theLen;
	
	while( *source != 0 )
	{
		isReference = isConst = isUnsigned = false;
		numPointers = 0;
		
		if( *source == 'R' )
		{
			isReference = true;
			source++;
		}
		
		while( *source == 'P' )
		{
			numPointers++;
			source++;
		}
		
		if( *source == 'C' )
		{
			isConst = true;
			source++;
		}
		
		if( *source == 'U' )
		{
			isUnsigned = true;
			source++;
		}
		
		if( isConst )
		{
			strcpy( dest, "const " );
			dest += 6;
		}
		
		if( isUnsigned )
		{
			strcpy( dest, "unsigned " );
			dest += 9;
		}
		
		theLen = decodeArgument( source, dest );
		dest += strlen(dest);
		source += theLen;
		
		while( numPointers )
		{
			*dest++ = '*';
			numPointers--;
		}
		
		if( isReference )
			*dest++ = '&';
		
		if( *source != 0 )
		{
			strcpy( dest, ", " );
			dest += 2;
		}
	}
	
	*dest = 0;
}

Int32 decodeArgument( ASCII8Str source, ASCII8Str dest )
{
	switch( *source )
	{
		case 'v':
			strcpy( dest, "void" );
			return 1;
		case 'c':
			strcpy( dest, "char" );
			return 1;
		case 's':
			strcpy( dest, "short" );
			return 1;
		case 'l':
			strcpy( dest, "long" );
			return 1;
		case 'f':
			strcpy( dest, "float" );
			return 1;
		case 'd':
			strcpy( dest, "double" );
			return 1;
		case 'x':
			strcpy( dest, "long long" );
			return 1;
		default:
			if( isNum(*source) )
				return decodeSymbol( source, dest );
			else
				return 0;
	}
}