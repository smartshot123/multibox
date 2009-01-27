#pragma once
#define EXTRA_INFO_NUMBER 62602 //random number used to identify events as injected by us or not

#include <wx/wx.h>

namespace MessageTypes
{
	enum
	{
		S_MOUSEMOVE, //relative mouse move
		S_MOUSEPOS, //absolute mouse move
		S_MOUSECLICK, //mouse click
		S_MOUSEWHEEL, //mouse wheel rotated/clicked
		S_KEYINPUT, //keyboard press or release
		S_FOCUSLOST, //client focus lost
		S_FOCUSGAINED, //client focus gained
		S_CLIPBOARD, //clipboard forwarding to the active client
		S_KEEPALIVE, //server response to a client keep-alive
		C_EDGEREACHED, //edge of screen touched with mouse
		C_CLIPBOARD, //clipboard data transfer
		C_KEEPALIVE, //keep alive packet
	};
}

struct MouseMove
{
	wxInt32 xDistance;
	wxInt32 yDistance;
};

struct MousePosition
{
	wxInt32 xPosition;
	wxInt32 yPosition;
};

struct MouseClick
{
	wxInt32 type;
	wxInt32 data;
};

struct MouseWheel
{
	wxInt16 delta;
};

struct KeyInput
{
	wxUint16 scanCode;
	wxUint32 flags;
};

struct EdgeReached
{
	enum Edge
	{
		LEFT,
		RIGHT,
		TOP,
		BOTTOM,
	} edge;
	wxInt32 x;
	wxInt32 y;
};

struct ClipboardData
{
	enum Type
	{
		TEXT,
		BITMAP,
	} type;
	wxUint32 length;
	void * data;
};
