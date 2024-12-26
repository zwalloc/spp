#pragma comment(lib, "Ws2_32.lib")
#include <ws2tcpip.h>
#include <stdio.h>
#include <assert.h>

#include "sys.h"

#include <string>

#define htons(x) (((x) << 8) | ((x) >> 8))
#define ntohs htons

static sockaddr ep_to_sa(const sck::ip::v4::Endpoint& ep)
{
	sockaddr_in a;
	a.sin_addr.S_un.S_addr = ep.addr.ul;
	a.sin_port = htons(ep.port);
	a.sin_family = AF_INET;

	return *(sockaddr*)&a;
}

static sck::ip::v4::Endpoint sa_to_ep(const sockaddr& sa)
{
	const sockaddr_in* p = reinterpret_cast<const sockaddr_in*>(&sa);
	return sck::ip::v4::Endpoint(p->sin_addr.S_un.S_addr, ntohs(p->sin_port));
}

namespace sck
{
	bool global_init()
	{
		WSADATA d;
		return WSAStartup(MAKEWORD(2, 2), &d) == 0;
	}

	void global_cleanup()
	{
		
		WSACleanup();
	}

	socket_value socket_tcp() { return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); }
	socket_value socket_udp() { return ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); }

	void close_socket(socket_value s) { closesocket(s); }

	bool bind(socket_value s, const ip::v4::Endpoint& ep) { sockaddr sa = ep_to_sa(ep); return ::bind(s, &sa, sizeof(sockaddr)) != -1; }
	bool connect(socket_value s, const ip::v4::Endpoint& ep) { sockaddr sa = ep_to_sa(ep); return ::connect(s, &sa, sizeof(sockaddr)) != -1; }
	bool listen(socket_value s) { return ::listen(s, SOMAXCONN) != -1; }
	socket_value accept(socket_value s) { return ::accept(s, nullptr, nullptr); }

	void non_blocking(socket_value s, bool t)
	{
		u_long iMode = u_long(t);
		::ioctlsocket(s, FIONBIO, &iMode);
	}

	ip::v4::Endpoint getpeername_ipv4(socket_value s)
	{
		sck::ip::v4::Endpoint ep;
		sockaddr_in ai;
		int si = sizeof(ai);

		::getpeername(s, (sockaddr*)&ai, &si);

		ep.addr = ai.sin_addr.S_un.S_addr;
		ep.port = ntohs(ai.sin_port);

		return ep;
	}

	std::string GetLastErrorAsString(DWORD error)
	{
		//Get the error message ID, if any.
		DWORD errorMessageID = error;
		if (errorMessageID == 0) {
			return std::string(); //No error message has been recorded
		}
		AF_INET;
		LPSTR messageBuffer = nullptr;

		//Ask Win32 to give us the string version of that message ID.
		//The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		//Copy the error message into a std::string.
		std::string message(messageBuffer, size);

		//Free the Win32's string's buffer.
		LocalFree(messageBuffer);

		return message;
	}

	int send(socket_value s, const void* buf, int len) { return ::send(s, (char*)buf, len, 0); }
	int recv(socket_value s, void* buf, int len) 
	{ 
		int rr = ::recv(s, (char*)buf, len, 0);

		
		if (rr == -1)
		{
			// printf("recv wsaerror: %d\n", WSAGetLastError());

			// gLog->Warn("recv error: {0}:0x{0:X}", WSAGetLastError());
		}

		return rr;

		/*
		WSABUF wb;
		wb.buf = (char*)buf;
		wb.len = len;

		DWORD flags = MSG_PUSH_IMMEDIATE;
		DWORD bytes = 0;

		int res = ::WSARecv(s, &wb, 1, &bytes, &flags, 0, 0);
		if (res == -1)
			return -1;
		
		return bytes;
		*/	
	}

	int sendto(socket_value s, const void* buf, int len , const ip::v4::Endpoint& ep)
	{
		sockaddr sa = ep_to_sa(ep); 
		return ::sendto(s, (char*)buf, len, 0, &sa, sizeof(sockaddr));
	}

	int recvfrom(socket_value s, void* buf, int len, ip::v4::Endpoint* out)
	{
		if (out)
		{
			sockaddr_in sa;
			int v = sizeof(sa);

			int result = ::recvfrom(s, (char*)buf, len, 0, (sockaddr*)&sa, &v);

			out->addr.ul = sa.sin_addr.S_un.S_addr;
			out->port = ntohs(sa.sin_port);

			return result;
		}
		else
		{
			return ::recvfrom(s, (char*)buf, len, 0, nullptr, nullptr);
		}
	}

	int select(void* r, void* w, void* e, timeval* t)
	{
		return ::select(0, (fd_set*)r, (fd_set*)w, (fd_set*)e, (::timeval*)t);


		// block till action
		// select(0, 0, 0, nullptr);

		// non blocking
		// ullong tt = 0;
		// select(0, 0, 0, &tt);

		// block delay
		// ullong tt = 228;
		// select(0,0,0, &tt);
	}

    int available(socket_value s)
    {
		u_long bytes;
		int rr = ioctlsocket(s, FIONREAD, &bytes);
		assert(rr == 0 && "ioctlsocket socket failed");
        return int(bytes);
    }

	size_t getaddrinfo_tcp_v4(const char* node, const char* service, sck::ip::v4::Endpoint* pEndpoints, size_t maxEndpoints)
	{
		addrinfo hints = {};
		addrinfo* result = NULL;

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		if (getaddrinfo(node, service, &hints, &result))
			return 0;

		size_t i = 0;
		for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next) 
		{
			if (i == maxEndpoints)
				break;

			pEndpoints[i] = sa_to_ep(*ptr->ai_addr);
			i++;

			break;
		}

		if (result)
			freeaddrinfo(result);

		return i;
	}

	/*
		int async_recv(socket_value s, void* buf, size_t len)
	{
		WSABUF wsb;
		wsb.buf = (char*)buf;
		wsb.len = len;

		DWORD flags = 0;
		WSARecv(s, &wsb, 1, nullptr, &flags, );
	}

	*/

}