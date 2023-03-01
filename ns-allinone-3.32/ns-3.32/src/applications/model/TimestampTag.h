#include "ns3/tag.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include <iostream>

#ifndef NS_3_DEV_TIMESTAMPTAG_H
#define NS_3_DEV_TIMESTAMPTAG_H

namespace ns3{

/**
 * \ingroup network
 * A simple example of an Tag implementation
 */
class TimestampTag : public Tag {
public:
  TimestampTag();
  virtual ~TimestampTag();

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);

  // these are our accessors to our tag structure
  void SetTimestamp (Time time);
  Time GetTimestamp (void) const;

  void Print (std::ostream &os) const;

private:
  Time m_timestamp;

};
}

#endif //NS_3_DEV_TIMESTAMPTAG_H


