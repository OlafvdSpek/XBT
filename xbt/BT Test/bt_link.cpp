// bt_link.cpp: implementation of the Cbt_link class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "bt_link.h"

#include "server.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Cbt_link::Cbt_link()
{
}

Cbt_link::Cbt_link(Cserver* server, const sockaddr_in& a, const Csocket& s)
{
	m_a = a;
	m_s = s;
	m_server = server;
	m_ctime = m_mtime = time(NULL);

	m_read_b.size(69);
}

int Cbt_link::pre_select(fd_set* fd_read_set, fd_set* fd_write_set, fd_set* fd_except_set)
{
	if (m_read_b.cb_w())
		FD_SET(m_s, fd_read_set);
	return m_s;
}

void Cbt_link::post_select(fd_set* fd_read_set, fd_set* fd_write_set, fd_set* fd_except_set)
{
	if (m_read_b.cb_w() && FD_ISSET(m_s, fd_read_set))
		recv();
	if (time(NULL) - m_ctime > 5)
		close();
}

void Cbt_link::recv()
{
	for (int r; r = m_s.recv(m_read_b.w(), m_read_b.cb_w()); )
	{
		if (r == SOCKET_ERROR)
		{
			int e = WSAGetLastError();
			switch (e)
			{
			case WSAECONNABORTED:
			case WSAECONNRESET:
				close();
			case WSAEWOULDBLOCK:
				break;
			default:
				cerr << "recv failed: " << e << endl;
				close();
			}
			return;
		}
		m_read_b.cb_w(r);
		m_mtime = time(NULL);
		if (m_read_b.cb_r() >= sizeof(t_bt_handshake))
		{
			const t_bt_handshake& m = *reinterpret_cast<const t_bt_handshake*>(m_read_b.r());
			if (m.cb_name == 19 && !memcmp(m.name, "BitTorrent protocol", 19))
				m_server->insert_peer(m, m_a, m_s);
			close();
			return;
		}
	}
	close();
}

void Cbt_link::close()
{
	m_s.close();
}
