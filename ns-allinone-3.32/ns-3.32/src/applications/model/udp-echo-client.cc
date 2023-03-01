/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "udp-echo-client.h"
#include "mytag.h"
#include "TimestampTag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpEchoClientApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpEchoClient);

TypeId
UdpEchoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpEchoClient")
                          .SetParent<Application> ()
                          .SetGroupName("Applications")
                          .AddConstructor<UdpEchoClient> ()
                          .AddAttribute ("MaxPackets",
                                         "The maximum number of packets the application will send",
                                         UintegerValue (100),
                                         MakeUintegerAccessor (&UdpEchoClient::m_count),
                                         MakeUintegerChecker<uint32_t> ())
                          .AddAttribute ("Interval",
                                         "The time to wait between packets",
                                         TimeValue (Seconds (1.0)),
                                         MakeTimeAccessor (&UdpEchoClient::m_interval),
                                         MakeTimeChecker ())
                          .AddAttribute ("RemoteAddress",
                                         "The destination Address of the outbound packets",
                                         AddressValue (),
                                         MakeAddressAccessor (&UdpEchoClient::m_peerAddress),
                                         MakeAddressChecker ())
                          .AddAttribute ("RemotePort",
                                         "The destination port of the outbound packets",
                                         UintegerValue (0),
                                         MakeUintegerAccessor (&UdpEchoClient::m_peerPort),
                                         MakeUintegerChecker<uint16_t> ())
                          .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                                         UintegerValue (100),
                                         MakeUintegerAccessor (&UdpEchoClient::SetDataSize,
                                                               &UdpEchoClient::GetDataSize),
                                         MakeUintegerChecker<uint32_t> ())
                          .AddAttribute ("TagCounter","Number of RTTs",
                                         UintegerValue (0),
                                         MakeUintegerAccessor (&UdpEchoClient::SetTagCounter),
                                         MakeUintegerChecker<uint8_t> ())
                          .AddTraceSource ("Tx", "A new packet is created and is sent",
                                           MakeTraceSourceAccessor (&UdpEchoClient::m_txTrace),
                                           "ns3::Packet::TracedCallback")
                          .AddTraceSource ("Rx", "A packet has been received",
                                           MakeTraceSourceAccessor (&UdpEchoClient::m_rxTrace),
                                           "ns3::Packet::TracedCallback")
                          .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                                           MakeTraceSourceAccessor (&UdpEchoClient::m_txTraceWithAddresses),
                                           "ns3::Packet::TwoAddressTracedCallback")
                          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                                           MakeTraceSourceAccessor (&UdpEchoClient::m_rxTraceWithAddresses),
                                           "ns3::Packet::TwoAddressTracedCallback")
      ;
  return tid;
}

UdpEchoClient::UdpEchoClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_tagCounter = 0;
}

UdpEchoClient::~UdpEchoClient()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_tagCounter = 0;
}

void
UdpEchoClient::SetTagCounter (uint8_t tagCounter)
{
  NS_LOG_FUNCTION (this << "tag Counter "<< tagCounter);
  m_tagCounter = tagCounter;
}

void
UdpEchoClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
UdpEchoClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
UdpEchoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
UdpEchoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      NS_LOG_INFO ("Opened socket!");
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }
      NS_LOG_INFO ("Binded socket!");


  m_socket->SetRecvCallback (MakeCallback (&UdpEchoClient::HandleRead, this));
  m_socket->SetAllowBroadcast (true);
  NS_LOG_INFO ("Before ScheduleTransmit!");

  ScheduleTransmit (Seconds (0.));
 // Simulator::Cancel (m_sendEvent);
 // m_sendEvent = Simulator::ScheduleNow (&UdpEchoClient::ScheduleTransmit, this);
}

void
UdpEchoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
UdpEchoClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
UdpEchoClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void
UdpEchoClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
UdpEchoClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
UdpEchoClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void UdpEchoClient::SetPacketSizes(uint32_t * sizes){
  m_PacketSizes = sizes;
}


uint32_t * UdpEchoClient::GetPacketSizes(){

  return m_PacketSizes;
}

void UdpEchoClient::PrintPacketSizes(uint32_t * arr, int length){
  for (int i=0;i<length;i++){
      std::cout << arr << " value " << *arr << std::endl;
      arr++;
    }
}

void UdpEchoClient::deleteFirst(uint32_t * arr, int length, uint32_t * retArr){

  for (int i=0;i<length-1;i++){
      retArr[i] = arr[i+1];
    }
}

uint32_t UdpEchoClient::returnElement(uint32_t *arr, int length, int index){
  for (int i=0;i<length;i++){
      if(i==index){
          return arr[index];
        }
    }
  return -1;
}

void
UdpEchoClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &UdpEchoClient::Send, this);

}

void
UdpEchoClient::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  //Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "UdpEchoClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "UdpEchoClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  Address localAddress;
  m_socket->GetSockName (localAddress);
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (p, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (p, localAddress, Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
    }

  if(!m_tagCounter){
      NS_LOG_INFO ("m_tagCounter is null" );

    }else{
      MyTag tag;
      //TimestampTag timestampTag;
      tag.SetSimpleValue (m_tagCounter);
      tag.SetPacketSizes(m_PacketSizes);

//      int64_t creationtime = Simulator::Now().GetNanoSeconds();
//      timestampTag.SetTimestamp((Time)creationtime) ;

//      NS_LOG_INFO ("tag,getttl" << +tag.GetSimpleValue());
      p->AddPacketTag (tag);
      //p->AddPacketTag (timestampTag);
      p->PrintPacketTags(std::cout);
    }
  m_socket->Send (p);
  NS_LOG_INFO ("m_socket->Send (p);" );
  ++m_sent;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to " <<
                   Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to " <<
                   Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
    }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to " <<
                   InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to " <<
                   Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }

  if (m_sent < m_count)
    {
      ScheduleTransmit (m_interval);
    }
}

void
UdpEchoClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((p = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received " << p->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received " << p->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }

      Simulator::Cancel (m_sendEvent);

      socket->GetSockName (localAddress);
      m_rxTrace (p);
      m_rxTraceWithAddresses (p, from, localAddress);

      //PrintPacketSizes(GetPacketSizes(),10);

      uint8_t ttl = 0;
      MyTag tag;
      bool found = p->RemovePacketTag (tag);

      if (!found){
          NS_LOG_INFO("cannot read tag and ttl, aborting!");
          return;

        }

      ttl = tag.GetSimpleValue ();
      NS_LOG_INFO("ttl:"<<+ttl);

      /*
       * 1) who should initiate and who should end the cycle?  check the pcaps
       * A) add the same payload manipulation logic to the server side,
       * A') work with real numbers from the handshake
       * B) we start from the real length of the array, so abort at tll==1
       * C) decrease verbosity to understand what is going on
       * D) who is going to halt the conversation, client or server?
       *  so what is the best way to end an application in ns3?
       */


      if (+ttl == 1) {
          NS_LOG_INFO("ttl is 1 client aborting packet cycle!");

	  NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << p->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());

          TimestampTag timestampTag;
          bool foundTimeStampTag =p->RemovePacketTag(timestampTag);
          if (foundTimeStampTag){
		NS_LOG_INFO("delay: " << Simulator::Now().GetNanoSeconds() - timestampTag.GetTimestamp().GetNanoSeconds ());
            }
          else{
              NS_LOG_INFO("cannot find timestamp tag!");
            }
	  //DoDispose();
          return;
          // ??? or exit(0);
          //
        }

//      tag.SetSimpleValue(--ttl);

      uint32_t *arr = GetPacketSizes();
      uint32_t newSize = returnElement(arr, 10, 10-(+ttl));
      uint32_t packetSize = p->GetSize ();
      NS_LOG_LOGIC("newSize ::"<< newSize);
      NS_LOG_LOGIC("old packetsize ::"<< packetSize);
     
      if (newSize < packetSize){
          
          NS_LOG_INFO( abs(newSize - packetSize));
	  p->RemoveAtEnd2( abs(newSize - packetSize) );


       
      }else if (newSize > packetSize){
    
	  NS_LOG_LOGIC("packet->AddPaddingAtEnd(packet->GetSize () - newSize );");
          p->AddPaddingAtEnd( abs(newSize - packetSize) );

        }else{
         
	  NS_LOG_LOGIC(" packet size does not change");
        }


      NS_LOG_LOGIC("updated packet size ::"<< p->GetSize ());
      
      p->RemoveAllByteTags();
      p->RemoveAllPacketTags();

      p->AddPacketTag(tag);
      socket->SendTo (p, 0, from);

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << p->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << p->GetSize () << " bytes to " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }


    }
}

} // Namespace ns3


