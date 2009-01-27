#include "Client.h"

Client::Client(wxSocketBase * socket, bool local)
{
	m_socket = socket;
	m_localHost = local;
}

Client::~Client()
{
	m_socket->Destroy();
}

bool Client::IsLocalHost()
{
	return m_localHost;
}
