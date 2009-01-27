#include "KeyBindingDialog.h"
#include <wx/sizer.h>

BEGIN_EVENT_TABLE(KeyBindingTextCtrl, wxTextCtrl)
	EVT_KEY_DOWN(KeyBindingTextCtrl::OnKeyDown)
END_EVENT_TABLE()

KeyBindingDialog::KeyBindingDialog(wxWindow * parent) : wxDialog(parent, (wxWindowID)wxID_ANY, wxString(L"Key Binding"))
{
	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	m_textCtrl = new KeyBindingTextCtrl(this);
	sizer->Add(m_textCtrl, 0, wxEXPAND | wxALL | wxALIGN_CENTER, 5);
	sizer->Add(CreateStdDialogButtonSizer(wxOK), 0, wxALL | wxALIGN_RIGHT, 5);
	SetSizerAndFit(sizer);
}

wxString KeyBindingTextCtrl::GetStringFromCode(wxUint32 keycode)
{
	wxString key;
	switch(keycode)
	{
		//case WXK_BACK: key = _T("BACK"); break;
		//case WXK_TAB: key = _T("TAB"); break;
		//case WXK_RETURN: key = _T("RETURN"); break;
		//case WXK_ESCAPE: key = _T("ESCAPE"); break;
		//case WXK_SPACE: key = _T("SPACE"); break;
		//case WXK_DELETE: key = _T("DELETE"); break;
		//case WXK_START: key = _T("START"); break;
		//case WXK_LBUTTON: key = _T("LBUTTON"); break;
		//case WXK_RBUTTON: key = _T("RBUTTON"); break;
		//case WXK_CANCEL: key = _T("CANCEL"); break;
		//case WXK_MBUTTON: key = _T("MBUTTON"); break;
		//case WXK_CLEAR: key = _T("CLEAR"); break;
		//case WXK_SHIFT: key = _T("SHIFT"); break;
		//case WXK_ALT: key = _T("ALT"); break;
		//case WXK_CONTROL: key = _T("CONTROL"); break;
		//case WXK_MENU: key = _T("MENU"); break;
		//case WXK_PAUSE: key = _T("PAUSE"); break;
		//case WXK_CAPITAL: key = _T("CAPITAL"); break;
		//case WXK_END: key = _T("END"); break;
		//case WXK_HOME: key = _T("HOME"); break;
		//case WXK_LEFT: key = _T("LEFT"); break;
		//case WXK_UP: key = _T("UP"); break;
		//case WXK_RIGHT: key = _T("RIGHT"); break;
		//case WXK_DOWN: key = _T("DOWN"); break;
		//case WXK_SELECT: key = _T("SELECT"); break;
		//case WXK_PRINT: key = _T("PRINT"); break;
		//case WXK_EXECUTE: key = _T("EXECUTE"); break;
		//case WXK_SNAPSHOT: key = _T("SNAPSHOT"); break;
		//case WXK_INSERT: key = _T("INSERT"); break;
		//case WXK_HELP: key = _T("HELP"); break;
		//case WXK_NUMPAD0: key = _T("NUMPAD0"); break;
		//case WXK_NUMPAD1: key = _T("NUMPAD1"); break;
		//case WXK_NUMPAD2: key = _T("NUMPAD2"); break;
		//case WXK_NUMPAD3: key = _T("NUMPAD3"); break;
		//case WXK_NUMPAD4: key = _T("NUMPAD4"); break;
		//case WXK_NUMPAD5: key = _T("NUMPAD5"); break;
		//case WXK_NUMPAD6: key = _T("NUMPAD6"); break;
		//case WXK_NUMPAD7: key = _T("NUMPAD7"); break;
		//case WXK_NUMPAD8: key = _T("NUMPAD8"); break;
		//case WXK_NUMPAD9: key = _T("NUMPAD9"); break;
		//case WXK_MULTIPLY: key = _T("MULTIPLY"); break;
		//case WXK_ADD: key = _T("ADD"); break;
		//case WXK_SEPARATOR: key = _T("SEPARATOR"); break;
		//case WXK_SUBTRACT: key = _T("SUBTRACT"); break;
		//case WXK_DECIMAL: key = _T("DECIMAL"); break;
		//case WXK_DIVIDE: key = _T("DIVIDE"); break;
		//case WXK_F1: key = _T("F1"); break;
		//case WXK_F2: key = _T("F2"); break;
		//case WXK_F3: key = _T("F3"); break;
		//case WXK_F4: key = _T("F4"); break;
		//case WXK_F5: key = _T("F5"); break;
		//case WXK_F6: key = _T("F6"); break;
		//case WXK_F7: key = _T("F7"); break;
		//case WXK_F8: key = _T("F8"); break;
		//case WXK_F9: key = _T("F9"); break;
		//case WXK_F10: key = _T("F10"); break;
		//case WXK_F11: key = _T("F11"); break;
		//case WXK_F12: key = _T("F12"); break;
		//case WXK_F13: key = _T("F13"); break;
		//case WXK_F14: key = _T("F14"); break;
		//case WXK_F15: key = _T("F15"); break;
		//case WXK_F16: key = _T("F16"); break;
		//case WXK_F17: key = _T("F17"); break;
		//case WXK_F18: key = _T("F18"); break;
		//case WXK_F19: key = _T("F19"); break;
		//case WXK_F20: key = _T("F20"); break;
		//case WXK_F21: key = _T("F21"); break;
		//case WXK_F22: key = _T("F22"); break;
		//case WXK_F23: key = _T("F23"); break;
		//case WXK_F24: key = _T("F24"); break;
		//case WXK_NUMLOCK: key = _T("NUMLOCK"); break;
		//case WXK_SCROLL: key = _T("SCROLL"); break;
		//case WXK_PAGEUP: key = _T("PAGEUP"); break;
		//case WXK_PAGEDOWN: key = _T("PAGEDOWN"); break;
		//case WXK_NUMPAD_SPACE: key = _T("NUMPAD_SPACE"); break;
		//case WXK_NUMPAD_TAB: key = _T("NUMPAD_TAB"); break;
		//case WXK_NUMPAD_ENTER: key = _T("NUMPAD_ENTER"); break;
		//case WXK_NUMPAD_F1: key = _T("NUMPAD_F1"); break;
		//case WXK_NUMPAD_F2: key = _T("NUMPAD_F2"); break;
		//case WXK_NUMPAD_F3: key = _T("NUMPAD_F3"); break;
		//case WXK_NUMPAD_F4: key = _T("NUMPAD_F4"); break;
		//case WXK_NUMPAD_HOME: key = _T("NUMPAD_HOME"); break;
		//case WXK_NUMPAD_LEFT: key = _T("NUMPAD_LEFT"); break;
		//case WXK_NUMPAD_UP: key = _T("NUMPAD_UP"); break;
		//case WXK_NUMPAD_RIGHT: key = _T("NUMPAD_RIGHT"); break;
		//case WXK_NUMPAD_DOWN: key = _T("NUMPAD_DOWN"); break;
		//case WXK_NUMPAD_PAGEUP: key = _T("NUMPAD_PAGEUP"); break;
		//case WXK_NUMPAD_PAGEDOWN: key = _T("NUMPAD_PAGEDOWN"); break;
		//case WXK_NUMPAD_END: key = _T("NUMPAD_END"); break;
		//case WXK_NUMPAD_BEGIN: key = _T("NUMPAD_BEGIN"); break;
		//case WXK_NUMPAD_INSERT: key = _T("NUMPAD_INSERT"); break;
		//case WXK_NUMPAD_DELETE: key = _T("NUMPAD_DELETE"); break;
		//case WXK_NUMPAD_EQUAL: key = _T("NUMPAD_EQUAL"); break;
		//case WXK_NUMPAD_MULTIPLY: key = _T("NUMPAD_MULTIPLY"); break;
		//case WXK_NUMPAD_ADD: key = _T("NUMPAD_ADD"); break;
		//case WXK_NUMPAD_SEPARATOR: key = _T("NUMPAD_SEPARATOR"); break;
		//case WXK_NUMPAD_SUBTRACT: key = _T("NUMPAD_SUBTRACT"); break;
		//case WXK_NUMPAD_DECIMAL: key = _T("NUMPAD_DECIMAL"); break;
		case VK_BACK: key = _T("VK_BACK"); break;; break;
		case VK_TAB: key = _T("VK_TAB"); break;
		case VK_CLEAR: key = _T("VK_CLEAR"); break;
		case VK_RETURN: key = _T("VK_RETURN"); break;
		case VK_SHIFT: key = _T("VK_SHIFT"); break;
		case VK_CONTROL: key = _T("VK_CONTROL"); break;
		case VK_MENU: key = _T("VK_MENU"); break;
		case VK_PAUSE: key = _T("VK_PAUSE"); break;
		case VK_CAPITAL: key = _T("VK_CAPITAL"); break;
		case VK_KANA: key = _T("VK_KANA"); break;
		case VK_JUNJA: key = _T("VK_JUNJA"); break;
		case VK_FINAL: key = _T("VK_FINAL"); break;
		case VK_KANJI: key = _T("VK_KANJI"); break;
		case VK_ESCAPE: key = _T("VK_ESCAPE"); break;
		case VK_CONVERT: key = _T("VK_CONVERT"); break;
		case VK_NONCONVERT: key = _T("VK_NONCONVERT"); break;
		case VK_ACCEPT: key = _T("VK_ACCEPT"); break;
		case VK_MODECHANGE: key = _T("VK_MODECHANGE"); break;
		case VK_SPACE: key = _T("VK_SPACE"); break;
		case VK_PRIOR: key = _T("VK_PRIOR"); break;
		case VK_NEXT: key = _T("VK_NEXT"); break;
		case VK_END: key = _T("VK_END"); break;
		case VK_HOME: key = _T("VK_HOME"); break;
		case VK_LEFT: key = _T("VK_LEFT"); break;
		case VK_UP: key = _T("VK_UP"); break;
		case VK_RIGHT: key = _T("VK_RIGHT"); break;
		case VK_DOWN: key = _T("VK_DOWN"); break;
		case VK_SELECT: key = _T("VK_SELECT"); break;
		case VK_PRINT: key = _T("VK_PRINT"); break;
		case VK_EXECUTE: key = _T("VK_EXECUTE"); break;
		case VK_SNAPSHOT: key = _T("VK_SNAPSHOT"); break;
		case VK_INSERT: key = _T("VK_INSERT"); break;
		case VK_DELETE: key = _T("VK_DELETE"); break;
		case VK_HELP: key = _T("VK_HELP"); break;
		case VK_LWIN: key = _T("VK_LWIN"); break;
		case VK_RWIN: key = _T("VK_RWIN"); break;
		case VK_APPS: key = _T("VK_APPS"); break;
		case VK_SLEEP: key = _T("VK_SLEEP"); break;
		case VK_NUMPAD0: key = _T("VK_NUMPAD0"); break;
		case VK_NUMPAD1: key = _T("VK_NUMPAD1"); break;
		case VK_NUMPAD2: key = _T("VK_NUMPAD2"); break;
		case VK_NUMPAD3: key = _T("VK_NUMPAD3"); break;
		case VK_NUMPAD4: key = _T("VK_NUMPAD4"); break;
		case VK_NUMPAD5: key = _T("VK_NUMPAD5"); break;
		case VK_NUMPAD6: key = _T("VK_NUMPAD6"); break;
		case VK_NUMPAD7: key = _T("VK_NUMPAD7"); break;
		case VK_NUMPAD8: key = _T("VK_NUMPAD8"); break;
		case VK_NUMPAD9: key = _T("VK_NUMPAD9"); break;
		case VK_MULTIPLY: key = _T("VK_MULTIPLY"); break;
		case VK_ADD: key = _T("VK_ADD"); break;
		case VK_SEPARATOR: key = _T("VK_SEPARATOR"); break;
		case VK_SUBTRACT: key = _T("VK_SUBTRACT"); break;
		case VK_DECIMAL: key = _T("VK_DECIMAL"); break;
		case VK_DIVIDE: key = _T("VK_DIVIDE"); break;
		case VK_F1: key = _T("VK_F1"); break;
		case VK_F2: key = _T("VK_F2"); break;
		case VK_F3: key = _T("VK_F3"); break;
		case VK_F4: key = _T("VK_F4"); break;
		case VK_F5: key = _T("VK_F5"); break;
		case VK_F6: key = _T("VK_F6"); break;
		case VK_F7: key = _T("VK_F7"); break;
		case VK_F8: key = _T("VK_F8"); break;
		case VK_F9: key = _T("VK_F9"); break;
		case VK_F10: key = _T("VK_F10"); break;
		case VK_F11: key = _T("VK_F11"); break;
		case VK_F12: key = _T("VK_F12"); break;
		case VK_F13: key = _T("VK_F13"); break;
		case VK_F14: key = _T("VK_F14"); break;
		case VK_F15: key = _T("VK_F15"); break;
		case VK_F16: key = _T("VK_F16"); break;
		case VK_F17: key = _T("VK_F17"); break;
		case VK_F18: key = _T("VK_F18"); break;
		case VK_F19: key = _T("VK_F19"); break;
		case VK_F20: key = _T("VK_F20"); break;
		case VK_F21: key = _T("VK_F21"); break;
		case VK_F22: key = _T("VK_F22"); break;
		case VK_F23: key = _T("VK_F23"); break;
		case VK_F24: key = _T("VK_F24"); break;

		default:
		{
		   if ( wxIsprint((int)keycode) && keycode != 255)
			   key.Printf(_T("'%c'"), (char)keycode);
		   else if ( keycode > 0 && keycode < 27 )
			   key.Printf(_("Ctrl-%c"), _T('A') + keycode - 1);
		   else
			   key.Printf(_T("unknown (%ld)"), keycode);
		}
	}
	return key;
}

void KeyBindingTextCtrl::OnKeyDown(wxKeyEvent & event)
{
	m_rawKeyCode = event.GetRawKeyCode();
	m_keyCode = event.GetKeyCode();
	SetValue(GetStringFromCode(m_rawKeyCode));
}
