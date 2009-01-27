#include <wx/socket.h>

class Client
{
public:
	wxSocketBase * m_socket;
	bool m_localHost;
	
	Client(wxSocketBase * socket, bool local = false);
	~Client();
	
	bool IsLocalHost();
};
