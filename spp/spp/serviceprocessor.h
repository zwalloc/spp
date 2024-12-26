#pragma once

#include <ulib/containers/list.h>
#include "iservice.h"
#include <sck/select.h>

namespace spp
{
	class ServiceProcessor
	{
	public:
		ServiceProcessor(sck::BlockingDelay microseconds)
			: mMicroseconds(microseconds)
		{}

		~ServiceProcessor() {}

		inline void SetBlockingTimeout(sck::BlockingDelay microseconds) { mMicroseconds = microseconds; }
		inline sck::BlockingDelay GetBlockingTimeout() { return mMicroseconds; }

		void Process(IService* host);
		void Process(const ulib::List<IService*>& list);

	private:
		sck::SockSet mReadSet;
		sck::SockSet mWriteSet;

		sck::BlockingDelay mMicroseconds;
	};
}