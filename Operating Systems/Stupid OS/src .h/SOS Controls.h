#ifndef __SOS_CONTROLS__
#define __SOS_CONTROLS__

// Generic control
class SOSControl
{
protected:
	ConstASCII8Str		controlName;
	UInt32			controlValue;
	UInt32			x;
	UInt32			y;
	class Console&		con;
	Boolean			selected;
	SOSControl(UInt32 x,UInt32 y,ConstASCII8Str name,UInt32 val,Console& con);
public:
	virtual ~SOSControl();
	
			UInt32	getControlValue();
	virtual	void		setControlValue(UInt32 val);
	virtual	void		selectControl();
	virtual	void		deselectControl();
	virtual	void		draw() = 0;
};

// "Popup" menu control
class SOSPopupControl	:	public SOSControl
{
protected:
	ConstASCII8Str*	nameTable;
	UInt32			numItems;
public:
	SOSPopupControl(UInt32 x,UInt32 y,ConstASCII8Str name,UInt32 selectedItem,Console& con,ConstASCII8Str* nameTable,UInt32 numItems);

	virtual	void		setControlValue(UInt32 val);
			void		nextItem();
			void		prevItem();
	virtual	void		selectControl();
	virtual	void		deselectControl();
	virtual	void		draw();
};

#endif /* __SOS_CONTROLS__ */
