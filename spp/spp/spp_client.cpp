#include "client.h"

namespace spp
{
	namespace tcp
	{
		void Client::Prepare() {}

		void Client::Set(sck::SockSet& readSet, sck::SockSet& writeSet)
		{
			auto s = mTransfer.GetSocket();

			readSet.Set(s);

			if (mTransfer.GetSendingSize())
				writeSet.Set(s);
		}

		void Client::Process(const sck::SockSet& readSet, const sck::SockSet& writeSet)
		{
			if (readSet.IsSet(mTransfer.GetSocket()))
			{
				ReceiveResult r = mTransfer.Receive();
				if (r == ReceiveResult::ConnectionClosed) // connection closed
				{
					DisconnectEvent(ClientDisconnectReason::FromServer);
					return;
				}
				else if (r == ReceiveResult::InternalError) // connection broken
				{
					DisconnectEvent(ClientDisconnectReason::Crash);
					return;
				}
				else
				{
					while (true)
					{
						ClientEvent ev;
						spp::UpdatePacketResult result = mTransfer.UpdatePacket(ev.pac);
						if (result == spp::UpdatePacketResult::Success)
						{
							ev.type = ClientEventType::TcpPacket;
							mEventProcessor->Process(ev);
						}
						else if (result == spp::UpdatePacketResult::OversizedPacket)
						{
							DisconnectEvent(ClientDisconnectReason::OversizedPacket);
							return;
						}
						else if (result == spp::UpdatePacketResult::NoData)
						{
							break;
						}
					}
				}
			}
			
			if (writeSet.IsSet(mTransfer.GetSocket()))
			{
				ProcessSendResult r = mTransfer.ProcessSend();
				if (r == ProcessSendResult::NoBytesSended) // connection closed
				{
					DisconnectEvent(ClientDisconnectReason::FromServer);
					return;
				}
				else if (r == ProcessSendResult::InternalError) // connection broken
				{				
					DisconnectEvent(ClientDisconnectReason::Crash);
					return;
				}
			}
		}	

		void Client::DisconnectEvent(ClientDisconnectReason reason)
		{
			ClientEvent ev;
			ev.type = ClientEventType::Disconnected;
			ev.disconnectReason = reason;
			mEventProcessor->Process(ev);
		}
	}

	namespace udp
	{
		void Client::Prepare() {}

		void Client::Set(sck::SockSet& readSet, sck::SockSet& writeSet)
		{
			readSet.Set(mTransfer.GetSocket());
		}

		void Client::Process(const sck::SockSet& readSet, const sck::SockSet& writeSet)
		{
			if (readSet.IsSet(mTransfer.GetSocket()))
			{
				mTransfer.ReceiveDataSnapshot();

				ClientEvent ev;
				while (true)
				{
					ReceiveResult rr = mTransfer.ReceivePacket(ev.pac, mEp);
					if (rr == ReceiveResult::Success)
					{
						ev.type = ClientEventType::UdpPacket;
						mEventProcessor->Process(ev);
					}
					else if (rr == ReceiveResult::NoDataAvailable)
					{
						break;
					}
					else
					{
						assert(false && "Undefined behavior in udp::Client::Process");
					}
				}
			}		
		}
	}
}