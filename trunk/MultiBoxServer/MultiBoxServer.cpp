#pragma region Includes
#include "../shared.h"
#include "Client.h"
#include "MultiBoxServer.h"
#include <wx/clipbrd.h>
#include <wx/xrc/xmlres.h>
#include <wx/timer.h>
#include <vector>
#include <windows.h>
//#include <crtdbg.h>
#pragma endregion

#pragma region MultiBoxServerApp Implementation and Events
//let the program know that this is my app class
IMPLEMENT_APP(MultiBoxServerApp);

enum
{
	wxID_LISTEN_SOCKET = wxID_HIGHEST,
	wxID_CLIENT_SOCKET,
	wxID_UDP_SOCKET,
};

//setup event handlers
BEGIN_EVENT_TABLE(MultiBoxServerApp, wxApp)
	EVT_SOCKET(wxID_UDP_SOCKET, MultiBoxServerApp::UDPSocketCallback)
	EVT_SOCKET(wxID_LISTEN_SOCKET, MultiBoxServerApp::ListenSocketCallback)
	EVT_SOCKET(wxID_CLIENT_SOCKET, MultiBoxServerApp::ClientSocketCallback)
END_EVENT_TABLE()
#pragma endregion

#pragma region Globals
//our global mouse/keyboard hooks
HHOOK g_mouseHook = NULL;
HHOOK g_keyboardHook = NULL;

//broadcast toggle
bool g_broadcastMouse;
bool g_broadcastKeyboard;
//what happens when we change clients, migrate the clipboard? mirror the mouse?
bool g_setMousePosition;
bool g_moveClipboard;
bool g_lockToScreen;
wxUint32 g_basePort;
wxUint32 g_broadcastKeyboardBinding;
wxUint32 g_broadcastMouseBinding;
wxUint32 g_lockToScreenBinding;

//a list of all connected clients
std::vector<Client*> g_clientList;
//the currently active client
int g_activeClient = -1;
//keep track of where the cursor is on the server so we know how far it moves
POINT g_lastServerMousePos = {0,0};

//our socket that listens for new clients
wxSocketServer * g_listenSocket = NULL;
wxDatagramSocket * g_datagramSocket = NULL;
#pragma endregion

//functions for sending data over the network
void SendDataUDP(Client * client, wxUint32 message, void * data, wxUint32 length)
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
	client->m_socket->GetPeer(address);
	address.Service(g_basePort + 2);
	g_datagramSocket->SendTo(address, packet, packetLength);
	//cleanup
	delete packet;
}

void SendDataTCP(Client * client, wxUint32 message, void * data, wxUint32 length)
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
	client->m_socket->SetFlags(wxSOCKET_WAITALL|wxSOCKET_BLOCK);
	client->m_socket->Write(packet, packetLength);
	client->m_socket->SetFlags(wxSOCKET_NOWAIT|wxSOCKET_BLOCK);
	//cleanup
	delete packet;
}

//functions for building messages to be sent to clients
void RemoteMouseMove(const POINT & newPoint, Client * client)
{
	//calculate how far we moved from the last known cursor coordinates
	MouseMove move;
	move.xDistance = newPoint.x - g_lastServerMousePos.x;
	move.yDistance = newPoint.y - g_lastServerMousePos.y;
	
	//send the move to the client
	SendDataUDP(client, MessageTypes::S_MOUSEMOVE, &move, sizeof(move));
}

void RemoteMousePosition(const POINT & newPoint, Client * client)
{
	//build our position structure
	MousePosition position;
	position.xPosition = newPoint.x;
	position.yPosition = newPoint.y;
	
	//send the absolute mouse move to the active client
	SendDataUDP(client, MessageTypes::S_MOUSEPOS, &position, sizeof(position));
}

void RemoteMouseClick(const unsigned int type, const unsigned int data, Client * client)
{
	//build our click structure
	MouseClick click;
	click.type = type;
	click.data = data;
	
	//send the click to the client
	SendDataUDP(client, MessageTypes::S_MOUSECLICK, &click, sizeof(click));
}

void RemoteMouseWheel(const wxUint16 delta, Client * client)
{
	//build our wheel structure
	MouseWheel wheel;
	wheel.delta = delta;
	
	//send the wheel to the client
	SendDataUDP(client, MessageTypes::S_MOUSEWHEEL, &wheel, sizeof(wheel));
}

void RemoteKeyInput(long infoLong, Client * client)
{
	KBDLLHOOKSTRUCT * info = (KBDLLHOOKSTRUCT*)infoLong;
	KeyInput key;
	key.scanCode = info->scanCode;
	key.flags = info->flags;
	
	SendDataUDP(client, MessageTypes::S_KEYINPUT, &key, sizeof(key));
}

void RemoteFocusLost(Client * client)
{
	wxUint8 data = 0; //have to send something
	SendDataTCP(client, MessageTypes::S_FOCUSLOST, &data, sizeof(data));
}

void RemoteFocusGained(Client * client)
{
	wxUint8 data = 0; //have to send something
	SendDataTCP(client, MessageTypes::S_FOCUSGAINED, &data, sizeof(data));
}

//client message handlers
void ProcessEdgeOfScreen(Client * client, EdgeReached * edge)
{
	//if we are broadcasting the mouse do nothing
	if(g_broadcastMouse) return;
	//if the mouse is locked to the screen do nothing
	if(g_lockToScreen) return;
	//if we recently switched screens do nothing (to avoid infinitely switching screens)
	static wxLongLong lastSwitch = wxGetLocalTimeMillis();
	//if(wxGetLocalTimeMillis() - lastSwitch < 100) return;
	lastSwitch = wxGetLocalTimeMillis();
	
	//save the previous client so we can still talk to it after we switch
	int previousClient = g_activeClient;
	//find the previous client in our grid
	wxFlexGridSizer * grid = ((MainFrame*)wxTheApp->GetTopWindow())->m_layoutSizer;
	wxChoice * oldChoice = 0;
	wxChoice * newChoice = 0;
	wxIPV4address peer;
	bool dualHorizontal = false;
	bool dualVertical = false;
	g_clientList[g_activeClient]->m_socket->GetPeer(peer);
	size_t i;
	for(i = 0; i < 16; ++i)
	{
		//get an easy pointer to this choice
		oldChoice = (wxChoice*)grid->GetItem(i)->GetWindow();
		if(peer.Hostname() == oldChoice->GetStringSelection()) break;
	}
	//if we can't find the previous client something has gone terribly wrong
	if(i == 16)
	{
		if(!g_clientList.size()) g_activeClient = -1;
		g_activeClient = 0;
		return;
	}
	
	size_t rows = grid->GetRows();
	size_t columns = grid->GetCols();
	
	//check for dual-monitors on the source
	if(i % columns && i != 0 && //not at the left edge and not the first monitor
	  ((wxChoice*)grid->GetItem(i-1)->GetWindow())->GetStringSelection() == oldChoice->GetStringSelection()) //monitor to the left is the same
	{
		dualHorizontal = true;
	}
	else if((i+1) % columns && i != 15 && //not at the right edge and not the last monitor
	  ((wxChoice*)grid->GetItem(i+1)->GetWindow())->GetStringSelection() == oldChoice->GetStringSelection()) //monitor to the right is the same
	  
	{
		dualHorizontal = true;
	}
	else if(i >= columns && //not at the top
	  ((wxChoice*)grid->GetItem(i - columns)->GetWindow())->GetStringSelection() == oldChoice->GetStringSelection()) //monitor above is the same
	{
		dualVertical = true;
	}
	else if(i < columns * (rows - 1) && //not at the bottom
	  ((wxChoice*)grid->GetItem(i + columns)->GetWindow())->GetStringSelection() == oldChoice->GetStringSelection()) //monitor above is the same
	{
		dualVertical = true;
	}
	
	//find the client we want to move to
	size_t iNew = 0; //the index of the new client
	switch(edge->edge)
	{
		case EdgeReached::LEFT:
		{
			if(!(i % columns) || i == 0) return;
			iNew = i-1;
			break;
		}
		case EdgeReached::RIGHT:
		{
			if(dualHorizontal)
			{
				if(!((i+2) % columns) || i == 14) return;
				iNew = i + 2;
			}
			else
			{
				if(!((i+1) % columns) || i == 15) return;
				iNew = i + 1;
			}
			break;
		}
		case EdgeReached::TOP:
		{
			if((int)i < columns) return;
			if(dualHorizontal)
			{
				if(edge->x <= 32767) //left half
				{
					edge->x = edge->x * 2;
					iNew = i - columns;
				}
				else //right half
				{
					edge->x = (edge->x - 32767) * 2;
					iNew = i + 1 - columns;
				}
			}
			else
			{
				iNew = i - columns;
			}
			break;
		}
		case EdgeReached::BOTTOM:
		{
			if((int)i >= columns * (rows - 1)) return;
			if(dualHorizontal)
			{
				if(edge->x <= 32767) //left half
				{
					edge->x = edge->x * 2;
					iNew = i + columns;
				}
				else //right half
				{
					edge->x = (edge->x - 32767) * 2;
					iNew = i + 1 + columns;
				}
			}
			else
			{
				iNew = i + columns;
				break;
			}
		}
	}

	//we have a potential destination monitor, let's see if it's a dual-mointor
	if(iNew % columns && iNew != 0 && //not at the left edge and not the first monitor
	  ((wxChoice*)grid->GetItem(iNew-1)->GetWindow())->GetStringSelection() == ((wxChoice*)grid->GetItem(iNew)->GetWindow())->GetStringSelection()) //monitor to the left is the same
	{
		// If going from dual horizontal to dual horizontal we don't need to make any adjustments
		if (!dualHorizontal)
		{
			edge->x = (edge->x / 2) + 32767;
		}
	}
	else if((iNew + 1) % columns && iNew != 15 && //not at the right edge and not the last monitor
	  ((wxChoice*)grid->GetItem(iNew + 1)->GetWindow())->GetStringSelection() == ((wxChoice*)grid->GetItem(iNew)->GetWindow())->GetStringSelection()) //monitor to the right is the same
	  
	{
		// If going from dual horizontal to dual horizontal we don't need to make any adjustments
		if (!dualHorizontal)
		{
			edge->x = (edge->x / 2);
		}
	}
	else if(iNew >= columns && //not at the top
	  ((wxChoice*)grid->GetItem(iNew - columns)->GetWindow())->GetStringSelection() == ((wxChoice*)grid->GetItem(iNew)->GetWindow())->GetStringSelection()) //monitor above is the same
	{
		// If going from dual vertical to dual vertical we don't need to make any adjustments
		if (!dualVertical)
		{
			edge->y = (edge->y / 2) + 32767;
		}
	}
	else if(iNew < columns * (rows - 1) && //not at the bottom
	  ((wxChoice*)grid->GetItem(iNew + columns)->GetWindow())->GetStringSelection() == ((wxChoice*)grid->GetItem(iNew)->GetWindow())->GetStringSelection()) //monitor above is the same
	{
		// If going from dual vertical to dual vertical we don't need to make any adjustments
		if (!dualVertical)
		{
			edge->y = (edge->y / 2);
		}
	}
	
	//we have a destination monitor
	newChoice = (wxChoice*)grid->GetItem(iNew)->GetWindow();

	//are we going to change clients?
	if(!newChoice) return;
	if(newChoice->GetStringSelection().IsEmpty()) return;
	
	//find the Client for the desination
	for(i = 0; i < g_clientList.size(); ++i)
	{
		g_clientList[i]->m_socket->GetPeer(peer);
		if(peer.Hostname() == newChoice->GetStringSelection())
		{
			//yay! we finally have the destination client
			g_activeClient = i;
			break;
		}
	}
	//the choice box has a client selected that doesn't exist, that's OK but don't switch
	if(previousClient == g_activeClient) return;
	
	//tell the new active client that they are gaining focus
	RemoteFocusGained(g_clientList[g_activeClient]);
	//set the mouse position on the new active client to mirror the old mouse position
	if(g_setMousePosition)
	{
		//tell the old active client that they are losing focus, do this here so mouse isn't hidden when set position is off
		RemoteFocusLost(g_clientList[previousClient]);
		POINT point;
		switch(edge->edge)
		{
			case EdgeReached::LEFT:
				point.x = 65435;
				point.y = edge->y;
				break;
			case EdgeReached::RIGHT:
				point.x = 100;
				point.y = edge->y;
				break;
			case EdgeReached::TOP:
				point.x = edge->x;
				point.y = 65435;
				break;
			case EdgeReached::BOTTOM:
				point.x = edge->x;
				point.y = 100;
				break;
		}
		RemoteMousePosition(point, g_clientList[g_activeClient]);
	}
}

//callback for mouse and keyboard hooks
long __stdcall GlobalMouseHookCallback(int code, unsigned int type, long infoLong)
{
	//convert the mouse information into something more useful
	MSLLHOOKSTRUCT * info = (MSLLHOOKSTRUCT*)infoLong;
	
	//don't do anything if there are no connected clients
	if(g_activeClient == -1) return CallNextHookEx(g_mouseHook, code, type, (long)info);
	
	//if this is one of our events let the move fall through but remember where we are
	if(info->flags & LLMHF_INJECTED) return CallNextHookEx(g_mouseHook, code, type, (long)info);
	
	//x buttons use the mouseData field, nothing else does so default to 0 and only fill it in if necessary
	unsigned int clickData = 0;
	//handle different types of mouse input differently
	switch(type)
	{
		case WM_MOUSEMOVE:
		{
			if(g_broadcastMouse)
			{
				//remember where the cursor is on the server so we can send relative movement events
				GetCursorPos(&g_lastServerMousePos);
				//send the mouse move to every connected client
				for(size_t i = 0; i < g_clientList.size(); ++i)
				{
					RemoteMouseMove(info->pt, g_clientList[i]);
				}
				//don't pass the mouse move on to any other application, the local client will handle that
				return 1;
			}
			//remember where the cursor is so we can send relative movement events
			GetCursorPos(&g_lastServerMousePos);
			//send the mouse move to the active client
			RemoteMouseMove(info->pt, g_clientList[g_activeClient]);
			//don't pass the mouse move on to any other application
			return 1;
		}
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			clickData = info->mouseData >> 16;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		{
			if(g_broadcastMouse)
			{
				//send the mouse press/release to all connected clients
				for(size_t i = 0; i < g_clientList.size(); ++i)
				{
					RemoteMouseClick(type, clickData, g_clientList[i]);
				}
				//don't pass the mouse click on to any other application, the local client will handle that
				return 1;
			}
			//send the click to the active client
			RemoteMouseClick(type, clickData, g_clientList[g_activeClient]);
			//don't pass the mouse click on to any other applications on the server
			return 1;
		}
		case WM_MOUSEWHEEL:
		{
			if(g_broadcastMouse)
			{
				//send the mouse wheel to all connected clients
				for(size_t i = 0; i < g_clientList.size(); ++i)
				{
					RemoteMouseWheel((info->mouseData >> 16), g_clientList[i]);
				}
				//don't pass the mouse wheel on to any other applications, the local client will handle that
				return 1;
			}
			//send the mouse wheel action to the active client
			RemoteMouseWheel((info->mouseData >> 16), g_clientList[g_activeClient]);
			//don't pass the mouse wheel on to any other applications, the local client will handle that
			return 1;
		}
		default:
			wxLogVerbose(L"Unhandled event: %d!", type);
			return CallNextHookEx(g_mouseHook, code, type, (long)info);
	}
}

long __stdcall GlobalKeyboardHookCallback(int code, unsigned int type, long infoLong)
{
	//convert the keyboard information into something more useful
	KBDLLHOOKSTRUCT * info = (KBDLLHOOKSTRUCT*)infoLong;
	
	//check for the secret "something broke" hotkey (escape 8 times in a row)
	static int escapeCount = 0;
	if(info->vkCode == VK_ESCAPE)
	{
		++escapeCount;
		if(escapeCount >= 16)
		{
			//if(wxIsDebuggerRunning()) wxFAIL_MSG(L"Escape pressed 8 times.");
			wxGetApp().GetTopWindow()->Close();
		}
	}
	else escapeCount = 0;
	
	//handle hotkeys
	if(info->vkCode == g_broadcastKeyboardBinding && !info->flags)
	{
		g_broadcastKeyboard = !g_broadcastKeyboard;
		XRCCTRL(*(&wxGetApp())->GetTopWindow(), "m_checkBox_keyboardBroadcast", wxCheckBox)->SetValue(g_broadcastKeyboard);
	}
	else if(info->vkCode == g_broadcastMouseBinding && !info->flags)
	{
		g_broadcastMouse = !g_broadcastMouse;
		XRCCTRL(*(&wxGetApp())->GetTopWindow(), "m_checkBox_mouseBroadcast", wxCheckBox)->SetValue(g_broadcastMouse);
	}
	else if(info->vkCode == g_lockToScreenBinding && !info->flags)
	{
		g_lockToScreen = !g_lockToScreen;
		XRCCTRL(*(&wxGetApp())->GetTopWindow(), "m_checkBox_lockToScreen", wxCheckBox)->SetValue(g_lockToScreen);
	}
	
	//don't do anything if there are no connected clients
	if(g_activeClient == -1) return CallNextHookEx(g_keyboardHook, code, type, infoLong);
	
	//don't do anything if this is an injected event (so we don't double everything)
 	if(info->dwExtraInfo == EXTRA_INFO_NUMBER) return CallNextHookEx(g_keyboardHook, code, type, infoLong);
	
	//fix right shift bug
	if(info->scanCode == 0x36) info->flags &= ~LLKHF_EXTENDED;
	
	//handle different types of keyboard events differently
	switch(type)
	{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			if(g_broadcastKeyboard)
			{
				//send the mouse move to every connected client
				for(size_t i = 0; i < g_clientList.size(); ++i)
				{
					RemoteKeyInput(infoLong, g_clientList[i]);
				}
				return 1;
			}
			RemoteKeyInput(infoLong, g_clientList[g_activeClient]);
			return 1;
		}
		default:
		{
			//if we get this far we received an unhandled event so let the server deal with it
			wxLogVerbose(L"Unhandled keyboard event: Code: %d\tType: %d", code, type);
			return CallNextHookEx(g_keyboardHook, code, type, infoLong);
		}
	}
}

//listen socket event handler
void MultiBoxServerApp::ListenSocketCallback(wxSocketEvent & event)
{
	switch(event.GetSocketEvent())
	{
		case wxSOCKET_CONNECTION:
			AddClient(((wxSocketServer*)(event.GetSocket()))->Accept(false));
			break;
		default:
			wxLogVerbose(L"Unhandled socket event on Listen Socket.");
	}
}

//client socket event handler
void MultiBoxServerApp::ClientSocketCallback(wxSocketEvent & event)
{
	wxSocketBase * socket = event.GetSocket();
	Client * client = (Client*)socket->GetClientData();
	
	switch(event.GetSocketEvent())
	{
		case wxSOCKET_LOST:
		{
			RemoveClient(socket);
			break;
		}
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
					socket->Peek(&messageSize, 4);
					if(socket->LastCount() != 4) break;
					buffer = new wxUint8[messageSize];
					socket->Peek(buffer, messageSize - 8);
					if(socket->LastCount() != messageSize - 8) break;
					
					//read the message
					socket->Read(&messageSize, 4);
					socket->Read(&messageType, 4);
					socket->Read(buffer, messageSize - 8);
					
					//wxLogVerbose(L"Received: Type: %d   Size: %d", messageType, messageSize -8);
					
					switch(messageType)
					{
						case MessageTypes::C_EDGEREACHED:
						{
							ProcessEdgeOfScreen(client, (EdgeReached*)buffer);
							break;
						}
						case MessageTypes::C_CLIPBOARD:
						{
							//just forward it on to the active client
							if(g_moveClipboard && g_clientList.size() > 1)
							{
								SendDataTCP(g_clientList[g_activeClient], MessageTypes::S_CLIPBOARD, buffer, messageSize - 8);
							}
							break;
						}
						case MessageTypes::C_KEEPALIVE:
						{
							//send back the server keep-alive response
							wxUint32 temp = 0;
							SendDataTCP((Client*)socket->GetClientData(), MessageTypes::S_KEEPALIVE, &temp, 4);
							break;
						}
						default:
						{
							wxLogError(L"Unknown message type (%d) received from client!", messageType);
						}
					}
					delete buffer;
				}
			}
			catch(int)
			{
				wxLogError(L"Not enough bytes!");
				socket->Close();
				return;
			}
			break;
		}
		default:
		{
			wxIPV4address address;
			socket->GetPeer(address);
			wxLogError(L"Unhandled socket event (%d) from client %s!", event.GetSocketEvent(), address.IPAddress());
			break;
		}
	}
}

void MultiBoxServerApp::UDPSocketCallback(wxSocketEvent & event)
{
	switch(event.GetSocketEvent())
	{
		case wxSOCKET_INPUT:
		{
			try
			{
				wxUint8 buffer[1024];
				wxUint32 messageType = 0;
				wxIPV4address address;
				
				//figure out which client this was
				
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
						case MessageTypes::C_EDGEREACHED:
						{
							ProcessEdgeOfScreen(g_clientList[g_activeClient], (EdgeReached*)(buffer + sizeof(messageType)));
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

void MultiBoxServerApp::SetupConnections()
{
	//kill any active connections (in the case of a global reconnect)
	if(g_datagramSocket)
	{
		g_datagramSocket->Destroy();
		g_datagramSocket = 0;
	}
	if(g_listenSocket)
	{
		g_listenSocket->Destroy();
		g_listenSocket = 0;
	}
	g_activeClient = -1;
	for(size_t i = 0; i < g_clientList.size(); ++i)
	{
		delete g_clientList[i];
		g_clientList.erase(g_clientList.begin() + i);
	}
	
	//create the datagram socket
	wxIPV4address UDPaddress;
	UDPaddress.AnyAddress();
	UDPaddress.Service(g_basePort);
	g_datagramSocket = new wxDatagramSocket(UDPaddress);
	if(!g_datagramSocket->Ok())
	{
		wxLogError(L"Unable to initialize outbound UDP socket on port %d!", g_basePort);
		return;
	}
	g_datagramSocket->SetEventHandler(*this, wxID_UDP_SOCKET);
	g_datagramSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG);
	g_datagramSocket->Notify(true);
	
	//create the TCP listen server
	wxIPV4address address;
	address.Service(g_basePort+1);
	g_listenSocket = new wxSocketServer(address, wxSOCKET_NOWAIT);
	if(!g_listenSocket->Ok())
	{
		wxLogError(L"Unable to listen on port %d!", g_basePort+1);
		return;
	}
	g_listenSocket->SetEventHandler(*this, wxID_LISTEN_SOCKET);
	g_listenSocket->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_OUTPUT_FLAG | wxSOCKET_CONNECTION_FLAG | wxSOCKET_LOST_FLAG);
	g_listenSocket->Notify(true);
	wxLogVerbose(L"Listening on port %d.", g_basePort+1);
}

//entry point
bool MultiBoxServerApp::OnInit()
{
	//_CrtSetBreakAlloc(7301);
	wxXmlResource::Get()->InitAllHandlers();
	wxXmlResource::Get()->Load(L"MultiBoxServer.xrc");
	
	//create the main frame
	MainFrame* frame = new MainFrame();
	SetTopWindow(frame);
	SetExitOnFrameDelete(true);
	
	//setup network sockets
	SetupConnections();
	
	//setup our mouse/keyboard hooks
	g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, &GlobalMouseHookCallback, GetModuleHandle(NULL), NULL);
	if(!g_mouseHook)
	{
		LPVOID errorBuffer;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorBuffer, 0, NULL);
		wxLogError(wxT("Could not set global mouse hook: %s!"), errorBuffer);
		return true;
	}
	wxLogVerbose(L"Mouse hook installed.");
	g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, &GlobalKeyboardHookCallback, GetModuleHandle(NULL), NULL);
	if(!g_keyboardHook)
	{
		LPVOID errorBuffer;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&errorBuffer, 0, NULL);
		wxLogError(L"Could not set global keyboard hook: %s!", errorBuffer);
		return true;
	}
	wxLogVerbose(L"Keyboard hook installed.");
	
	//set the process priority to high
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	
	return true;
}

//exit point
int MultiBoxServerApp::OnExit()
{
	//cleanup
	UnhookWindowsHookEx(g_keyboardHook);
	UnhookWindowsHookEx(g_mouseHook);
	if(g_listenSocket) g_listenSocket->Destroy();
	if(g_datagramSocket) g_datagramSocket->Destroy();
	//destroy all the clients
	for(unsigned int i = 0; i < g_clientList.size(); ++i)
	{
		delete g_clientList[i];
	}
	return wxApp::OnExit();
}

//helper functions
void MultiBoxServerApp::RebuildLayoutChoices()
{
	//get a pointer to the main frame and the layout grid
	MainFrame * frame = (MainFrame*)GetTopWindow();
	wxFlexGridSizer * grid = frame->m_layoutSizer;
	
	//loop through each client and add it to a list of available strings
	wxArrayString strings;
	strings.Add(wxEmptyString);
	wxIPV4address peer;
	for(size_t i = 0; i < g_clientList.size(); ++i)
	{
		g_clientList[i]->m_socket->GetPeer(peer);
		strings.Add(peer.Hostname());
	}
	
	//loop through each wxChoice and drop this array in
	wxChoice * choice;
	for(size_t i = 0; i < grid->GetChildren().GetCount(); ++i)
	{
		choice = (wxChoice*)grid->GetItem(i)->GetWindow();
		wxString currentSelection = choice->GetStringSelection();
		choice->Clear();
		choice->Append(strings);
		if(!choice->SetStringSelection(currentSelection))
		{
			//the string wasn't found, let's add it to this one wxChoice
			choice->Append(currentSelection);
			choice->SetStringSelection(currentSelection);
		}
	}
}

void MultiBoxServerApp::RemoveClient(wxSocketBase * socket)
{
	Client * client = (Client*)socket->GetClientData();
	//get the address of the peer
	wxIPV4address address;
	socket->GetPeer(address);
	//loop through our client list to find this client so we can delete it
	for(size_t i = 0; i < g_clientList.size(); ++i)
	{
		if(g_clientList[i] == client)
		{
			//delete the client
			delete client;
			g_clientList.erase(g_clientList.begin() + i);
			//if the active client disconnected we need to do something about it
			if(i == g_activeClient)
			{
				//if this was the last client then we don't have an active client
				if(g_clientList.size() == 0)
				{
					g_activeClient = -1;
				}
				//we'll just set it to the first client connected for now
				else
				{
					g_activeClient = 0;
					RemoteFocusGained(g_clientList[g_activeClient]);
				}
			}
			RebuildLayoutChoices();
			wxLogVerbose(L"Client lost: %s (%s).", address.Hostname(), address.IPAddress());
			return;
		}
	}
	wxLogError(L"Could not find client in list to destroy it (IP: %s)!", address.IPAddress());
}

void MultiBoxServerApp::AddClient(wxSocketBase * server)
{
	//setup the event handler for this socket
	server->SetFlags(wxSOCKET_NOWAIT|wxSOCKET_BLOCK);
	server->SetEventHandler(*this, wxID_CLIENT_SOCKET);
	server->SetNotify(wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG);
	server->Notify(true);
	//try and guess if this client is localhost
	wxIPV4address local;
	wxIPV4address peer;
	server->GetLocal(local);
	server->GetPeer(peer);
	bool isLocal = false;
	if(local.IPAddress() == peer.IPAddress() || local.Hostname() == peer.Hostname() || peer.IsLocalHost()) isLocal = true;
	//create a new Client
	Client * client = new Client(server, isLocal);
	//leave a pointer to the client in the socket so we can find it when we get a socket event
	server->SetClientData(client);
	//add the client to our list of clients
	g_clientList.push_back(client);
	//if this is the only connected client then set it to active and take note of the server mouse position
	if(g_clientList.size() == 1)
	{
		g_activeClient = 0;
		GetCursorPos(&g_lastServerMousePos);
	}
	//rebuild our choice lists
	RebuildLayoutChoices();
	
	wxLogVerbose(L"Client connected: %s (%s).", peer.Hostname(), peer.IPAddress());
}
