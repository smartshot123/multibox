#pragma region Includes
#include "../shared.h"
#include "MultiBoxClient.h"
#include <wx/clipbrd.h>
#include <wx/config.h>
#include <wx/xrc/xmlres.h>
#include <windows.h>
#pragma endregion

#pragma region MultiBoxClientApp Declaration and Implementation
//let the program know that this is my app class
struct MultiBoxClientApp : wxApp
{
	virtual bool OnInit();
	virtual int OnExit();
	
	void KeepAliveConnection(wxTimerEvent & event);
	void AttemptConnection(wxTimerEvent & event);
	void UDPSocketCallback(wxSocketEvent & event);
	void TCPSocketCallback(wxSocketEvent & event);
	
protected:
	DECLARE_EVENT_TABLE();
};

IMPLEMENT_APP(MultiBoxClientApp);

//event IDs
enum
{
	TCP_ID = wxID_HIGHEST + 1,
	UDP_ID,
	TIMER_ID,
	KEEP_ALIVE_ID,
};

//setup event handlers
BEGIN_EVENT_TABLE(MultiBoxClientApp, wxApp)
	EVT_TIMER(KEEP_ALIVE_ID, MultiBoxClientApp::KeepAliveConnection)
	EVT_TIMER(TIMER_ID, MultiBoxClientApp::AttemptConnection)
	EVT_SOCKET(TCP_ID, MultiBoxClientApp::TCPSocketCallback)
	EVT_SOCKET(UDP_ID, MultiBoxClientApp::UDPSocketCallback)
END_EVENT_TABLE()
#pragma endregion

#pragma region Globals
//options
wxUint8 g_mouseSpeed = 8;
wxString g_serverName = L"";
wxUint32 g_basePort = 9435;
long g_reconnectTimeout = 30000;

wxSocketClient * g_clientSocket;
wxDatagramSocket * g_datagramSocket;
wxTimer g_connectionTimer;
wxTimer g_keepAliveTimer;
unsigned long g_lastKeepAliveResponse;
#pragma endregion

//send some data back to the server
void SendDataTCP(wxUint32 message, void * data, wxUint32 length)
{
	//wxLogVerbose(L"Sent: Type: %d   Length: %d", message, length);
	//figure out how big the packet is (including the length value)
	wxUint32 packetLength = sizeof(packetLength) + sizeof(message) + length;
	wxUint8 * packet = new wxUint8[packetLength];
	//pack the message length, the message type and the message data into a byte array
	memcpy(&packet[0], &packetLength, sizeof(packetLength));
	memcpy(&packet[sizeof(packetLength)], &message, sizeof(message));
	memcpy(&packet[sizeof(packetLength) + sizeof(message)], data, length);
	//send the packet, making sure it all goes through
	g_clientSocket->SetFlags(wxSOCKET_WAITALL|wxSOCKET_BLOCK);
	g_clientSocket->Write(packet, packetLength);
	g_clientSocket->SetFlags(wxSOCKET_NOWAIT|wxSOCKET_BLOCK);
	//cleanup
	delete packet;
}

void SendDataUDP(wxUint32 message, void * data, wxUint32 length)
{
	//figure out how big the packet is
	wxUint32 packetLength = sizeof(message) + length;
	wxUint8 * packet = new wxUint8[packetLength];
	//pack the message length, the message type and the message data into a byte array
	memcpy(&packet[0], &message, sizeof(message));
	memcpy(&packet[sizeof(message)], data, length);
	//send the packet
	g_datagramSocket->SetFlags(wxSOCKET_NOWAIT);
	wxIPV4address address;
	g_clientSocket->GetPeer(address);
	address.Service(g_basePort);
	g_datagramSocket->SendTo(address, packet, packetLength);
	//cleanup
	delete packet;
}

//prepare and send the clipboard data to the server
void SendClipboard()
{
	//only send the clipboard once every second, they can get big and we don't want to hammer the connection
	static wxUint32 s_lastClipboard = GetTickCount();
	if(GetTickCount() - s_lastClipboard < 1000) return;
	s_lastClipboard = GetTickCount();
	if(wxTheClipboard->Open())
	{
		//handle text data on the clipboard
		if(wxTheClipboard->IsSupported(wxDF_TEXT))
		{
			//get the text off the clipboard
			ClipboardData clipboard;
			wxUint32 clipboardSize = sizeof(clipboard);
			wxTextDataObject object;
			wxTheClipboard->GetData(object);
			//populate the clipboard structure to send 
			clipboard.type = ClipboardData::TEXT;
			clipboard.length = object.GetText().Length();
			wxUint32 stringLengthInBytes = clipboard.length * sizeof(wxChar);
			//create a buffer that will contain both the clipboard structure and the data
			wxUint32 bufferSize = clipboardSize + stringLengthInBytes;
			wxUint8 * buffer = new wxUint8[bufferSize];
			//copy the structure and the actual text into our buffer
			memcpy(buffer, &clipboard, clipboardSize);
			memcpy(buffer + clipboardSize, (void *)object.GetText().c_str(), stringLengthInBytes);
			SendDataTCP(MessageTypes::C_CLIPBOARD, buffer, bufferSize);
			delete buffer;
		}
		wxTheClipboard->Close();
	}
}

//check to see if we have hit the edge of the screen and handle it
void CheckEdgeOfScreen()
{
	//get the current cursor coordinates
	POINT point;
	GetCursorPos(&point);
	//convert the coordinates to 0-65535 instead of SM_XVIRTUALSCREEN-(SM_CXVIRUALSCREEN - SM_XVIRTUALSCREEN)
	EdgeReached edge;
	edge.x = ((point.x - GetSystemMetrics(SM_XVIRTUALSCREEN)) * 65535) / (GetSystemMetrics(SM_CXVIRTUALSCREEN)-1);
	edge.y = ((point.y - GetSystemMetrics(SM_YVIRTUALSCREEN)) * 65535) / (GetSystemMetrics(SM_CYVIRTUALSCREEN)-1);
	//left side of the screen
	if(edge.x <= 0) edge.edge = EdgeReached::LEFT;
	//right side of the screen
	else if(edge.x >= 65535) edge.edge = EdgeReached::RIGHT;
	//top of the screen
	else if(edge.y <= 0) edge.edge = EdgeReached::TOP;
	//bottom of the screen
	else if(edge.y >= 65535) edge.edge = EdgeReached::BOTTOM;
	//we aren't at any edge of the screen so return
	else return;
	//send the edge message and the clipboard message
	SendDataUDP(MessageTypes::C_EDGEREACHED, &edge, sizeof(edge));
	SendClipboard();
}

//process incoming messages
void ProcessMouseMove(MouseMove * move)
{
	//turn off mouse acceleration
	unsigned int oldAcceleration[3];
	unsigned int oldSpeed;
	unsigned int newAcceleration[3] = {1024, 1024, 0};
	unsigned int newSpeed = g_mouseSpeed;
	SystemParametersInfo(SPI_GETMOUSE, 0, oldAcceleration, 0);
	SystemParametersInfo(SPI_GETMOUSESPEED, 0, &oldSpeed, 0);
	SystemParametersInfo(SPI_SETMOUSE, 0, newAcceleration, SPIF_SENDCHANGE);
	SystemParametersInfo(SPI_SETMOUSESPEED, 0, (void *)newSpeed, SPIF_SENDCHANGE);
	//build the mouse move event
	MOUSEINPUT mouseInput;
	mouseInput.dx = move->xDistance;
	mouseInput.dy = move->yDistance;
	mouseInput.mouseData = 0;
	mouseInput.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_VIRTUALDESK;
	mouseInput.time = 0;
	mouseInput.dwExtraInfo = 0;
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
	//turn mouse acceleration back on
	SystemParametersInfo(SPI_SETMOUSE, 0, oldAcceleration, 0);
	SystemParametersInfo(SPI_SETMOUSESPEED, 0, (void *)oldSpeed, 0);
	//check to see if we have hit the edge of the screen or not
	CheckEdgeOfScreen();
}

void ProcessMousePosition(MousePosition * position)
{
	//build the mouse move event
	MOUSEINPUT mouseInput;
	mouseInput.dx = position->xPosition;
	mouseInput.dy = position->yPosition;
	mouseInput.mouseData = 0;
	mouseInput.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
	mouseInput.time = 0;
	mouseInput.dwExtraInfo = 0;
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
}

void ProcessMouseClick(MouseClick * click)
{
	//build the mouse click event
	MOUSEINPUT mouseInput;
	mouseInput.dx = 0;
	mouseInput.dy = 0;
	mouseInput.mouseData = 0;
	mouseInput.dwFlags = 0;
	switch(click->type)
	{
		case WM_LBUTTONDOWN:
			mouseInput.dwFlags |= MOUSEEVENTF_LEFTDOWN;
			break;
		case WM_LBUTTONUP:
			mouseInput.dwFlags |= MOUSEEVENTF_LEFTUP;
			break;
		case WM_RBUTTONDOWN:
			mouseInput.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
			break;
		case WM_RBUTTONUP:
			mouseInput.dwFlags |= MOUSEEVENTF_RIGHTUP;
			break;
		case WM_MBUTTONDOWN:
			mouseInput.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
			break;
		case WM_MBUTTONUP:
			mouseInput.dwFlags |= MOUSEEVENTF_MIDDLEUP;
			break;
		case WM_XBUTTONDOWN:
			mouseInput.dwFlags |= MOUSEEVENTF_XDOWN;
			mouseInput.mouseData = click->data;
			break;
		case WM_XBUTTONUP:
			mouseInput.dwFlags |= MOUSEEVENTF_XUP;
			mouseInput.mouseData = click->data;
			break;
	}
	mouseInput.time = 0;
	mouseInput.dwExtraInfo = 0;
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
}

void ProcessMouseWheel(MouseWheel * wheel)
{
	//build the mouse wheel event
	MOUSEINPUT mouseInput;
	mouseInput.dx = 0;
	mouseInput.dy = 0;
	mouseInput.mouseData = wheel->delta;
	mouseInput.dwFlags = MOUSEEVENTF_WHEEL;
	mouseInput.time = 0;
	mouseInput.dwExtraInfo = 0;
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi = mouseInput;
	SendInput(1, &input, sizeof(INPUT));
}

void ProcessKeyInput(KeyInput * key)
{
	KEYBDINPUT keyboardInput;
	keyboardInput.wVk = 0;
	keyboardInput.wScan = key->scanCode;
	keyboardInput.dwFlags = KEYEVENTF_SCANCODE;
	if(key->flags & LLKHF_EXTENDED) keyboardInput.dwFlags |= KEYEVENTF_EXTENDEDKEY;
	if(key->flags & LLKHF_UP) keyboardInput.dwFlags |= KEYEVENTF_KEYUP;
	keyboardInput.time = 0;
	keyboardInput.dwExtraInfo = EXTRA_INFO_NUMBER;
	INPUT input;
	input.type = INPUT_KEYBOARD;
	input.ki = keyboardInput;
	SendInput(1, &input, sizeof(INPUT));
}

void ProcessFocusLost()
{
	//move the mouse cursor to the lower right of the screen ("hidden")
	//MOUSEINPUT mouseInput;
	//mouseInput.dx = 65535;
	//mouseInput.dy = 65535;
	//mouseInput.mouseData = 0;
	//mouseInput.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK;
	//mouseInput.time = 0;
	//mouseInput.dwExtraInfo = 0;
	//INPUT input;
	//input.type = INPUT_MOUSE;
	//input.mi = mouseInput;
	//SendInput(1, &input, sizeof(INPUT));
}

void ProcessFocusGained()
{
	
}

void ProcessClipboard(ClipboardData * clipboard)
{
	switch(clipboard->type)
	{
		case ClipboardData::TEXT:
		{
			if(wxTheClipboard->Open())
			{
				wxTheClipboard->SetData(new wxTextDataObject(wxString((wxChar*)clipboard->data, clipboard->length)));
				wxTheClipboard->Close();
			}
		}
	}
}

//attempt to connect to the server
void MultiBoxClientApp::AttemptConnection(wxTimerEvent & WXUNUSED(event))
{
	//create the TCP socket
	g_clientSocket = new wxSocketClient();
	wxIPV4address address;
	address.Hostname(g_serverName);
	address.Service(g_basePort+1);
	g_clientSocket->SetEventHandler(*this, TCP_ID);
	g_clientSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	g_clientSocket->Notify(true);
	g_clientSocket->SetFlags(wxSOCKET_NOWAIT|wxSOCKET_BLOCK);
	g_clientSocket->Connect(address, false);
	
	//create the datagram socket for receiving UDP packets
	wxIPV4address UDPAddress;
	UDPAddress.AnyAddress();
	UDPAddress.Service(g_basePort+2);
	g_datagramSocket = new wxDatagramSocket(UDPAddress, wxSOCKET_NOWAIT);
	if(!g_datagramSocket->Ok())
	{
		//cleanup both sockets since we will be recreating both of them
		g_clientSocket->Destroy();
		g_clientSocket = 0;
		g_datagramSocket->Destroy();
		g_datagramSocket = 0;
		wxLogError(L"Unable to initialize outbound UDP socket on port %d! Next attempt in %d seconds.", g_basePort+2, g_reconnectTimeout);
		if(!g_connectionTimer.Start(g_reconnectTimeout * 1000, true)) wxLogError(L"Could not start the connection timer!");
	}
	g_datagramSocket->SetEventHandler(*this, UDP_ID);
	g_datagramSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
	g_datagramSocket->Notify(true);
}

//keep the TCP connection alive by sending a "ping" every 30 seconds or so
void MultiBoxClientApp::KeepAliveConnection(wxTimerEvent & WXUNUSED(event))
{
	//check to see if it has been more then 30 seconds since the last keep-alive response was received
	if(g_lastKeepAliveResponse + 30000 < GetTickCount())
	{
		g_clientSocket->Destroy();
		g_clientSocket = 0;
		g_datagramSocket->Destroy();
		g_datagramSocket = 0;
		g_keepAliveTimer.Stop();
		wxLogVerbose(L"Connection lost (keep-alive timeout): Next attempt in %d seconds.", g_reconnectTimeout);
		if(!g_connectionTimer.Start(g_reconnectTimeout * 1000, true)) wxLogError(L"Could not start the connection timer!");
		return;
	}
	wxUint32 temp = 0;
	SendDataTCP(MessageTypes::C_KEEPALIVE, &temp, 4);
}

//socket callback
void MultiBoxClientApp::UDPSocketCallback(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
		case wxSOCKET_INPUT:
		{
			try
			{
				wxUint8 buffer[1024];
				wxUint32 messageSize = 0;
				wxUint32 messageType = 0;
				wxIPV4address address;
				
				//receive all waiting messages
				while(true)
				{
					g_datagramSocket->RecvFrom(address, buffer, 1024);
					//break from the loop once we have received all waiting messages
					if(g_datagramSocket->LastCount() == 0) break;
					//throw an exception if there was an error (report it to the user but continue on)
					if(g_datagramSocket->Error()) throw 1;
					//get the messageType from the begining of this message
					messageType = *((wxUint32*)buffer);
					//handle the message based on it's type
					switch(messageType)
					{
						case MessageTypes::S_MOUSEMOVE:
						{
							ProcessMouseMove((MouseMove*)(buffer + sizeof(messageType)));
							break;
						}
						case MessageTypes::S_MOUSEPOS:
						{
							ProcessMousePosition((MousePosition*)(buffer + sizeof(messageType)));
							break;
						}
						case MessageTypes::S_MOUSECLICK:
						{
							ProcessMouseClick((MouseClick*)(buffer + sizeof(messageType)));
							break;
						}
						case MessageTypes::S_MOUSEWHEEL:
						{
							ProcessMouseWheel((MouseWheel*)(buffer + sizeof(messageType)));
							break;
						}
						case MessageTypes::S_KEYINPUT:
						{
							ProcessKeyInput((KeyInput*)(buffer + sizeof(messageType)));
							break;
						}
						default:
						{
							wxLogError(L"Unknown message type (%d) received from server!", messageType);
							break;
						}
					}
				}
			}
			catch(int)
			{
				wxLogError(L"Error receiving the datagram!");
				return;
			}
			break;
		}
	}
}

void MultiBoxClientApp::TCPSocketCallback(wxSocketEvent& event)
{
	switch(event.GetSocketEvent())
	{
		case wxSOCKET_INPUT:
		{
			try
			{
				wxUint8 * buffer;
				wxUint32 messageSize = 0;
				wxUint32 messageType = 0;
				
				//read all available messages
				while(true)
				{
					//check to see if a whole message is here yet, break out of the loop if not
					g_clientSocket->Peek(&messageSize, 4);
					if(g_clientSocket->LastCount() != 4) break;
					buffer = new wxUint8[messageSize];
					g_clientSocket->Peek(buffer, messageSize - 8);
					if(g_clientSocket->LastCount() != messageSize - 8) break;
					
					//read the message
					g_clientSocket->Read(&messageSize, 4);
					g_clientSocket->Read(&messageType, 4);
					g_clientSocket->Read(buffer, messageSize - 8);
					
					//wxLogVerbose(L"Received: Type: %d   Size: %d", messageType, messageSize -8);
					
					switch(messageType)
					{
						case MessageTypes::S_FOCUSLOST:
						{
							ProcessFocusLost();
							break;
						}
						case MessageTypes::S_FOCUSGAINED:
						{
							ProcessFocusGained();
							break;
						}
						case MessageTypes::S_CLIPBOARD:
						{
							ClipboardData * clipboard = (ClipboardData*)buffer;
							clipboard->data = buffer + sizeof(*clipboard);
							ProcessClipboard(clipboard);
							break;
						}
						case MessageTypes::S_KEEPALIVE:
						{
							g_lastKeepAliveResponse = GetTickCount();
							break;
						}
						default:
						{
							wxLogError(L"Unknown message type (%d) received from server!", messageType);
							break;
						}
					}
					delete buffer;
				}
			}
			catch(int)
			{
				wxLogError(L"Not enough bytes!");
				g_clientSocket->Close();
				return;
			}
			break;
		}
		case wxSOCKET_CONNECTION:
		{
			wxIPV4address peer;
			g_clientSocket->GetPeer(peer);
			wxLogVerbose(L"Connected to %s on port %d.", peer.Hostname(), peer.Service());
			//start the keep-alive timer
			g_lastKeepAliveResponse = GetTickCount();
			if(!g_keepAliveTimer.Start(5000, false)) wxLogError(L"Could not start the keep-alive timer!");
			return;
		}
		case wxSOCKET_LOST:
		{
			g_clientSocket->Destroy();
			g_clientSocket = 0;
			g_datagramSocket->Destroy();
			g_datagramSocket = 0;
			wxLogVerbose(L"Connection lost: Next attempt in %d seconds.", g_reconnectTimeout);
			if(!g_connectionTimer.Start(g_reconnectTimeout * 1000, true)) wxLogError(L"Could not start the connection timer!");
			g_keepAliveTimer.Stop();
			return;
		}
		default:
		{
			wxLogError(L"Unexpected socket event!");
			return;
		}
	}
}
//entry point
bool MultiBoxClientApp::OnInit()
{
	//_CrtSetBreakAlloc(6466);
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load(L"MultiBoxClient.xrc");
	
	//create the main frame
	MainFrame* frame = new MainFrame();
	SetTopWindow(frame);
	SetExitOnFrameDelete(true);
	
	//initialize the (re)connect timer
	g_connectionTimer.SetOwner(this, TIMER_ID);
	//do the first connection attempt right away if we have a hostname
	if(!g_serverName.IsEmpty())
	{
		if(!g_connectionTimer.Start(1, true))
		{
			wxLogError(L"Could not start the connection timer!");
		}
	}
	
	//initialize the keep alive timer
	g_keepAliveTimer.SetOwner(this, KEEP_ALIVE_ID);
	
	//set the process priority to high
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	
	return true;
}

//exit point
int MultiBoxClientApp::OnExit()
{
	g_keepAliveTimer.Stop();
	g_connectionTimer.Stop();
	if(g_clientSocket) g_clientSocket->Destroy();
	if(g_datagramSocket) g_datagramSocket->Destroy();
	g_clientSocket = 0;
	g_datagramSocket = 0;
	return wxApp::OnExit();
}
