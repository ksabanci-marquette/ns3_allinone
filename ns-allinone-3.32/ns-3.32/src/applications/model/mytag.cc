#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include <iostream>
#include "mytag.h"


namespace ns3 {
    NS_LOG_COMPONENT_DEFINE ("MyTag");

    MyTag::MyTag(){
        NS_LOG_FUNCTION (this);
        ttl = 0;
    }

    MyTag::~MyTag(){
        NS_LOG_FUNCTION (this);
        ttl = 0;
    }

    TypeId MyTag::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::MyTag")
                .SetParent<Tag> ()
                .AddConstructor<MyTag> ()
                .AddAttribute ("SimpleValue",
                               "A simple value",
                               EmptyAttributeValue (),
                               MakeUintegerAccessor (&MyTag::GetSimpleValue),
                               MakeUintegerChecker<uint8_t> ())
        ;
        return tid;
    }

    TypeId MyTag::GetInstanceTypeId (void) const
    {
        return GetTypeId ();
    }

    uint32_t MyTag::GetSerializedSize (void) const
    {
        return 1;
    }

    void MyTag::Serialize (TagBuffer i) const
    {
        i.WriteU8 (ttl);
    }

    void MyTag::Deserialize (TagBuffer i)
    {
        ttl = i.ReadU8 ();
    }

    void MyTag::Print (std::ostream &os) const
    {
        os << "v=" << (uint32_t)ttl;
    }

    void MyTag::SetSimpleValue (uint8_t value)
    {
        NS_LOG_FUNCTION (this);
        ttl = value;
        NS_LOG_FUNCTION ("ttl "<<ttl);

    }

    uint8_t MyTag::GetSimpleValue (void) const
    {
        NS_LOG_FUNCTION (this);
        return ttl;
    }

    void MyTag::SetPacketSizes (uint32_t * value)
    {
        NS_LOG_FUNCTION (this);
        m_packetSizes = value;
    }

    uint32_t * MyTag::GetPacketSizes (void)
    {
        NS_LOG_FUNCTION (this);
        return m_packetSizes;
    }

//    void MyTag::SetInitialTtl (uint8_t value)
//    {
//        NS_LOG_FUNCTION (this);
//        initialTtl = value;
//        NS_LOG_FUNCTION ("initialTtl "<<initialTtl);
//    }
//
//    uint8_t MyTag::GetInitialTtl (void) const
//    {
//        NS_LOG_FUNCTION (this);
//        return initialTtl;
//    }
//
//    void MyTag::SetTimestamp (Time time)
//    {
//        NS_LOG_FUNCTION (this);
//        m_timestamp = time;
//        NS_LOG_FUNCTION ("m_timestamp "<<m_timestamp);
//
//    }
//
//    Time MyTag::GetTimestamp (void) const
//    {
//        NS_LOG_FUNCTION (this);
//        return m_timestamp;
//    }
}

