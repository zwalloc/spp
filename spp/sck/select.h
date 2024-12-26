#pragma once

#include <sck/sck.h>

namespace sck
{
	struct SockSetData
	{
		union
		{
			uint count;
			void* __pad;
		};

		inline socket_value* GetSockets() { return (socket_value*)(this + 1); }
	};

	template<size_t Size>
	struct FixedSockSet : public SockSetData
	{
		socket_value socks[Size];
	};

	class SockSet
	{
	public:
		SockSet();
		~SockSet();

		void Set(BasicSocket s);
		bool IsSet(BasicSocket s) const;

		inline void Clear() { mInfo->count = 0; };
		inline SockSetData* GetData() { return mInfo; };
		inline size_t GetSize() const { return mInfo->count; };

	private:

		SockSetData* mInfo; // fd_set
		size_t mCapacity;
	};

	typedef long BlockingDelay;
	enum : BlockingDelay
	{
		BlockingDelay_NonBlocking = 0,
		BlockingDelay_TillAction = -1
	};

	int Select(SockSetData* pSetRead, SockSetData* pSetWrite, SockSetData* pSetExcept, BlockingDelay microseconds);
}