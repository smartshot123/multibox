#include "TaskBarIcon.h"

enum
{
	ID_TASKBAR_QUIT = wxID_HIGHEST,
};

BEGIN_EVENT_TABLE(TaskBarIcon, wxTaskBarIcon)
    EVT_MENU(ID_TASKBAR_QUIT, TaskBarIcon::OnMenuQuit)
    EVT_TASKBAR_LEFT_UP(TaskBarIcon::OnLeftClick)
END_EVENT_TABLE()

TaskBarIcon::TaskBarIcon(wxWindow * parent)
{
	m_parentWindow = parent;
}

wxMenu * TaskBarIcon::CreatePopupMenu()
{
	wxMenu *menu = new wxMenu();
	menu->Append(ID_TASKBAR_QUIT, L"Quit");
	return menu;
}

void TaskBarIcon::OnMenuQuit(wxCommandEvent & event)
{
	m_parentWindow->Close();
}

void TaskBarIcon::OnLeftClick(wxTaskBarIconEvent & event)
{
	if(m_parentWindow->IsShown()) m_parentWindow->Hide();
	else m_parentWindow->Show();
}
