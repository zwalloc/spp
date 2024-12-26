#pragma once

#include <ulib/containers/list.h>
#include "packet.h"

#include <bpacs/bpacs.hpp>

#include <map>

namespace spp
{
	enum class CallHandlersResult
	{
		Success = 0,
		NotFound = 1,
		BadProtocol = 2,
	};

	template<class StateT, class ConnT, class PacketViewT, int index = 0>
	class HandlerSystem
	{
	public:
		struct IHandler
		{
			virtual uint GetPacketTypeId() = 0;
			virtual uint GetPacketSizeLimit() = 0;
			virtual bool OnPacket(StateT state, ConnT conn, PacketViewT pac) = 0;
		};

		using Handlers = ulib::List<IHandler*>;
		using StateType = StateT;
		using ConnType = ConnT;
		using PacketViewType = PacketViewT;

		static constexpr int Index = index;

		static HandlerSystem& GetInstance()
		{
			static HandlerSystem inst;
			return inst;
		}

		inline void Register(IHandler* pHandler) { mHandlers.Add(pHandler); }
		inline Handlers& GetHandlers() { return mHandlers; }

		inline CallHandlersResult CallHandlers(StateT state, ConnT conn, PacketViewT pac, uint type)
		{
			bool result = false;
			for (IHandler* pHandler : mHandlers)
			{
				if (pHandler->GetPacketTypeId() == type)
				{
					if (!pHandler->OnPacket(state, conn, pac))
						return CallHandlersResult::BadProtocol;

					result = true;
				}
			}

			return result ? CallHandlersResult::Success : CallHandlersResult::NotFound;
		}

		inline IHandler* GetHandler(uint type)
		{
			for (IHandler* pHandler : mHandlers)
				if (pHandler->GetPacketTypeId() == type)
					return pHandler;

			return nullptr;
		}

	private:
		Handlers mHandlers;
	};

	template<class TrT, class HnT>
	void LoadPacketSizeLimits(TrT& dest, HnT& src)
	{
		auto& hns = src.GetHandlers();
		for (auto pt : hns)
			dest.SetPacketSizeLimit(pt->GetPacketTypeId(), pt->GetPacketSizeLimit());
	}
}

#define SPP_DECL_HANDLER(name, packetType, packetSizeLimit, HandlerSystemT, state, conn, pac) \
struct __SPP_##_##name##_Handler : public HandlerSystemT::IHandler \
{ \
	__SPP_##_##name##_Handler() { HandlerSystemT::GetInstance().Register(this); }\
	uint GetPacketTypeId() { return packetType; } \
	uint GetPacketSizeLimit() { return packetSizeLimit; } \
	bool OnPacket(HandlerSystemT::StateType state, HandlerSystemT::ConnType conn, HandlerSystemT::PacketViewType pac); \
}; \
static __SPP_##_##name##_Handler G__SPP_##_##name##_##_Handler_InitObject; \
bool __SPP_##_##name##_Handler::OnPacket(HandlerSystemT::StateType state, HandlerSystemT::ConnType conn, HandlerSystemT::PacketViewType pac)

