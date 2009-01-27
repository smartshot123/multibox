#pragma once
#include <wx/spinctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/wx.h>
#include "icon.xpm"
#include "../TaskBarIcon.h"

class MainFrame : public wxFrame
{
public:
	MainFrame();
	~MainFrame();
	
	TaskBarIcon * m_taskBarIcon;
	
	void OnReconnectTimeoutChange(wxSpinEvent & event);
	void OnPortChange(wxSpinEvent & event);
	void OnServerNameChanged(wxCommandEvent & event);
	void OnMouseSpeedChanged(wxScrollEvent & event);

private:
	wxLogTextCtrl* logOutput_textCtrl;
	DECLARE_EVENT_TABLE()
};
