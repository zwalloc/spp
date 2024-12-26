#include "packet.h"

#include <string.h>

#pragma warning(disable : 4996)

namespace spp
{
	PacketSizeLimits::PacketSizeLimits()
	{
		mData = new uint[128];
		mCapacity = 128;
		memset(mData, 0, sizeof(*mData) * 128);
	}

	PacketSizeLimits::~PacketSizeLimits()
	{
		delete[] mData;
	}

	uint PacketSizeLimits::GetLimit(uint idx) const
	{
		if (idx >= mCapacity)
			return 0;
		return mData[idx];
	}

	void spp::PacketSizeLimits::SetLimit(uint idx, uint size)
	{
		assert(idx <= 4092 && "Too big packet size limit");

		if (idx >= mCapacity)
		{
			uint newCapacity = mCapacity + idx;

			uint* pOldData = mData;
			mData = new uint[newCapacity];

			memcpy(mData, pOldData, mCapacity);
			memset(mData + mCapacity, 0, newCapacity - mCapacity);
		}

		mData[idx] = size;
	}

	Packet::Packet(size_t headerSize)
	{
		const size_t ALLOC_SIZE = headerSize + 128;

		mData = new uchar[ALLOC_SIZE];
		mAllocSize = ALLOC_SIZE;

		mOffset = headerSize;
		mHeaderSize = headerSize;
	}

	Packet::Packet(const Packet& pac)
	{
		mData = (uchar*)new uchar[pac.RawSize()];
		mAllocSize = pac.RawSize();

		mOffset = pac.mOffset;
		memcpy(mData, pac.RawData(), pac.RawSize());
	}

	Packet::~Packet()
	{
		delete[] mData;
	}

	void spp::Packet::operator=(const Packet& pac)
	{
		ValidateData(pac.RawSize());
		mOffset = pac.mOffset;
		memcpy(mData, pac.mData, pac.RawSize());
	}

	void spp::Packet::Write(const void* data, size_t size)
	{
		ValidateData(size);
		memcpy((char*)(mData + mOffset), data, size);
		StepWrite(size);
	}

	void spp::Packet::Update(const void* pBuffer, size_t size)
	{
		if (size > mAllocSize)
		{
			delete[] mData;
			mData = new uchar[size];
		}

		memcpy(mData, pBuffer, size);
	}

	void spp::Packet::ValidateData(size_t size)
	{
		if (mAllocSize < mOffset + size)
		{
			size_t newAllocSize = mOffset + size + mAllocSize;
			uchar* newPtr = new uchar[newAllocSize];
			memcpy(newPtr, mData, mOffset);
			delete[] mData;
			mData = newPtr;
			mAllocSize = newAllocSize;
		}
	}
}

