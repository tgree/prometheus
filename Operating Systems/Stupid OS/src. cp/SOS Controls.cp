#include "NKDebuggerNub.h"
#include "SOS Controls.h"
#include "Kernel Console.h"
#include "ANSI.h"

SOSControl::SOSControl(UInt32 _x,UInt32 _y,ConstASCII8Str name,UInt32 val,Console& _con):
	con(_con)
{
	controlName = name;
	controlValue = val;
	x = _x;
	y = _y;
	selected = false;
}

SOSControl::~SOSControl()
{
}

UInt32 SOSControl::getControlValue()
{
	return controlValue;
}

void SOSControl::setControlValue(UInt32 val)
{
	controlValue = val;
}

void SOSControl::selectControl()
{
	selected = true;
}

void SOSControl::deselectControl()
{
	selected = false;
}

SOSPopupControl::SOSPopupControl(UInt32 x,UInt32 y,ConstASCII8Str name,UInt32 selectedItem,Console& con,ConstASCII8Str* _nameTable,UInt32 _numItems):
	SOSControl(x,y,name,selectedItem,con)
{
	nameTable = _nameTable;
	numItems = _numItems;
}

void SOSPopupControl::setControlValue(UInt32 val)
{
	FatalAssert(val < numItems);
	if(val != getControlValue())
	{
		con.moveCursorAbs(x,y);
		con << controlName << " ";
		UInt32	itemLen = strlen(nameTable[controlValue]);
		for(UInt32 i=0;i<itemLen;i++)
			con << " ";
		SOSControl::setControlValue(val);
		draw();
	}
}

void SOSPopupControl::nextItem()
{
	if(getControlValue() + 1 < numItems)
		setControlValue(getControlValue() + 1);
}

void SOSPopupControl::prevItem()
{
	if(getControlValue())
		setControlValue(getControlValue() - 1);
}

void SOSPopupControl::draw()
{
	con.moveCursorAbs(x,y);
	con << controlName << " ";
	if(selected)
		con.inverseText();
	con << nameTable[controlValue];
	con.plainText();
}

void SOSPopupControl::selectControl()
{
	if(!selected)
	{
		SOSControl::selectControl();
		draw();
	}
}

void SOSPopupControl::deselectControl()
{
	if(selected)
	{
		SOSControl::deselectControl();
		draw();
	}
}
