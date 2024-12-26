#include "sys.h"
#include "sck.h"

namespace sck
{
	Initializator::Initializator()
	{
		mIsInit = global_init();
	}

	Initializator::~Initializator()
	{
		if (mIsInit)
			global_cleanup();
	}

	using namespace ip;

	BasicSocket::BasicSocket() : mSock(-1) {}
	BasicSocket::BasicSocket(socket_value sock) : mSock(sock) {}
	void BasicSocket::Close() { sck::close_socket(mSock); mSock = -1; }
	bool BasicSocket::IsOpen() const { return mSock != -1; }
	void BasicSocket::SetNonBlocking(bool t) { sck::non_blocking(mSock, t); }
	bool BasicSocket::Bind(const v4::Endpoint& ep) { return sck::bind(mSock, ep); }
	v4::Endpoint BasicSocket::GetIpv4() { return sck::getpeername_ipv4(mSock); }
	socket_value BasicSocket::Value() { return mSock; }
	int BasicSocket::Available() { return sck::available(mSock); }

	tcp::Socket::Socket() : BasicSocket() {}
	tcp::Socket::Socket(socket_value sock) : BasicSocket(sock) {}
	bool tcp::Socket::Open() { return (mSock = socket_tcp()) != -1; }
	bool tcp::Socket::Connect(const v4::Endpoint& ep) { return sck::connect(mSock, ep); }
	int tcp::Socket::Send(const void* buf, int len) { return sck::send(mSock, buf, len); }
	int tcp::Socket::Recv(void* buf, int len) { return sck::recv(mSock, buf, len); }

	bool tcp::Acceptor::Listen() { return sck::listen(mSock); }
	tcp::Socket tcp::Acceptor::Accept() { return tcp::Socket(sck::accept(mSock)); }

	udp::Socket::Socket() : BasicSocket() {}
	udp::Socket::Socket(socket_value sock) : BasicSocket(sock) {}

	bool udp::Socket::Open() { return (mSock = sck::socket_udp()) != -1; }
	int udp::Socket::Send(const void* buf, int len, const ip::v4::Endpoint& ep) { return sck::sendto(mSock, buf, len, ep); }
	int udp::Socket::Recv(void* buf, int len, ip::v4::Endpoint* out) { return sck::recvfrom(mSock, buf, len, out); }

	size_t GetAddrInfoTcpV4(const char* node, const char* service, sck::ip::v4::Endpoint* pEndpoints, size_t maxEndpoints)
	{
		return sck::getaddrinfo_tcp_v4(node, service, pEndpoints, maxEndpoints);
	}

}
