#pragma once

#include "handler.h"
#include "schema.h"

#define SPP_DECL_SCHEMA_HANDLER(name, schema, HandlerSystemT, state, conn, pac) \
struct __SPP_##_##name##_Handler : public HandlerSystemT::IHandler \
{ \
	__SPP_##_##name##_Handler() { mPacketSizeLimit = uint(spp::max_size_schema<schema>()); HandlerSystemT::GetInstance().Register(this); }\
	uint GetPacketTypeId() { return schema::PacketType; } \
	uint GetPacketSizeLimit() { return mPacketSizeLimit; } \
	bool OnPacket(HandlerSystemT::StateType state, HandlerSystemT::ConnType conn, HandlerSystemT::PacketViewType packet) { schema sch; if(!spp::read_schema(packet, sch)) return false; OnSchema(state, conn, sch); return true;  } \
	void OnSchema(HandlerSystemT::StateType state, HandlerSystemT::ConnType conn, const schema& packet); \
	uint mPacketSizeLimit; \
}; \
static __SPP_##_##name##_Handler G__SPP_##_##name##_##_Handler_InitObject; \
void __SPP_##_##name##_Handler::OnSchema(HandlerSystemT::StateType state, HandlerSystemT::ConnType conn, const schema& packet)