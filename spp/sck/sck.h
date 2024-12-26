#pragma once

#include "ip.h"

namespace sck
{
	class Initializator
	{
	public:
		Initializator();
		~Initializator();

		inline bool IsInitialized() { return mIsInit; };

	private:
		bool mIsInit;
	};

	class BasicSocket
	{
	public:
		BasicSocket();
		BasicSocket(socket_value v);

		void Close();
		bool IsOpen() const;
		void SetNonBlocking(bool t);
		bool Bind(const ip::v4::Endpoint& ep);
		ip::v4::Endpoint GetIpv4();

		socket_value Value();
		int Available();

	protected:
		socket_value mSock;
	};

	namespace tcp
	{
		class Socket : public BasicSocket
		{
		public:
			Socket();
			Socket(socket_value v);

			bool Open();
			bool Connect(const ip::v4::Endpoint& ep);

			int Send(const void* buf, int len);
			int Recv(void* buf, int len);
		};

		class Acceptor : public Socket
		{
		public:
			bool Listen();
			Socket Accept();
		};
	};

	namespace udp
	{
		class Socket : public BasicSocket
		{
		public:
			Socket();
			Socket(socket_value v);

			bool Open();
			int Send(const void* buf, int len, const ip::v4::Endpoint& ep);
			int Recv(void* buf, int len, ip::v4::Endpoint* out = nullptr);
		};
	};

	size_t GetAddrInfoTcpV4(const char* node, const char* service, sck::ip::v4::Endpoint* pEndpoints, size_t maxEndpoints);
}

