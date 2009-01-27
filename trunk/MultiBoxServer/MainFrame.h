#pragma once
#include <wx/xrc/xmlres.h>
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include "icon.xpm"
#include "../TaskBarIcon.h"

class MainFrame : public wxFrame
{
public:
	MainFrame();
	~MainFrame();
	
	TaskBarIcon * m_taskBarIcon;
	wxFlexGridSizer * m_layoutSizer;
	
	inline void LayoutGrid();
	void OnKeyboardBinding(wxCommandEvent & event);
	void OnMouseBinding(wxCommandEvent & event);
	void OnLockToScreenBinding(wxCommandEvent & event);
	void OnLayoutChoice(wxCommandEvent & event);
	void OnLayoutDimensionChange(wxSpinEvent & event);
	void OnPortChange(wxSpinEvent & event);
	void OnKeyboardBroadcastToggle(wxCommandEvent & event);
	void OnMouseBroadcastToggle(wxCommandEvent & event);
	void OnLockToScreenToggle(wxCommandEvent & event);
	void OnAdjustMousePositionToggle(wxCommandEvent & event);
	void OnMoveClipboardToggle(wxCommandEvent & event);

private:
	wxLogTextCtrl* logOutput_textCtrl;
	DECLARE_EVENT_TABLE()
};
