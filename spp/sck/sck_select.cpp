#include "select.h"
#include "sys.h"

#include <cstring> // memcpy
#include <assert.h>

namespace sck
{
	size_t CalcSockSetDataLength(size_t count)
	{
		return sizeof(size_t) + (sizeof(socket_value) * count);
	}

	SockSet::SockSet()
	{
		mCapacity = 64;
		mInfo = (SockSetData*)new uchar[CalcSockSetDataLength(mCapacity)];
		mInfo->count = 0;
	}

	SockSet::~SockSet()
	{
		delete[] mInfo;
	}

	void SockSet::Set(BasicSocket s)
	{
		if (mInfo->count == mCapacity)
		{
			size_t newCapacity = mInfo->count + 64;
			SockSetData* pOldData = mInfo;
			mInfo = (SockSetData*)new uchar[CalcSockSetDataLength(newCapacity)];
			memcpy(mInfo, pOldData, CalcSockSetDataLength(pOldData->count));
			delete[](uchar*)pOldData;
			mCapacity = newCapacity;
		}

		socket_value* socks = mInfo->GetSockets();
		socks[mInfo->count] = s.Value();
		mInfo->count++;
	}

	bool SockSet::IsSet(BasicSocket s) const
	{
		socket_value* socks = mInfo->GetSockets();
		for (uint i = 0; i != mInfo->count; i++)
			if (socks[i] == s.Value())
				return true;
		return false;
	}

	int Select(SockSetData* pSetRead, SockSetData* pSetWrite, SockSetData* pSetExcept, BlockingDelay microseconds)
	{
		assert(microseconds >= BlockingDelay_TillAction);

		if (microseconds == BlockingDelay_TillAction)
		{
			return sck::select((void*)pSetRead, (void*)pSetWrite, (void*)pSetExcept, nullptr);
		}
		else
		{
			timeval t = { 0, long(microseconds) };
			return sck::select((void*)pSetRead, (void*)pSetWrite, (void*)pSetExcept, &t);
		}
	}
}