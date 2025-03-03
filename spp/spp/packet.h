#pragma once

#include <string.h>
#include <assert.h>
#include <bpacs/bpacs.hpp>
#include <ulib/types.h>
#include <limits.h>

namespace spp
{
	struct IPacketRules
	{
		virtual size_t GetHeaderSize() = 0;
		virtual size_t GetPacketSize(const void* pHeader) = 0;
		virtual bool IsValid(const void* pHeader) = 0;
		virtual size_t GetMaxPacketSize() = 0;

		inline bool IsHeaderSizeReached(size_t size) { return size >= GetHeaderSize(); }
		inline bool IsMaxPacketSizeOverlapped(size_t size) { return size > GetMaxPacketSize(); }
	};

	struct DefaultPacketHeader
	{
		uint size;
		uint type;
	};

	class DefaultPacketRules : public IPacketRules
	{
	public:
		size_t GetHeaderSize()
		{
			return size_t(sizeof(DefaultPacketHeader));
		}

		size_t GetPacketSize(const void* pHeader)
		{
			const DefaultPacketHeader* pPacketHeader = reinterpret_cast<const DefaultPacketHeader*>(pHeader);
			return pPacketHeader->size;
		}

		bool IsValid(const void* pHeader)
		{
			const DefaultPacketHeader* pPacketHeader = reinterpret_cast<const DefaultPacketHeader*>(pHeader);
			return pPacketHeader->size <= 0x10000;
		}

		size_t GetMaxPacketSize()
		{
			return 0x10000;
		}
	};

	class PacketSizeLimits
	{
	public:
		PacketSizeLimits();
		~PacketSizeLimits();

		PacketSizeLimits(const PacketSizeLimits&) = delete;
		PacketSizeLimits(PacketSizeLimits&&) = delete;

		uint GetLimit(uint idx) const;
		void SetLimit(uint idx, uint size);

	private:
		uint* mData;
		uint mCapacity;
	};

	/*

	class DefaultPacketRules
	{
	public:
		void CompleteHeader(DefaultPacketHeader& header, size_t contentSize) { header.size = contentSize + sizeof(DefaultPacketHeader); }
		uint ExtractSize(const DefaultPacketHeader& header) { return header.size; }
		bool IsPacketValid(const DefaultPacketHeader& header) { return header.size >= sizeof(DefaultPacketHeader); }

	private:

	};
	*/
	/*
	struct IPacketRules
	{
		virtual ~IPacketRules() {}
		virtual bool IsPacketValid(const spp::PacketHeader& header) = 0;
	};

	class PacketRuleable
	{
	public:
		PacketRuleable() : mPacketRules(nullptr) {}

		inline void SetPacketRules(IPacketRules* pPacketRules) { mPacketRules = pPacketRules; }
		inline IPacketRules* GetPacketRules() { return mPacketRules; }
	protected:
		IPacketRules* mPacketRules;
	};

	class DefaultPacketRules : public IPacketRules
	{
	public:
		bool IsPacketValid(const spp::PacketHeader& header) { return true; }
	};

	class LimitsPacketRules : public IPacketRules
	{
	public:
		void SetPacketSizeLimit(uint idx, uint size) { mLimits.SetLimit(idx, size); }
		bool IsPacketValid(const spp::PacketHeader& header) { return mLimits.GetLimit(header.type) >= header.size; }
	private:
		PacketSizeLimits mLimits;
	};
	*/

	class Buffer
	{
	public:
		inline Buffer(size_t capacity)
		{
			mBuffer = new uchar[capacity];
			assert(mBuffer != nullptr);

			mCapacity = capacity;
			mSize = 0;
		}

		inline ~Buffer()
		{
			delete[] mBuffer;
		}

		inline size_t Size() { return mSize; }
		inline size_t FreeSpaceSize() { return mCapacity - mSize; }
		inline size_t Capacity() { return mCapacity; }

		inline uchar* End() { return mBuffer + mSize; }
		inline void AddSize(size_t size) 
		{
			mSize += size; 

			assert(mSize <= mCapacity);
			assert(mSize < 0x07ffffff); // 128mb
		}


		void ResetCapacity(size_t newCapacity)
		{
			assert(mSize <= newCapacity);
			assert(newCapacity < 0x07ffffff); // 128mb

			uchar* newPtr = new uchar[newCapacity];
			assert(newPtr != nullptr);

			memcpy(newPtr, mBuffer, mSize);
			delete[] mBuffer;

			mBuffer = newPtr;
			mCapacity = newCapacity;			
		}

		void AcquireCapacity(size_t necessaryCapacity)
		{
			if (necessaryCapacity > mCapacity)
				ResetCapacity(necessaryCapacity);

			/*
			size_t freeSpaceSize = FreeSpaceSize();
			if (necessarySpaceSize > freeSpaceSize)
				ResetCapacity(mSize + necessarySpaceSize);
			*/
		}

		void AcquireFreeSpace(size_t necessaryFreeSpaceSize)
		{
			size_t necessaryCapacity = mSize + necessaryFreeSpaceSize;
			if (necessaryCapacity > mCapacity)
				ResetCapacity(necessaryCapacity);

			/*
			size_t freeSpaceSize = FreeSpaceSize();
			if (necessarySpaceSize > freeSpaceSize)
				ResetCapacity(mSize + necessarySpaceSize);
			*/
		}

		void Write(const void* pData, size_t size)
		{
			AcquireFreeSpace(size);

			memcpy(mBuffer + mSize, pData, size);
			mSize += size;
		}

		inline const uchar* Data() const { return mBuffer; }
		inline uchar* Data() { return mBuffer; }

		// erase data from begin to begin + length
		inline void MoveLeft(size_t length)
		{
			assert(mSize >= length);

			mSize -= length;
			memmove(mBuffer, mBuffer + length, mSize);
		}

		inline void Clear() { mSize = 0; }

	protected:
		uchar* mBuffer;
		size_t mSize;
		size_t mCapacity;
	};

	class BufferQueue
	{
	public:
		BufferQueue(Buffer* pBuffer)
		{
			mBuffer = pBuffer;
			mIndex = 0;
		}

		~BufferQueue()
		{

		}

		uchar* CurrentData() { return mBuffer->Data() + mIndex; }
		void Step(size_t count) { mIndex += count; }
		size_t Left() 
		{ 
			assert(mBuffer->Size() >= mIndex);
			return mBuffer->Size() - mIndex;
		}

		size_t GetIndex() { return mIndex; }
		void ResetIndex() { mIndex = 0; }


	private:
		Buffer* mBuffer;
		size_t mIndex;
	};

	class QueueBuffer : public Buffer
	{
	public:
		QueueBuffer(size_t capacity)
			: Buffer(capacity)
		{

		}

		uchar* CurrentData() { return mBuffer + mIndex; }
		void StepIndex(size_t count) { mIndex += count; }
		void ResetQueue()
		{
			MoveLeft(mSize - mIndex);
			mIndex = 0;
		}

		size_t CurrentSize()
		{
			assert(mSize >= mIndex);
			return mSize - mIndex; 
		}

	private:
		size_t mIndex;
	};

	class SendingBuffer : public Buffer
	{
	public:
		SendingBuffer(uint capacity)
			: Buffer(capacity)
		{

		}

		void Acquire(size_t size) { AcquireFreeSpace(size); }

		inline const uchar* Data() const { return mBuffer; }
		inline uchar* Data() { return mBuffer; }

		inline void Move(size_t length)
		{
			MoveLeft(length);
		}

		inline uchar* Get(size_t size)
		{
			AcquireFreeSpace(size);
			uchar* data = mBuffer + mSize;
			mSize += size;
			return data;
		}

		inline void Step(size_t length)
		{
			Acquire(length);
			mSize += length;
		}
	};

	class PacketWrite
	{
	public:
		inline PacketWrite(SendingBuffer* pBuffer, size_t headerSize)
			: mBuffer(pBuffer), mHeaderSize(pBuffer->Size())
		{
			mBuffer->Step(headerSize);
		}

		inline void Write(const void* data, size_t size)
		{
			assert(size < INT_MAX);

			mBuffer->Write(data, size);
		}

		template<class T>
		inline void Write(const T& data) { Write(&data, sizeof(T)); }

		inline uchar* Get(size_t size)
		{
			assert(size < INT_MAX);

			return mBuffer->Get(size);
		}

		template<class T>
		inline T* Get() { return (T*)Get(sizeof(T)); }

		template<class CharT = char>
		inline void WriteString(const CharT* sz)
		{
			size_t s = sizeof(CharT);

			const CharT* i = sz;
			while (*i) { s += sizeof(CharT); i++; } // strlen<T>(sz) + t

			mBuffer->Write(sz, s);
		}

		inline uchar* Data() { return mBuffer->Data() + mHeaderSize; }
		inline const uchar* Data() const { return mBuffer->Data() + mHeaderSize; }

		inline size_t RawSize() const { return mBuffer->Size() - mHeaderSize; }
		inline size_t HeaderSize() const { return mHeaderSize; }

	private:
		SendingBuffer* mBuffer;
		size_t mHeaderSize;
	};

	class Packet
	{
	public:
		Packet(size_t headerSize);
		Packet(const Packet& pac);
		~Packet();

		void operator=(const Packet& pac);
		void Write(const void* data, size_t size);

		template<class CharT = char>
		inline void WriteString(const CharT* sz)
		{
			size_t s = sizeof(CharT);

			const CharT* i = sz;
			while (*i) { s += sizeof(CharT); i++; } // strlen<T>(sz) + t

			ValidateData(s);
			memcpy((char*)(mData + mOffset), sz, s);
			StepWrite(s);
		}

		void Update(const void* pBuffer, size_t size);
		void ValidateData(size_t size);

		inline void Step(size_t length)
		{
			ValidateData(length);
			memset((char*)(mData + mOffset), 0, length);
			StepWrite(length);
		}

		inline void StepWrite(size_t length)
		{
			mOffset += length;
			assert(mOffset <= mAllocSize);
		}

		inline size_t RawSize() const { return mOffset; }
		inline size_t Size() const { return mOffset - mHeaderSize; }

		inline void Clear() { mOffset = mHeaderSize; }

		template<class T>
		inline void Write(T v) { Write(&v, sizeof(v)); }

		inline const uchar* RawData() const { return mData; }
		inline const uchar* Data() const { return mData + mHeaderSize; }

		inline size_t HeaderSize() const { return mHeaderSize; }
		inline size_t AllocSize() const { return mAllocSize; }

	protected:
		uchar* mData;
		size_t mAllocSize;
		size_t mHeaderSize;
		size_t mOffset;
	};

	class PacketView
	{
	public:
		PacketView() = default;
		PacketView(const uchar* pData, size_t size, size_t headerSize) : mData(pData), mOffset(headerSize), mSize(size) {}
		PacketView(const spp::PacketWrite& pac) : mData(pac.Data()), mOffset(pac.HeaderSize()), mSize(pac.RawSize()) {}
		PacketView(const Packet& pac) : mData(pac.RawData()), mOffset(pac.HeaderSize()), mSize(pac.RawSize()) {}

		inline size_t RawSize() const { return mSize; }
		inline const uchar* RawData() const { return mData; }

		template<class T>
		inline T* Read(size_t size)
		{
			assert(CanRead(size) && "Bad protocol in PacketView::Read<T>(uint size)");

			T* result = (T*)(mData + mOffset);
			mOffset += size;
			return result;
		}

		template<class T>
		inline const T& Read()
		{
			assert(CanRead(sizeof(T)) && "Bad protocol in PacketView::Read<T>()");

			T* result = (T*)(mData + mOffset);
			mOffset += sizeof(T);
			return *result;
		}

		inline void Read(void* buf, size_t size) { memcpy(buf, Read<void>(size), size); }

		inline bool CanRead(size_t length) { return mOffset + length <= mSize; }
		inline bool CanRead() { return mSize != mOffset; }

		template<class T>
		inline bool CanRead() { return mOffset + sizeof(T) <= mSize; }

		template<class CharT = char>
		const CharT* ReadString()
		{
			assert(CanReadString<CharT>() && "Bad protocol in PacketView::ReadString<CharT>()");

			const CharT* sz = (CharT*)(mData + mOffset);
			for (const CharT* p = sz; *p != 0; p++)
				mOffset += sizeof(CharT);
			mOffset += sizeof(CharT);

			return sz;
		}

		template<class CharT = char>
		bool CanReadString() const
		{
			for (size_t i = mOffset; i + (sizeof(CharT) - 1) < mSize; i += sizeof(CharT))
			{
				if (*(CharT*)(mData + i) == 0)
					return true;
			}

			return false;
		}

		template<class CharT = char>
		bool CanReadString(size_t maxElements) const
		{
			for (size_t i = mOffset; i + (sizeof(CharT) - 1) < mSize; i += sizeof(CharT))
			{
				CharT ch = *(CharT*)(mData + i);
				if (ch == 0)
					return true;

				maxElements--;
				if (maxElements == 0)
					return false;
			}

			return false;
		}

	protected:
		const uchar* mData;
		size_t mOffset;
		size_t mSize;
	};

}

