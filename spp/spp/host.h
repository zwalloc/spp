#pragma once

#include <sck/sck.h>
#include <ulib/containers/list.h>
#include <ulib/containers/queue.h>
#include "transfer.h"
#include "iservice.h"

namespace spp
{
	namespace tcp
	{
		class RemoteTransfer : public Transfer
		{
		public:
			RemoteTransfer(sck::tcp::Socket s, IPacketRules* pPacketRules);

			template<class T> T* GetIdentifier() { return (T*)mMI; }
			template<class T> void SetIdentifier(T* m) { mMI = m; }

			inline bool IsDisconnecting() { return mDn; }
			inline void Disconnect() { mDn = true; }

			inline void SetLastSendActivityTime(time_t t) { mLastSendActivityTime = t; }
			inline time_t GetLastSendActivityTime() { return mLastSendActivityTime; }
			inline void SetLastRecvActivityTime(time_t t) { mLastRecvActivityTime = t; }
			inline time_t GetLastRecvActivityTime() { return mLastRecvActivityTime; }

		private:

			bool mDn;
			void* mMI;
			time_t mLastSendActivityTime, mLastRecvActivityTime;
		};
	}

	namespace tcp
	{
		enum class HostEventType
		{
			Connected,
			Disconnected,
			TcpPacket,
		};

		enum class HostDisconnectReason
		{
			FromClient,
			FromServer,
			Broken,
			Crash,
			OversizedPacket,
			SendingTimeout,
			ReceivingTimeout,
		};

		struct HostEvent
		{
			HostEvent() {}
			~HostEvent() {}

			HostEventType type;
			tcp::RemoteTransfer* conn;

			union
			{
				spp::PacketView pac;
				HostDisconnectReason disconnectReason;
			};

		};

		struct IEventProcessor
		{
			virtual void Process(const HostEvent& event) = 0;
		};
		
		class Host : public IService
		{
		public:
			Host(IEventProcessor* pEventProcessor, IPacketRules* pPacketRules) : mPacketRules(pPacketRules), mEventProcessor(pEventProcessor) {}
			~Host();


			int Run(ushort port);
			void Stop();

			void Prepare();
			void Set(sck::SockSet& readSet, sck::SockSet& writeSet);
			void Process(const sck::SockSet& readSet, const sck::SockSet& writeSet);

			void BreakConnection(RemoteTransfer* conn);
			inline void Disconnect(RemoteTransfer* conn) { conn->Disconnect(); }

		private:

			bool RemoveConnection(RemoteTransfer* conn);
			void BreakConnection(RemoteTransfer* conn, HostDisconnectReason reason);

			sck::tcp::Acceptor mAcceptor;
			ulib::List<RemoteTransfer*> mConnections;

			IPacketRules* mPacketRules;
			IEventProcessor* mEventProcessor;
		};
	}

	namespace udp
	{
		enum class HostEventType
		{
			UdpPacket,
			BadUdpPacket
		};

		struct HostEvent
		{
			HostEvent() {}
			~HostEvent() {}

			HostEventType type;
			sck::ip::v4::Endpoint endPoint;
			spp::PacketView pac;
		};

		struct IEventProcessor
		{
			virtual void Process(const HostEvent& event) = 0;
		};

		class Host : public IService
		{
		public:
			Host(IEventProcessor* pEventProcessor, IPacketRules* pPacketRules) : mIo(pPacketRules), mEventProcessor(pEventProcessor) {}
			~Host() { mIo.Close(); }

			int Run(ushort port);
			void Stop();
			void Prepare();
			void Set(sck::SockSet& readSet, sck::SockSet& writeSet);
			void Process(const sck::SockSet& readSet, const sck::SockSet& writeSet);
			inline int Send(PacketView pac, const sck::ip::v4::Endpoint& ep) { return mIo.Send(pac, ep); }
			inline udp::Transfer& GetTransfer() { return mIo; }


		private:
			udp::Transfer mIo;
			IEventProcessor* mEventProcessor;
		};
	}
}
