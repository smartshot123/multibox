#pragma once
#include "MainFrame.h" //includes <wx/wx.h>
#include <wx/socket.h>
//global options
extern wxUint8 g_mouseSpeed;
extern wxString g_serverName;
extern wxUint32 g_basePort;
extern long g_reconnectTimeout;

extern wxSocketClient * g_clientSocket;
extern wxDatagramSocket * g_datagramSocket;
extern wxTimer g_connectionTimer;
