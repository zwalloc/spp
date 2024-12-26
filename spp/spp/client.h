#pragma once

#include "transfer.h"
#include "iservice.h"

namespace spp
{
	namespace tcp
	{
		enum class ClientEventType
		{
			TcpPacket,
			Disconnected
		};

		enum class ClientDisconnectReason
		{
			FromClient,
			FromServer,
			Crash,
			OversizedPacket
		};


		struct ClientEvent
		{
			ClientEvent() {}
			~ClientEvent() {}

			ClientEventType type;

			union
			{
				PacketView pac;
				ClientDisconnectReason disconnectReason;
			};
		};

		struct IEventProcessor
		{
			virtual void Process(const ClientEvent& event) = 0;
		};

		class Client : public IService
		{
		public:
			Client(IEventProcessor* pEventProcessor, IPacketRules* pPacketRules) : mEventProcessor(pEventProcessor), mTransfer(pPacketRules) {}
			~Client() {}

			void Prepare();
			void Set(sck::SockSet& readSet, sck::SockSet& writeSet);
			void Process(const sck::SockSet& readSet, const sck::SockSet& writeSet);

			inline bool IsOpen() { return mTransfer.IsOpen(); }
			inline bool Connect(const sck::ip::v4::Endpoint& ep) { return mTransfer.Connect(ep); }
			inline void Disconnect() { mTransfer.Close(); }
			inline tcp::Transfer& GetTransfer() { return mTransfer; }
			inline sck::tcp::Socket& GetSocket() { return mTransfer.GetSocket(); }

			inline void Send(spp::PacketView pac) { mTransfer.Send(pac); }
			inline spp::PacketWrite Send() { return mTransfer.Send(); }		

		private:

			// only execute event
			void DisconnectEvent(ClientDisconnectReason reason);

			tcp::Transfer mTransfer;
			IEventProcessor* mEventProcessor;
		};
	}

	namespace udp
	{
		enum class ClientEventType
		{
			UdpPacket
		};

		struct ClientEvent
		{
			ClientEvent() {}
			~ClientEvent() {}

			ClientEventType type;
			PacketView pac;
		};

		struct IEventProcessor
		{
			virtual void Process(const ClientEvent& event) = 0;
		};

		class Client : public IService
		{
		public:
			Client(IEventProcessor* pEventProcessor, IPacketRules* pPacketRules) : mEventProcessor(pEventProcessor), mTransfer(pPacketRules) {}
			~Client() { mTransfer.Close(); }

			void Prepare();
			void Set(sck::SockSet& readSet, sck::SockSet& writeSet);
			void Process(const sck::SockSet& readSet, const sck::SockSet& writeSet);

			bool Connect(const sck::ip::v4::Endpoint& ep) 
			{
				if (!mTransfer.Open())
					return false;

				mEp = ep;
				return true;
			}
			void Disconnect() { mTransfer.Close(); }
			int Send(spp::PacketView pac) { return mTransfer.Send(pac, mEp); }
			inline udp::Transfer& GetTransfer() { return mTransfer; }
			inline sck::udp::Socket& GetSocket() { return mTransfer.GetSocket(); }
			inline bool IsOpen() { return mTransfer.IsOpen(); }
			

		private:
			sck::ip::v4::Endpoint mEp;
			udp::Transfer mTransfer;
			IEventProcessor* mEventProcessor;
		};
	}
}