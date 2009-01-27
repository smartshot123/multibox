#include "MultiBoxServer.h"
#include "KeyBindingDialog.h"
#include <vector>
#include <wx/config.h>
#include <wx/xrc/xmlres.h>

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
	EVT_CHOICE(XRCID("m_choice0"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice1"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice2"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice3"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice4"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice5"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice6"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice7"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice8"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice9"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice10"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice11"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice12"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice13"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice14"), MainFrame::OnLayoutChoice)
	EVT_CHOICE(XRCID("m_choice15"), MainFrame::OnLayoutChoice)
	EVT_BUTTON(XRCID("m_button_keyboardBinding"), MainFrame::OnKeyboardBinding)
	EVT_BUTTON(XRCID("m_button_mouseBinding"), MainFrame::OnMouseBinding)
	EVT_BUTTON(XRCID("m_button_lockToScreenBinding"), MainFrame::OnLockToScreenBinding)
	EVT_SPINCTRL(XRCID("m_spinCtrl_rows"), MainFrame::OnLayoutDimensionChange)
	EVT_SPINCTRL(XRCID("m_spinCtrl_columns"), MainFrame::OnLayoutDimensionChange)
	EVT_SPINCTRL(XRCID("m_spinCtrl_portRange"), MainFrame::OnPortChange)
	EVT_CHECKBOX(XRCID("m_checkBox_keyboardBroadcast"), MainFrame::OnKeyboardBroadcastToggle)
	EVT_CHECKBOX(XRCID("m_checkBox_mouseBroadcast"), MainFrame::OnMouseBroadcastToggle)
	EVT_CHECKBOX(XRCID("m_checkBox_lockToScreen"), MainFrame::OnLockToScreenToggle)
	EVT_CHECKBOX(XRCID("m_checkBox_adjustMouse"), MainFrame::OnAdjustMousePositionToggle)
	EVT_CHECKBOX(XRCID("m_checkBox_moveClipboard"), MainFrame::OnMoveClipboardToggle)
END_EVENT_TABLE()

DECLARE_APP(MultiBoxServerApp)

MainFrame::MainFrame() : wxFrame()
{
	//setup the taskbar icon/menu
	m_taskBarIcon = new TaskBarIcon(this);
	wxIcon icon(icon_xpm);
	m_taskBarIcon->SetIcon(icon, L"MultiBoxServer");
	
	//load the frame from XRC
	if(!wxXmlResource::Get()->LoadFrame(this, NULL, L"MainFrame"))
	{
		wxFAIL_MSG(L"Unable to load XRC file.");
	}
	//setup logging
	logOutput_textCtrl = new wxLogTextCtrl(XRCCTRL(*this, "m_textCtrl_log", wxTextCtrl));
	delete wxLog::SetActiveTarget(logOutput_textCtrl);
	wxLog::SetVerbose();
	wxLogVerbose(L"Logging enabled.");
	//get saved config data
	long basePort;
	wxConfig::Get()->Read(L"Base Port", &basePort, 9435);
	g_basePort = (wxUint32)(basePort & 0xFFFFFFFF);
	XRCCTRL(*this, "m_spinCtrl_portRange", wxSpinCtrl)->SetValue(g_basePort);
	XRCCTRL(*this, "m_staticText_portRange", wxStaticText)->SetLabel(wxString::Format(L"%d", g_basePort+2));
	//broadcast keyboard
	long broadcastKeyboard;
	wxConfig::Get()->Read(L"Broadcast Keyboard", &broadcastKeyboard, 0);
	g_broadcastKeyboard = (broadcastKeyboard == 0) ? false : true;
	XRCCTRL(*this, "m_checkBox_keyboardBroadcast", wxCheckBox)->SetValue(g_broadcastKeyboard);
	//broadcast mouse
	long broadcastMouse;
	wxConfig::Get()->Read(L"Broadcast Mouse", &broadcastMouse, 0);
	g_broadcastMouse = (broadcastMouse == 0) ? false : true;
	XRCCTRL(*this, "m_checkBox_mouseBroadcast", wxCheckBox)->SetValue(g_broadcastMouse);
	//lock to screen
	long lockToScreen;
	wxConfig::Get()->Read(L"Lock to Screen", &lockToScreen, 0);
	g_lockToScreen = (lockToScreen == 0) ? false : true;
	XRCCTRL(*this, "m_checkBox_lockToScreen", wxCheckBox)->SetValue(g_lockToScreen);
	//broadcast keyboard binding
	wxConfig::Get()->Read(L"Broadcast Keyboard Binding", (long*)&g_broadcastKeyboardBinding, 0);
	//broadcast mouse binding
	wxConfig::Get()->Read(L"Broadcast Mouse Binding", (long*)&g_broadcastMouseBinding, 0);
	//lock to screen binding
	wxConfig::Get()->Read(L"Lock To Screen Binding", (long*)&g_lockToScreenBinding, 0);
	//set mouse position
	long setMousePosition;
	wxConfig::Get()->Read(L"Set Mouse Position", &setMousePosition, 1);
	g_setMousePosition = (setMousePosition == 0) ? false : true;
	XRCCTRL(*this, "m_checkBox_adjustMouse", wxCheckBox)->SetValue(g_setMousePosition);
	//move clipboard
	long moveClipboard;
	wxConfig::Get()->Read(L"Move Clipboard", &moveClipboard, 1);
	g_moveClipboard = (moveClipboard == 0) ? false : true;
	XRCCTRL(*this, "m_checkBox_moveClipboard", wxCheckBox)->SetValue(g_moveClipboard);
	GetSizer()->SetSizeHints(this);
	//client layout
	wxChoice * choice;
	//loop through all 16 choices and load any saved variables for them
	for(int i = 0; i < 16; ++i)
	{
		//get the name of the choice box we are looking at
		wxString name = wxString::Format(wxT("m_choice%d"), i);
		//get a pointer to the choice box
		choice = wxStaticCast(FindWindow(wxXmlResource::GetXRCID(name)), wxChoice);
		//clear the choice box and add an empty string to the list of options
		choice->Clear();
		choice->Append(wxEmptyString);
		//load the previous selection from file
		wxString previousSelection;
		if(wxConfig::Get()->Read(name, &previousSelection, wxEmptyString))
		{
			//if there is a previous selction, add and select it
			choice->SetSelection(choice->Append(previousSelection));
		}
	}
	long clientRows;
	long clientColumns;
	wxConfig::Get()->Read(L"Layout Rows", &clientRows, 2);
	wxConfig::Get()->Read(L"Layout Columns", &clientColumns, 3);
	XRCCTRL(*this, "m_spinCtrl_rows", wxSpinCtrl)->SetValue(clientRows);
	XRCCTRL(*this, "m_spinCtrl_columns", wxSpinCtrl)->SetValue(clientColumns);
	wxSizer * sizer = XRCCTRL(*this, "m_panel_top", wxPanel)->GetSizer()->GetItem(1)->GetSizer();
	wxASSERT_MSG(sizer, wxT("Not a sizer!"));
	wxASSERT_MSG(sizer->IsKindOf(CLASSINFO(wxGridSizer)), wxT("Not a grid!"));
	m_layoutSizer = (wxFlexGridSizer*)sizer;
	LayoutGrid();

	Fit();
}

MainFrame::~MainFrame()
{
	m_taskBarIcon->RemoveIcon();
	delete m_taskBarIcon;
}

void MainFrame::OnKeyboardBinding(wxCommandEvent & event)
{
	//create a keybinding dialog
	KeyBindingDialog keyBinding(this);
	//set the keybinding dialog text box to display the current binding
	if(g_broadcastKeyboardBinding)
	{
		keyBinding.m_textCtrl->SetValue(keyBinding.m_textCtrl->GetStringFromCode(g_broadcastKeyboardBinding));
		keyBinding.m_textCtrl->m_rawKeyCode = g_broadcastKeyboardBinding;
	}
	//display the dialog to the user
	keyBinding.ShowModal();
	//retrieve the key they chose
	g_broadcastKeyboardBinding = keyBinding.m_textCtrl->m_rawKeyCode;
	//save the key to disk
	wxConfig::Get()->Write(L"Broadcast Keyboard Binding", (long)g_broadcastKeyboardBinding);
}

void MainFrame::OnLockToScreenBinding(wxCommandEvent & event)
{
	KeyBindingDialog keyBinding(this);
	if(g_lockToScreenBinding)
	{
		keyBinding.m_textCtrl->SetValue(keyBinding.m_textCtrl->GetStringFromCode(g_lockToScreenBinding));
		keyBinding.m_textCtrl->m_rawKeyCode = g_lockToScreenBinding;
	}
	keyBinding.ShowModal();
	g_lockToScreenBinding = keyBinding.m_textCtrl->m_rawKeyCode;
	wxConfig::Get()->Write(L"Lock To Screen Binding", (long)g_lockToScreenBinding);
}

void MainFrame::OnMouseBinding(wxCommandEvent & event)
{
	//create a keybinding dialog
	KeyBindingDialog keyBinding(this);
	//set the keybinding dialog text box to display the current binding
	if(g_broadcastMouseBinding)
	{
		keyBinding.m_textCtrl->SetValue(keyBinding.m_textCtrl->GetStringFromCode(g_broadcastMouseBinding));
		keyBinding.m_textCtrl->m_rawKeyCode = g_broadcastMouseBinding;
	}
	//display the dialog to the user
	keyBinding.ShowModal();
	//retrieve the key they chose
	g_broadcastMouseBinding = keyBinding.m_textCtrl->m_rawKeyCode;
	//save the key to disk
	wxConfig::Get()->Write(L"Broadcast Mouse Binding", (long)g_broadcastMouseBinding);
}

void MainFrame::OnLayoutChoice(wxCommandEvent & event)
{
	wxChoice * choice = (wxChoice*)event.GetEventObject();
	wxString name = choice->GetName();
	
	//save the selection to disk
	wxConfig::Get()->Write(name, choice->GetStringSelection());
	
	//if(!choice->GetStringSelection().IsEmpty())
	//{
	//	//loop through all the other choices and remove duplicates
	//	wxChoice * tempChoice;
	//	for(size_t i = 0; i < 16; ++i)
	//	{
	//		tempChoice = (wxChoice*)m_layoutSizer->GetItem(i)->GetWindow();
	//		if(tempChoice->GetStringSelection() == choice->GetStringSelection() && tempChoice != choice)
	//		{
	//			tempChoice->SetStringSelection(wxEmptyString);
	//			wxConfig::Get()->Write(tempChoice->GetName(), wxEmptyString);
	//		}
	//	}
	//}
}

void MainFrame::LayoutGrid()
{
	//get the new value
	long clientRows = XRCCTRL(*this, "m_spinCtrl_rows", wxSpinCtrl)->GetValue();
	long clientColumns = XRCCTRL(*this, "m_spinCtrl_columns", wxSpinCtrl)->GetValue();
	
	//set the number of rows/columns in the grid
	m_layoutSizer->SetRows(clientRows);
	m_layoutSizer->SetCols(clientColumns);
	//hide all but rows*columns worth
	for(int i = 0; i < 16; ++i)
	{
		if(i < clientRows*clientColumns)
		{
			m_layoutSizer->Show((size_t)i);
		}
		else
		{
			m_layoutSizer->Hide(i);
		}
	}
	m_layoutSizer->Layout();
	Fit();
}

void MainFrame::OnLayoutDimensionChange(wxSpinEvent & event)
{
	//save the new value
	if(event.GetId() == XRCID("m_spinCtrl_rows")) wxConfig::Get()->Write(L"Layout Rows", (long)event.GetPosition());
	if(event.GetId() == XRCID("m_spinCtrl_columns")) wxConfig::Get()->Write(L"Layout Columns", (long)event.GetPosition());
	LayoutGrid();
}

void MainFrame::OnPortChange(wxSpinEvent & event)
{
	g_basePort = event.GetPosition();
	wxConfig::Get()->Write(L"Base Port", (long)g_basePort);
	XRCCTRL(*this, "m_staticText_portRange", wxStaticText)->SetLabel(wxString::Format(L"%d", g_basePort+2));
	wxGetApp().SetupConnections();
}

void MainFrame::OnKeyboardBroadcastToggle(wxCommandEvent & event)
{
	g_broadcastKeyboard = event.IsChecked();
	wxConfig::Get()->Write(L"Broadcast Keyboard", (g_broadcastKeyboard ? 1 : 0));
}

void MainFrame::OnMouseBroadcastToggle(wxCommandEvent & event)
{
	g_broadcastMouse = event.IsChecked();
	wxConfig::Get()->Write(L"Broadcast Mouse", (g_broadcastMouse ? 1 : 0));
}

void MainFrame::OnLockToScreenToggle(wxCommandEvent & event)
{
	g_lockToScreen = event.IsChecked();
	wxConfig::Get()->Write(L"Broadcast Mouse", (g_lockToScreen ? 1 : 0));
}

void MainFrame::OnAdjustMousePositionToggle(wxCommandEvent & event)
{
	g_setMousePosition = event.IsChecked();
	wxConfig::Get()->Write(L"Set Mouse Position", (g_setMousePosition ? 1 : 0));
}

void MainFrame::OnMoveClipboardToggle(wxCommandEvent & event)
{
	g_moveClipboard = event.IsChecked();
	wxConfig::Get()->Write(L"Move Clipboard", (g_moveClipboard ? 1 : 0));
}
