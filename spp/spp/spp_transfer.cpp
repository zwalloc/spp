#include "transfer.h"
#include <string.h>

namespace spp
{
    namespace tcp
    {
        class IoService
        {
        public:
        private:
        };

        class Extractor
        {
        public:
            Extractor(const void *data, size_t size)
            {
                mBegin = (uchar *)data;
                mIt = mIt;
                mEnd = mBegin + size;
            }

            ~Extractor() {}

            size_t Read() {}

        private:
            const uchar *mIt;
            const uchar *mBegin;
            const uchar *mEnd;

            IPacketRules *mPacketRules;
        };

        Transfer::Transfer(IPacketRules *pPacketRules)
            : mSendingBuffer(MAX_PACKET_SIZE), mReceiveBuffer(MAX_PACKET_SIZE), mPacketRules(pPacketRules),
              mBufferQueue(&mReceiveBuffer)
        {
            mCryptor = nullptr;

            size_t headerSize = mPacketRules->GetHeaderSize();

            // for headers with size bigger than 0x10000
            if (mReceiveBuffer.Capacity() < headerSize)
                mReceiveBuffer.AcquireFreeSpace(headerSize);
        }

        Transfer::Transfer(sck::tcp::Socket sock, IPacketRules *pPacketRules)
            : mSocket(sock), mSendingBuffer(MAX_PACKET_SIZE), mReceiveBuffer(MAX_PACKET_SIZE),
              mPacketRules(pPacketRules), mBufferQueue(&mReceiveBuffer)
        {
            mCryptor = nullptr;

            size_t headerSize = mPacketRules->GetHeaderSize();

            // for headers with size bigger than 0x10000
            if (mReceiveBuffer.Capacity() < headerSize)
                mReceiveBuffer.AcquireFreeSpace(headerSize);
        }

        Transfer::~Transfer()
        {
            if (mSocket.IsOpen())
                mSocket.Close();
        }

        void Transfer::Close()
        {
            if (mSocket.IsOpen())
                mSocket.Close();

            mReceiveBuffer.Clear();
            mSendingBuffer.Clear();
            mBufferQueue.ResetIndex();
        }

        bool Transfer::ProcessSendFull()
        {
            while (mSendingBuffer.Size())
            {
                switch (ProcessSend())
                {
                case ProcessSendResult::InternalError:
                    return false;
                default:
                    break;
                }
            }

            return true;
        }

        ProcessSendResult Transfer::ProcessSend()
        {
            if (!mSendingBuffer.Size())
                return ProcessSendResult::SendingBufferIsEmpty;

			if (mCryptor)
				mCryptor->Encrypt(mSendingBuffer.Data(), mSendingBuffer.Size());

            switch (int sended = mSocket.Send(mSendingBuffer.Data(), int(mSendingBuffer.Size())))
            {
            case 0:
                return ProcessSendResult::NoBytesSended;
            case -1:
                return ProcessSendResult::InternalError;
            default:
				if (mCryptor)
				{
					size_t size = size_t(sended);
					if (size != mSendingBuffer.Size())
						mCryptor->Decrypt(mSendingBuffer.Data() + size, mSendingBuffer.Size() - size);
				}

                mSendingBuffer.Move(sended);
                return ProcessSendResult::Success;
            }
        }

        RecvFullResult Transfer::RecvFull(PacketView &out)
        {
            while (true)
            {
                UpdatePacketResult result = UpdatePacket(out);
                if (result == UpdatePacketResult::NoData)
                {
                    switch (Receive())
                    {
                    case ReceiveResult::ConnectionClosed:
                        return RecvFullResult::ConnectionClosed;
                    case ReceiveResult::InternalError:
                        return RecvFullResult::InternalError;
                    default:
                        break;
                    }
                }
                else
                {
                    switch (result)
                    {
                    case UpdatePacketResult::OversizedPacket:
                        return RecvFullResult::OversizedPacket;
                    default:
                        return RecvFullResult::Success;
                    }
                }
            }
        }

        ReceiveResult Transfer::Receive()
        {
            assert(mBufferQueue.GetIndex() == 0);

            const size_t recvSize = mReceiveBuffer.FreeSpaceSize();
            assert(recvSize <= INT_MAX);

            switch (int r = mSocket.Recv(mReceiveBuffer.End(), int(recvSize)))
            {
            case 0:
                return ReceiveResult::ConnectionClosed;
            case -1:
                return ReceiveResult::InternalError;
            default:

                if (mCryptor)
                    mCryptor->Decrypt(mReceiveBuffer.End(), recvSize);

                mReceiveBuffer.AddSize(r);
                return ReceiveResult::Success;
            }
        }

        UpdatePacketResult Transfer::UpdatePacket(PacketView &out)
        {
            size_t headerSize = mPacketRules->GetHeaderSize();
            size_t left = mBufferQueue.Left();
            if (left >= headerSize)
            {
                uchar *data = mBufferQueue.CurrentData();
                size_t packetSize = mPacketRules->GetPacketSize(data);

                if (packetSize > mPacketRules->GetMaxPacketSize() || !packetSize || packetSize < headerSize ||
                    !mPacketRules->IsValid(data))
                {
                    return UpdatePacketResult::OversizedPacket;
                }

                if (left >= packetSize)
                {
                    out = PacketView(data, packetSize, headerSize);
                    mBufferQueue.Step(packetSize);

                    return UpdatePacketResult::Success;
                }
                else
                {
                    mReceiveBuffer.AcquireCapacity(packetSize);
                }
            }

            if (mBufferQueue.GetIndex())
            {
                mReceiveBuffer.MoveLeft(mBufferQueue.GetIndex());
                mBufferQueue.ResetIndex();
            }

            return UpdatePacketResult::NoData;
        }
    } // namespace tcp

    namespace udp
    {
        Transfer::Transfer(IPacketRules *pPacketRules)
            : mReceiveBuffer(new uchar[MAX_PACKET_SIZE * 2]), mPacketRules(pPacketRules)
        {
            mDataLeft = 0;
        }

        Transfer::Transfer(sck::udp::Socket sock, IPacketRules *pPacketRules)
            : mReceiveBuffer(new uchar[MAX_PACKET_SIZE * 2]), mSocket(sock), mPacketRules(pPacketRules)
        {
            mDataLeft = 0;
        }

        Transfer::~Transfer()
        {
            if (mSocket.IsOpen())
                mSocket.Close();
            delete[] mReceiveBuffer;
        }

        bool Transfer::Open()
        {
            if (mSocket.IsOpen())
                return true;
            if (!mSocket.Open())
                return false;

            return true;
        }

        void Transfer::ReceiveDataSnapshot() { mDataLeft = mSocket.Available(); }

        ReceiveResult Transfer::ReceivePacket(PacketView &out, sck::ip::v4::Endpoint &endpoint)
        {
            if (!mDataLeft)
                return ReceiveResult::NoDataAvailable;

            int r = mSocket.Recv(mReceiveBuffer, MAX_PACKET_SIZE, &endpoint);
            if (r > 0)
            {
                if (r >= mPacketRules->GetHeaderSize())
                {
                    size_t packetSize = mPacketRules->GetPacketSize(mReceiveBuffer);
                    if (packetSize != size_t(r))
                        return ReceiveResult::BadProtocolDetected; // bad protocol

                    mDataLeft -= r;

                    new (&out) PacketView(mReceiveBuffer, packetSize, mPacketRules->GetHeaderSize());
                    return ReceiveResult::Success;
                }
                else
                {
                    return ReceiveResult::BadProtocolDetected; // bad protocol
                }
            }
            else if (r == -1) // stupid error
            {
                return ReceiveResult::InternalError;
            }
            else
            {
                return ReceiveResult::ConnectionClosed;
            }
        }
    } // namespace udp

    struct ProtocolOperation
    {
        uchar *data;
        size_t dataSize;
        uint proto;
    };

    struct ProtocolOperationResult
    {
        size_t readenBytes;
        size_t requiredSize;
    };

    struct IProtocolProcessor
    {
        virtual UpdatePacketResult Read(ProtocolOperation &op, ProtocolOperationResult &res);
    };

    class BinaryProtocolProcessor : public IProtocolProcessor
    {
    public:
        virtual UpdatePacketResult Read(ProtocolOperation &op, ProtocolOperationResult &res)
        {
            PacketView out;

            size_t headerSize = mPacketRules->GetHeaderSize();
            if (op.dataSize >= headerSize)
            {
                const uchar *data = op.data;
                size_t packetSize = mPacketRules->GetPacketSize(data);

                if (packetSize > mPacketRules->GetMaxPacketSize() || !packetSize || packetSize < headerSize ||
                    !mPacketRules->IsValid(data))
                {
                    return UpdatePacketResult::OversizedPacket;
                }

                if (op.dataSize >= packetSize)
                {
                    out = PacketView(data, packetSize, headerSize);
                    res.readenBytes = packetSize;

                    return UpdatePacketResult::Success;
                }
                else
                {
                    res.requiredSize = packetSize;
                }
            }

            return UpdatePacketResult::NoData;
        }

        virtual UpdatePacketResult Write(ProtocolOperation &op, ProtocolOperationResult &res) {}

    private:
        IPacketRules *mPacketRules;
    };

} // namespace spp