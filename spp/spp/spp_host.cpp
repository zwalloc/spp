#include "host.h"

#include <time.h>

template<class T>
class DifferenceChecker
{
public:
	inline DifferenceChecker(T value)
		: mValue(value)
	{}

	inline bool IsDifference(T value) { return mValue != value; }

private:
	T mValue;
};

spp::tcp::RemoteTransfer::RemoteTransfer(sck::tcp::Socket s, IPacketRules* pPacketRules) : Transfer(s, pPacketRules), mMI(0), mDn(false)
{
	mLastSendActivityTime =
		mLastRecvActivityTime =
		time(0);
}

spp::tcp::Host::~Host()
{
	if (mAcceptor.IsOpen())
		mAcceptor.Close();

	for (tcp::RemoteTransfer* pConn : mConnections) delete pConn;
}

/*
template<class T>
bool GetSocketOption(sck::socket_value s, int level, int option, T& out)
{
	int optlen = sizeof(out);
	return getsockopt(s, level, option, (char*)&out, &optlen) == 0;
}


void CheckSocketOption(sck::socket_value s, int level, int option)
{
	int data;
	if (!GetSocketOption<int>(s, level, option, data))
	{
		return;
	}
}

*/

int spp::tcp::Host::Run(ushort port)
{
	if (!mAcceptor.Open()) return 1;
	if (!mAcceptor.Bind(sck::ip::v4::Endpoint(sck::ip::v4::Addr::any, port))) return 2;
	if (!mAcceptor.Listen()) return 3;

	return 0;
}
void spp::tcp::Host::Stop()
{
	if (mAcceptor.IsOpen())
		mAcceptor.Close();

	for (tcp::RemoteTransfer* pConn : mConnections) delete pConn;

	mConnections.Clear();
}

void spp::tcp::Host::Prepare()
{
	
}

void spp::tcp::Host::Set(sck::SockSet& readSet, sck::SockSet& writeSet)
{
	readSet.Set(mAcceptor);
	for (tcp::RemoteTransfer* conn : mConnections)
	{
		sck::tcp::Socket sock = conn->GetSocket();
		if (!conn->IsDisconnecting())
			readSet.Set(sock);

		if (conn->GetSendingSize())
			writeSet.Set(sock);
	}
}

void spp::tcp::Host::Process(const sck::SockSet& readSet, const sck::SockSet& writeSet)
{
	time_t tt = time(0);
	for (auto it = mConnections.Begin(); it != mConnections.End();)
	{
		tcp::RemoteTransfer* conn = *it;
		if (!conn->IsDisconnecting() && readSet.IsSet(conn->GetSocket()))
		{
			conn->SetLastRecvActivityTime(tt);

			ReceiveResult rr = conn->Receive();
			if (rr == ReceiveResult::ConnectionClosed || rr == ReceiveResult::InternalError)
			{
				BreakConnection(conn, rr == ReceiveResult::ConnectionClosed ? HostDisconnectReason::FromClient : HostDisconnectReason::Crash);
				continue;
			}

			HostEvent ev;
			ev.conn = conn;
			ev.type = HostEventType::TcpPacket;

			UpdatePacketResult result;
			while ((result = conn->UpdatePacket(ev.pac)) == UpdatePacketResult::Success)
			{
				DifferenceChecker check(mConnections.End());
				mEventProcessor->Process(ev);

				// check if mEventProcessor->Process(ev) removes one of more connections
				if (check.IsDifference(mConnections.End()))
					break;
			}		

			if (result == UpdatePacketResult::Success)
				continue;

			if (result == UpdatePacketResult::OversizedPacket)
			{
				BreakConnection(conn, HostDisconnectReason::OversizedPacket);
				continue;
			}
		}

		it++;
	}

	for (tcp::RemoteTransfer* conn : mConnections)
	{
		if (writeSet.IsSet(conn->GetSocket()))
		{
			if (conn->ProcessSend() == ProcessSendResult::Success)
				conn->SetLastSendActivityTime(tt);
		}

		if (conn->GetSendingSize() == 0)
		{
			if (conn->IsDisconnecting())
			{
				BreakConnection(conn, HostDisconnectReason::FromServer);
				continue;
			}
		}
		else
		{
			if (tt - conn->GetLastSendActivityTime() > 60)
			{
				BreakConnection(conn, HostDisconnectReason::SendingTimeout);
				continue;
			}
		}

		if (conn->GetReceivedSize())
		{
			if (tt - conn->GetLastRecvActivityTime() > 60)
			{
				BreakConnection(conn, HostDisconnectReason::ReceivingTimeout);
				continue;
			}
		}
	}

	if (readSet.IsSet(mAcceptor))
	{
		sck::tcp::Socket cs = mAcceptor.Accept();
		if (cs.IsOpen())
		{
			tcp::RemoteTransfer* conn = new tcp::RemoteTransfer(cs, mPacketRules);
			mConnections.Add(conn);

			HostEvent ev;
			ev.type = HostEventType::Connected;
			ev.conn = conn;
			mEventProcessor->Process(ev);
		}
	}

}

void spp::tcp::Host::BreakConnection(tcp::RemoteTransfer* conn)
{
	BreakConnection(conn, HostDisconnectReason::Broken);
}

bool spp::tcp::Host::RemoveConnection(tcp::RemoteTransfer* conn)
{
	for (auto it = mConnections.Begin(); it != mConnections.End(); it++)
	{
		if (*it == conn)
		{
			mConnections.FastErase(it);
			return true;
		}
	}

	return false;
}

void spp::tcp::Host::BreakConnection(tcp::RemoteTransfer* conn, HostDisconnectReason reason)
{
	if (RemoveConnection(conn))
	{
		HostEvent ev;
		ev.type = HostEventType::Disconnected;
		ev.conn = conn;
		ev.disconnectReason = reason;
		mEventProcessor->Process(ev);
		delete conn;
	}
}

int spp::udp::Host::Run(ushort port)
{
	sck::udp::Socket udpSocket;

	if (!udpSocket.Open()) return 4;
	if (!udpSocket.Bind(sck::ip::v4::Endpoint(sck::ip::v4::Addr::any, port))) return 5;

	mIo.SetSocket(udpSocket);

	return 0;
}
void spp::udp::Host::Stop()
{
	mIo.Close();
}

void spp::udp::Host::Prepare()
{

}

void spp::udp::Host::Set(sck::SockSet& readSet, sck::SockSet& writeSet)
{
	readSet.Set(mIo.GetSocket());
}

void spp::udp::Host::Process(const sck::SockSet& readSet, const sck::SockSet& writeSet)
{
	if (readSet.IsSet(mIo.GetSocket()))
	{
		HostEvent ev;

		mIo.ReceiveDataSnapshot();
		while (true)
		{
			ReceiveResult rr = mIo.ReceivePacket(ev.pac, ev.endPoint);
			if (rr == ReceiveResult::Success)
			{
				ev.type = HostEventType::UdpPacket;
				mEventProcessor->Process(ev);
			}				
			else if (rr == ReceiveResult::NoDataAvailable)
			{
				break;
			}
			else if (rr == ReceiveResult::BadProtocolDetected)
			{
				ev.type = HostEventType::BadUdpPacket;
				mEventProcessor->Process(ev);
			}
			else if (rr == ReceiveResult::ConnectionClosed)
			{
				ev.type = HostEventType::BadUdpPacket;
				mEventProcessor->Process(ev);
			}
			else // internal error
			{
				ev.type = HostEventType::BadUdpPacket;
				mEventProcessor->Process(ev);
			}
		}
	}
}

