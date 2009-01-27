#include "MultiBoxClient.h"
#include <wx/config.h>
#include <wx/xrc/xmlres.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_SPINCTRL(XRCID("m_spinCtrl_reconnectTimeout"), MainFrame::OnReconnectTimeoutChange)
	EVT_SPINCTRL(XRCID("m_spinCtrl_portRange"), MainFrame::OnPortChange)
	EVT_TEXT_ENTER(XRCID("m_textCtrl_serverName"), MainFrame::OnServerNameChanged)
	EVT_COMMAND_SCROLL(XRCID("m_slider_mouseSpeed"), MainFrame::OnMouseSpeedChanged)
END_EVENT_TABLE()

MainFrame::MainFrame() : wxFrame()
{
	//setup the taskbar icon/menu
	m_taskBarIcon = new TaskBarIcon(this);
	wxIcon icon(icon_xpm);
	m_taskBarIcon->SetIcon(icon, L"MultiBoxClient");
	
	//load the frame from XRC
	if(!wxXmlResource::Get()->LoadFrame(this, NULL, L"MainFrame")) wxFAIL_MSG(wxT("Unable to load XRC file."));
	//wxXmlResource::Get()->LoadFrame(this, NULL, L"MainFrame");
	GetSizer()->Fit(this);
	GetSizer()->SetSizeHints(this);
	//setup logging
	logOutput_textCtrl = new wxLogTextCtrl(XRCCTRL(*this, "m_textCtrl_log", wxTextCtrl));
	delete wxLog::SetActiveTarget(logOutput_textCtrl);
	wxLog::SetVerbose();
	wxLogVerbose(L"Logging enabled.");
	
	//get saved config data
	wxConfig::Get()->Read(L"Server Name", &g_serverName, L"");
	XRCCTRL(*this, "m_textCtrl_serverName", wxTextCtrl)->SetValue(g_serverName);
	//reconnect timeout
	wxConfig::Get()->Read(L"Reconnect Timeout", &g_reconnectTimeout, 30);
	XRCCTRL(*this, "m_spinCtrl_reconnectTimeout", wxSpinCtrl)->SetValue(g_reconnectTimeout);
	//base port
	long basePort;
	wxConfig::Get()->Read(L"Base Port", &basePort, 9435);
	g_basePort = basePort & 0xFFFFFFFF;
	XRCCTRL(*this, "m_spinCtrl_portRange", wxSpinCtrl)->SetValue(g_basePort);
	XRCCTRL(*this, "m_staticText_portRange", wxStaticText)->SetLabel(wxString::Format(L"%d", g_basePort+2));
	//mouse speed
	long mouseSpeed;
	wxConfig::Get()->Read(L"Mouse Speed", &mouseSpeed, 10);
	XRCCTRL(*this, "m_slider_mouseSpeed", wxSlider)->SetValue(mouseSpeed);
	g_mouseSpeed = (wxUint8)mouseSpeed;
}

MainFrame::~MainFrame()
{
	m_taskBarIcon->RemoveIcon();
	delete m_taskBarIcon;
}

void MainFrame::OnReconnectTimeoutChange(wxSpinEvent & event)
{
	g_reconnectTimeout = event.GetPosition();
	wxConfig::Get()->Write(L"Reconnect Timeout", (long)g_reconnectTimeout);
}

void MainFrame::OnPortChange(wxSpinEvent & event)
{
	g_basePort = event.GetPosition();
	wxConfig::Get()->Write(L"Base Port", (long)g_basePort);
	XRCCTRL(*this, "m_staticText_portRange", wxStaticText)->SetLabel(wxString::Format(L"%d", g_basePort+2));
}

void MainFrame::OnServerNameChanged(wxCommandEvent & WXUNUSED(event))
{
	g_serverName = XRCCTRL(*this, "m_textCtrl_serverName", wxTextCtrl)->GetValue();
	wxConfig::Get()->Write(L"Server Name", g_serverName);
	//reconnect using the new server name
	if(g_clientSocket) g_clientSocket->Destroy();
	if(g_datagramSocket) g_datagramSocket->Destroy();
	g_clientSocket = 0;
	g_datagramSocket = 0;
	if(!g_connectionTimer.Start(1, true)) wxLogError(L"Could not start the connection timer!");
}

void MainFrame::OnMouseSpeedChanged(wxScrollEvent & WXUNUSED(event))
{
	wxUint32 mouseSpeed = XRCCTRL(*this, "m_slider_mouseSpeed", wxSlider)->GetValue();
	wxConfig::Get()->Write(L"Mouse Speed", (long)mouseSpeed);
	g_mouseSpeed = (wxUint8)mouseSpeed;
}
