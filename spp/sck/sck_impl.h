#pragma once

#include "sck_ip.h"

namespace sck
{
	socket_value socket_tcp();
	socket_value socket_udp();
	void close_socket(socket_value s);

	bool bind(socket_value s, const sck::ip::v4::Endpoint& ep);
	bool connect(socket_value s, const sck::ip::v4::Endpoint& ep);
	bool listen(socket_value s);
	socket_value accept(socket_value s);

	void non_blocking(socket_value s, bool t);

	sck::ip::v4::Endpoint getpeername_ipv4(socket_value s);

	int send(socket_value s, const void* buf, int len);
	int recv(socket_value s, void* buf, int len);

	int sendto(socket_value s, const void* buf, int len, const sck::ip::v4::Endpoint& ep);
	int recvfrom(socket_value s, void* buf, int len, sck::ip::v4::Endpoint* out);

	int select(void* r, void* w, void* e);
	int nbselect(void* r, void* w, void* e);
}