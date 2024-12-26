#pragma once

#include "packet.h"
#include <sck/sck.h>
#include <ulib/containers/queue.h>

namespace spp
{
    constexpr uint MAX_PACKET_SIZE = 65535;
    constexpr uint PACKET_SIZE_LIMIT = 10 * 1024 * 1024; // 1 mb

    enum class UpdatePacketResult
    {
        OversizedPacket = 0,
        NoData = 1,
        Success = 2,
    };

    struct ICryptor
    {
        virtual ~ICryptor() {};
        virtual void Encrypt(void *data, size_t size) = 0;
        virtual void Decrypt(void *data, size_t size) = 0;
    };

    namespace tcp
    {
        enum class ReceiveResult
        {
            Success,
            ConnectionClosed,
            InternalError,
        };

        enum class ProcessSendResult
        {
            Success,
            SendingBufferIsEmpty,
            NoBytesSended,
            InternalError
        };

        enum class RecvFullResult
        {
            Success,
            OversizedPacket,
            ConnectionClosed,
            InternalError
        };

        class Transfer
        {
        public:
            Transfer(IPacketRules *pPacketRules);
            Transfer(sck::tcp::Socket sock, IPacketRules *pPacketRules);

            Transfer(const Transfer &) = delete;
            ~Transfer();

            inline sck::tcp::Socket &GetSocket() { return mSocket; }

            inline void SetSocket(sck::tcp::Socket sock)
            {
                mSocket = sock;

                mBufferQueue.ResetIndex();
                mReceiveBuffer.Clear();
                mSendingBuffer.Clear();
            }

            bool Connect(const sck::ip::v4::Endpoint &ep)
            {
                if (mSocket.IsOpen())
                    return false;
                if (!mSocket.Open())
                    return false;
                if (!mSocket.Connect(ep))
                {
                    mSocket.Close();
                    return false;
                }

                return true;
            }
            void SetNonBlocking(bool b) { mSocket.SetNonBlocking(b); }
            void Close();
            bool IsOpen() { return mSocket.IsOpen(); }

            bool ProcessSendFull();

            inline void SetCryptor(ICryptor *pCryptor) { mCryptor = pCryptor; }
            inline ICryptor *GetCryptor() { return mCryptor; }

            inline size_t GetSendingSize() { return mSendingBuffer.Size(); }
            inline size_t GetReceivedSize() { return mReceiveBuffer.Size(); }
            inline PacketWrite Send() { return PacketWrite(&mSendingBuffer, mPacketRules->GetHeaderSize()); }
            inline void Send(PacketView pac) { mSendingBuffer.Write(pac.RawData(), pac.RawSize()); }

            ProcessSendResult ProcessSend();
            RecvFullResult RecvFull(PacketView &out);

            void ExpandReceiveBuffer(size_t newSize);
            ReceiveResult Receive();
            UpdatePacketResult UpdatePacket(PacketView &out);

        private:
            SendingBuffer mSendingBuffer;
            Buffer mReceiveBuffer;
            BufferQueue mBufferQueue;

            IPacketRules *mPacketRules;
            sck::tcp::Socket mSocket;

            ICryptor *mCryptor;
        };
    } // namespace tcp

    namespace udp
    {
        enum class ReceiveResult
        {
            Success,
            NoDataAvailable,
            BadProtocolDetected,
            InternalError,
            ConnectionClosed
        };

        class Transfer
        {
        public:
            Transfer(IPacketRules *pPacketRules);
            Transfer(const Transfer &) = delete;
            Transfer(sck::udp::Socket sock, IPacketRules *pPacketRules);
            ~Transfer();

            bool Open();
            inline void SetNonBlocking(bool b) { mSocket.SetNonBlocking(b); }
            inline void Close()
            {
                if (mSocket.IsOpen())
                    mSocket.Close();
            }
            inline bool IsOpen() { return mSocket.IsOpen(); }

            inline void SetSocket(sck::udp::Socket sock) { mSocket = sock; }
            inline sck::udp::Socket &GetSocket() { return mSocket; }

            void ReceiveDataSnapshot();
            inline size_t GetDataLeft() { return mDataLeft; }
            ReceiveResult ReceivePacket(PacketView &out, sck::ip::v4::Endpoint &endpoint);

            inline int Send(PacketView pac, const sck::ip::v4::Endpoint &ep)
            {
                return mSocket.Send(pac.RawData(), int(pac.RawSize()), ep);
            }

        private:
            IPacketRules *mPacketRules;
            sck::udp::Socket mSocket;
            uchar *mReceiveBuffer;
            size_t mDataLeft;
            ulib::FastQueue<sck::ip::v4::Endpoint> mEndpointsQueue;
        };
    } // namespace udp

    inline const char *to_string(UpdatePacketResult v)
    {
        switch (v)
        {
        case UpdatePacketResult::Success:
            return "Success";
        case UpdatePacketResult::OversizedPacket:
            return "OversizedPacket";
        case UpdatePacketResult::NoData:
            return "NoData";
        }

        return "[error]";
    }

    inline const char *to_string(tcp::ReceiveResult v)
    {
        switch (v)
        {
        case tcp::ReceiveResult::Success:
            return "Success";
        case tcp::ReceiveResult::ConnectionClosed:
            return "ConnectionClosed";
        case tcp::ReceiveResult::InternalError:
            return "InternalError";
        }

        return "[error]";
    }
    inline const char *to_string(tcp::ProcessSendResult v)
    {
        switch (v)
        {
        case tcp::ProcessSendResult::Success:
            return "Success";
        case tcp::ProcessSendResult::SendingBufferIsEmpty:
            return "SendingBufferIsEmpty";
        case tcp::ProcessSendResult::NoBytesSended:
            return "NoBytesSended";
        case tcp::ProcessSendResult::InternalError:
            return "InternalError";
        }

        return "[error]";
    }
    inline const char *to_string(tcp::RecvFullResult v)
    {
        switch (v)
        {
        case tcp::RecvFullResult::Success:
            return "Success";
        case tcp::RecvFullResult::OversizedPacket:
            return "InternalError";
        case tcp::RecvFullResult::ConnectionClosed:
            return "ConnectionClosed";
        case tcp::RecvFullResult::InternalError:
            return "InternalError";
        }

        return "[error]";
    }

    inline const char *to_string(udp::ReceiveResult v)
    {
        switch (v)
        {
        case udp::ReceiveResult::Success:
            return "Success";
        case udp::ReceiveResult::NoDataAvailable:
            return "NoDataAvailable";
        case udp::ReceiveResult::BadProtocolDetected:
            return "BadProtocolDetected";
        case udp::ReceiveResult::InternalError:
            return "InternalError";
        }

        return "[error]";
    }
} // namespace spp