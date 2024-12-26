#pragma once

#include <sck/select.h>

namespace spp
{
	struct IService
	{
		~IService() {}

		virtual void Prepare() = 0;
		virtual void Set(sck::SockSet& readSet, sck::SockSet& writeSet) = 0;
		virtual void Process(const sck::SockSet& readSet, const sck::SockSet& writeSet) = 0;
	};
}