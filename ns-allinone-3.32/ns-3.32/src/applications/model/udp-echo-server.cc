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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include  "string.h"
#include "udp-echo-server.h"
#include "ns3/udp-header.h"
#include "ns3/tag.h"
#include "mytag.h"
#include "TimestampTag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpEchoServerApplication");

NS_OBJECT_ENSURE_REGISTERED (UdpEchoServer);

TypeId
UdpEchoServer::GetTypeId (void)
{
 static TypeId tid = TypeId ("ns3::UdpEchoServer")
                         .SetParent<Application> ()
                         .SetGroupName("Applications")
                         .AddConstructor<UdpEchoServer> ()
                         .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                                        UintegerValue (9),
                                        MakeUintegerAccessor (&UdpEchoServer::m_port),
                                        MakeUintegerChecker<uint16_t> ())
                         .AddTraceSource ("Rx", "A packet has been received",
                                          MakeTraceSourceAccessor (&UdpEchoServer::m_rxTrace),
                                          "ns3::Packet::TracedCallback")
                         .AddTraceSource ("RxWithAddresses", "A packet has been received",
                                          MakeTraceSourceAccessor (&UdpEchoServer::m_rxTraceWithAddresses),
                                          "ns3::Packet::TwoAddressTracedCallback")
     ;
 return tid;
}

UdpEchoServer::UdpEchoServer ()
{
 NS_LOG_FUNCTION (this);
}

UdpEchoServer::~UdpEchoServer()
{
 NS_LOG_FUNCTION (this);
 m_socket = 0;
 m_socket6 = 0;
}

void
UdpEchoServer::DoDispose (void)
{
 NS_LOG_FUNCTION (this);
 Application::DoDispose ();
}

void UdpEchoServer::SetPacketSizes(uint32_t * sizes){
  m_PacketSizes = sizes;
}


uint32_t * UdpEchoServer::GetPacketSizes(){

  return m_PacketSizes;
}


uint32_t UdpEchoServer::returnElement(uint32_t *arr, int length, int index){
  for (int i=0;i<length;i++){
      if(i==index){
          return arr[index];
        }
    }
  return -1;
}


void
UdpEchoServer::StartApplication (void)
{
 NS_LOG_FUNCTION (this);

 if (m_socket == 0)
   {
     TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
     m_socket = Socket::CreateSocket (GetNode (), tid);
     InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
     NS_LOG_FUNCTION ("InetSocketAddress local" << local.GetIpv4());

     if (m_socket->Bind (local) == -1)
       {
         NS_FATAL_ERROR ("Failed to bind socket");
       }
     if (addressUtils::IsMulticast (m_local))
       {
         Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
         if (udpSocket)
           {
             // equivalent to setsockopt (MCAST_JOIN_GROUP)
             udpSocket->MulticastJoinGroup (0, m_local);
           }
         else
           {
             NS_FATAL_ERROR ("Error: Failed to join multicast group");
           }
       }
   }

 if (m_socket6 == 0)
   {
     TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
     m_socket6 = Socket::CreateSocket (GetNode (), tid);
     Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
     if (m_socket6->Bind (local6) == -1)
       {
         NS_FATAL_ERROR ("Failed to bind socket");
       }
     if (addressUtils::IsMulticast (local6))
       {
         Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
         if (udpSocket)
           {
             // equivalent to setsockopt (MCAST_JOIN_GROUP)
             udpSocket->MulticastJoinGroup (0, local6);
           }
         else
           {
             NS_FATAL_ERROR ("Error: Failed to join multicast group");
           }
       }
   }

 m_socket->SetRecvCallback (MakeCallback (&UdpEchoServer::HandleRead, this));
 m_socket6->SetRecvCallback (MakeCallback (&UdpEchoServer::HandleRead, this));
}

void
UdpEchoServer::StopApplication ()
{
 NS_LOG_FUNCTION (this);

 if (m_socket != 0)
   {
     m_socket->Close ();
     m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
   }
 if (m_socket6 != 0)
   {
     m_socket6->Close ();
     m_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
   }
}

void
UdpEchoServer::HandleRead (Ptr<Socket> socket) {
 NS_LOG_FUNCTION(this << socket);

 Ptr<Packet> packet;
 Address from;
 Address localAddress;
 while ((packet = socket->RecvFrom(from))) {
     socket->GetSockName(localAddress);
     m_rxTrace(packet);
     m_rxTraceWithAddresses(packet, from, localAddress);
     if (InetSocketAddress::IsMatchingType(from)) {
         NS_LOG_INFO("At time "
                      << Simulator::Now().As(Time::S) << " server received "
                      << packet->GetSize() << " bytes from "
                      << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                      << InetSocketAddress::ConvertFrom(from).GetPort());
       } else if (Inet6SocketAddress::IsMatchingType(from)) {
         NS_LOG_INFO("At time "
                      << Simulator::Now().As(Time::S) << " server received "
                      << packet->GetSize() << " bytes from "
                      << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                      << Inet6SocketAddress::ConvertFrom(from).GetPort());
       }

     // packet->RemoveAllPacketTags ();
     // packet->RemoveAllByteTags ();

     // packet->PrintPacketTags(std::cout);
     /*
   NS_LOG_LOGIC ("Echoing packet");
   uint8_t *buffer = new uint8_t[packet->GetSize ()];
   packet->CopyData(buffer, packet->GetSize ());
   char temp[packet->GetSize()];
   memcpy(temp,buffer,packet->GetSize());
   NS_LOG_LOGIC("Received:"<<temp);
   char * pos;
   int cntr;
   pos=std::strchr(temp,':');

   if (pos != NULL) {
     cntr = temp[pos-temp+1] - '0';
           NS_LOG_LOGIC("cntr:"<<cntr);

   }
   */
     uint8_t ttl = 0;
     MyTag tag;
     bool found = packet->RemovePacketTag(tag);

     if (!found) {
         NS_LOG_LOGIC("cannot read tag and ttl");
         return;
       }

     ttl = tag.GetSimpleValue();
     NS_LOG_LOGIC("ttl:" << +ttl);

     if (+ttl == 1) {
         NS_LOG_LOGIC("ttl is 1 server aborting packet cycle!");
         TimestampTag timestampTag;
         bool foundTimeStampTag =packet->RemovePacketTag(timestampTag);
         if (foundTimeStampTag){
            NS_LOG_LOGIC("delay: " << Simulator::Now().GetNanoSeconds() - timestampTag.GetTimestamp().GetNanoSeconds ());
              NS_LOG_LOGIC("not delay: " << timestampTag.GetTimestamp().GetNanoSeconds () - timestampTag.GetTimestamp().GetNanoSeconds ());

           }else{
		NS_LOG_LOGIC("cannot find timestamp tag!");
	}

         return;
       }

     //packet->RemoveAllPacketTags();
     tag.SetSimpleValue(--ttl);

     ////////
     uint32_t *arr = GetPacketSizes();
     uint32_t newSize = returnElement(arr, 10, 10-(+ttl));
     uint32_t packetSize = packet->GetSize ();
     NS_LOG_LOGIC("newSize ::"<< newSize);
     NS_LOG_LOGIC("old packetsize ::"<< packetSize);

     if (newSize < packetSize){
       
	 NS_LOG_LOGIC("packet->RemoveAtEnd( newSize - packet->GetSize () );");
         packet->RemoveAtEnd2( abs(newSize - packetSize) );
       
     }else if (newSize > packetSize){
         
	 NS_LOG_LOGIC("packet->AddPaddingAtEnd(packet->GetSize () - newSize );");
         packet->AddPaddingAtEnd( abs(newSize - packetSize) );

       }else{
    
	  NS_LOG_LOGIC(" packet size does not change");
       }
     //////

     NS_LOG_LOGIC("updated packet size ::"<< packet->GetSize ());
     packet->RemoveAllByteTags();
     packet->RemoveAllPacketTags();

     packet->AddPacketTag(tag);
     socket->SendTo(packet, 0, from);

     if (InetSocketAddress::IsMatchingType(from)) {
         NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " server sent "
                                 << " from "<< InetSocketAddress::ConvertFrom(localAddress).GetIpv4() << " "
                                 << packet->GetSize() << " bytes to "
                                 << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                 << " port "
                                 << InetSocketAddress::ConvertFrom(from).GetPort());
       } else if (Inet6SocketAddress::IsMatchingType(from)) {
         NS_LOG_INFO("At time "
                      << Simulator::Now().As(Time::S) << " server sent "
                      << packet->GetSize() << " bytes to "
                      << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                      << Inet6SocketAddress::ConvertFrom(from).GetPort());
       }
   }
}
} // Namespace ns3

