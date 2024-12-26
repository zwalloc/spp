#pragma once

/*
	- sck/sys.h
	- This file will be included only in .cpp sck files.
*/

#include "ip.h"

namespace sck
{
	bool global_init();
	void global_cleanup();

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

	struct timeval { long sec, usec; };
	int select(void* r, void* w, void* e, timeval* t);

	int available(socket_value s);

	size_t getaddrinfo_tcp_v4(const char* node, const char* service, sck::ip::v4::Endpoint* pEndpoints, size_t maxEndpoints);
}