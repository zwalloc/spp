#include <spp/client.h>
#include <spp/serviceprocessor.h>
#include <spp/schema.h>

#include <stdio.h>

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

int main()
{
    spp::sp_string<20> str;

    spp::ServiceProcessor sproc(sck::BlockingDelay_TillAction);

    EventProcessor evproc;
    spp::DefaultPacketRules rules;

    spp::tcp::Client cl(&evproc, &rules);
    if(cl.Connect(sck::ip::v4::Endpoint(sck::ip::v4::Addr(127,0,0,1), 1703)))
    {
        printf("succefully connected\n");
    }
    else
    {
        printf("connect failed\n");
        return 0;
    }

    sproc.Process(&cl);

    return 0;
}