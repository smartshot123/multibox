#include <wx/taskbar.h>
#include <wx/menu.h>
#include <wx/app.h>
#include <wx/window.h>

struct TaskBarIcon : wxTaskBarIcon
{
	wxWindow * m_parentWindow;
	
	TaskBarIcon(wxWindow * parent);
	
	virtual wxMenu * CreatePopupMenu();
	void OnMenuQuit(wxCommandEvent & event);
	void OnLeftClick(wxTaskBarIconEvent & event);
	
	DECLARE_EVENT_TABLE()
};
