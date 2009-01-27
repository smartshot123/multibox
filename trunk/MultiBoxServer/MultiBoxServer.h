#pragma once
#include "MainFrame.h" //includes <wx/wx.h>
#include <wx/socket.h>
//global options
extern bool g_broadcastMouse;
extern bool g_broadcastKeyboard;
extern bool g_setMousePosition;
extern bool g_moveClipboard;
extern bool g_lockToScreen;
extern wxUint32 g_basePort; //+0:UDP Server; +1:TCP; +2:UDP Client
extern wxUint32 g_broadcastKeyboardBinding;
extern wxUint32 g_broadcastMouseBinding;
extern wxUint32 g_lockToScreenBinding;

struct MultiBoxServerApp : wxApp
{
	virtual int OnExit();
	virtual bool OnInit();
	
	void UDPSocketCallback(wxSocketEvent & event);
	void ListenSocketCallback(wxSocketEvent & event);
	void ClientSocketCallback(wxSocketEvent & event);
	
	void SetupConnections();
	void RebuildLayoutChoices();
	void RemoveClient(wxSocketBase * socket);
	void AddClient(wxSocketBase * server);
	
protected:
	DECLARE_EVENT_TABLE();
};
