
#include <gtest/gtest.h>

#include <spp/client.h>
#include <spp/serviceprocessor.h>
#include <sck/sck.h>

class EventProcessor : public spp::tcp::IEventProcessor
{
public:
    void Process(const spp::tcp::ClientEvent &event)
    {
        // if(event.type == spp::tcp::ClientEventType::TcpPacket)
        // {
        //     auto head = (spp::DefaultPacketHeader*)event.pac.RawData();
        //     printf("packet size: %d\n", int(head->size));
        // }
        // else
        // {
        //     printf("some event occurred\n");
        // }
    }

private:
};

TEST(Client, Connect)
{
    sck::Initializator init;

    spp::ServiceProcessor sproc(sck::BlockingDelay_TillAction);

    EventProcessor evproc;
    spp::DefaultPacketRules rules;

    spp::tcp::Client cl(&evproc, &rules);
    ASSERT_TRUE(cl.Connect(sck::ip::v4::Endpoint(sck::ip::v4::Addr(1, 1, 1, 1), 80)));
}