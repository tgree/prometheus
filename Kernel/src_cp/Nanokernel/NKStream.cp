#include "NKStream.h"
#include "ANSI.h"
#include "Streams.h"

NKStream& operator<<(NKStream& s,ConstASCII8Str str)
{
	if(!s.write)
		return s;
	UInt32 len = strlen(str);
	while(len--)
		s.write(s,*str++);
	return s;
}

NKStream& operator<<(NKStream& s,UInt32 n)
{
	ASCII8	str[20];
	if(s.baseMode == baseModeDec)
		unum2str(n,str);
	else
		num2hex(n,str);
	s << str;
	return s;
}

NKStream& operator<<(NKStream& s,const StreamMessage& m)
{
	if(m.type == decMessage)
		s.baseMode = baseModeDec;
	else if(m.type == hexMessage)
		s.baseMode = baseModeHex;
	else if(s.msg)
		s.msg(s,m);
	return s;
}

NKStream& operator>>(NKStream& s,ASCII8Str str)
{
	if(!s.read)
	{
		str[0] = 0;
		return s;
	}
	for(;;)
	{
		*str = s.read(s);
		if(*str == '\r' || *str == '\n')
		{
			*str = 0;
			return s;
		}
		if(s.write)
			s.write(s,*str);
		str++;
	}
}

NKStream& operator>>(NKStream& s,UInt32& n)
{
	n = 0;
	if(!s.read)
		return s;
	
	ASCII8	str[20] = {0};
	UInt32	len = 0;
	while(len <= 11)
	{
		str[len] = s.read(s);
		if(!isdigit(str[len]) || !(len == 0 && str[len] == '-'))
			break;
		if(s.write)
			s.write(s,str[len]);
		len++;
	}
	str[len] = 0;
	for(UInt32 i=0;i<len;i++)
	{
		if(str[i] == '-')
			continue;
		n *= 10;
		n += str[i] - 48;
	}
	if(str[0] == '-')
		n = -n;
	
	return s;
}
