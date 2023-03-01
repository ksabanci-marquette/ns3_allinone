#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include <iostream>

#ifndef MYTAG_H
#define MYTAG_H

namespace ns3{

/**
 * \ingroup network
 * A simple example of an Tag implementation
 */
    class MyTag : public Tag
    {
    public:

        MyTag();

        virtual ~MyTag();
        /**
         * \brief Get the type ID.
         * \return the object TypeId
         */
        static TypeId GetTypeId (void);
        virtual TypeId GetInstanceTypeId (void) const;
        virtual uint32_t GetSerializedSize (void) const;
        virtual void Serialize (TagBuffer i) const;
        virtual void Deserialize (TagBuffer i);
        virtual void Print (std::ostream &os) const;
        void SetPacketSizes (uint32_t * value);
        uint32_t * GetPacketSizes (void);

        void SetSimpleValue (uint8_t value);
//        void SetInitialTtl (uint8_t value);

        uint8_t GetSimpleValue (void) const;
//        uint8_t GetInitialTtl (void) const;

//        void SetTimestamp (Time time);
//        Time GetTimestamp (void) const;

    private:
        uint8_t ttl;
        //uint8_t initialTtl;
        uint32_t * m_packetSizes;
        //Time m_timestamp;
    };
}
#endif

