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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "my-tcp-echo-server.h"
#include "ns3/tag.h"
//#include "mytag.h"
#include "TimestampTag.h"
#include  "string.h"
#include <thread>
#include <iostream>
#include <chrono>
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include <thread>
#include <iostream>
#include <chrono>


namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("MyTcpEchoServerApplication");
    NS_OBJECT_ENSURE_REGISTERED (MyTcpEchoServer);

    TypeId
    MyTcpEchoServer::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::MyTcpEchoServer")
                .SetParent<Application> ()
                .AddConstructor<MyTcpEchoServer> ()
                .AddAttribute ("Local", "The Address on which to Bind the rx socket.",
                               AddressValue (),
                               MakeAddressAccessor (&MyTcpEchoServer::m_local),
                               MakeAddressChecker ())
                .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                               UintegerValue (9),
                               MakeUintegerAccessor (&MyTcpEchoServer::m_port),
                               MakeUintegerChecker<uint16_t> ())
                .AddAttribute ("ServerNo","ServerNo",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoServer::SetServerNo),
                               MakeUintegerChecker<uint8_t> ())
                .AddAttribute ("MSS","MSS",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoServer::SetMSS),
                               MakeUintegerChecker<uint32_t> ())
                .AddTraceSource ("Rx", "A packet has been received",
                                 MakeTraceSourceAccessor (&MyTcpEchoServer::m_rxTrace),
                                 "ns3::Packet::TracedCallback")
                .AddTraceSource ("RxWithAddresses", "A packet has been received",
                                 MakeTraceSourceAccessor (&MyTcpEchoServer::m_rxTraceWithAddresses),
                                 "ns3::Packet::TwoAddressTracedCallback")
        ;
        return tid;
    }

    MyTcpEchoServer::MyTcpEchoServer ()
    {
        NS_LOG_FUNCTION (this);
    }

    MyTcpEchoServer::~MyTcpEchoServer()
    {
        NS_LOG_FUNCTION (this);
        m_socket = 0;
    }

    void
    MyTcpEchoServer::DoDispose (void)
    {
        NS_LOG_FUNCTION (this);
        Application::DoDispose ();
    }

    void MyTcpEchoServer::SetPacketSizes(uint32_t * sizes){
        m_PacketSizes = sizes;
    }


    uint32_t * MyTcpEchoServer::GetPacketSizes(){

        return m_PacketSizes;
    }

    void MyTcpEchoServer::SetMSS(uint32_t mss){
        NS_LOG_FUNCTION (this<< mss);
        m_maxSegmentSize = mss;
    }

    void
    MyTcpEchoServer::SetServerNo(uint8_t serverNo)
    {
        NS_LOG_FUNCTION (this << "serverNo: "<< serverNo);
        m_serverNo = serverNo;
    }


    void
    MyTcpEchoServer::StartApplication (void)
    {
        startingIndex = 3;
        NS_LOG_FUNCTION (this);
        NS_LOG_FUNCTION ("m_local: " << m_local);
        NS_LOG_FUNCTION ("m_local" << m_port);
        NS_LOG_FUNCTION ("SERVER MSS" << m_maxSegmentSize);

        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (m_maxSegmentSize));
        Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));


        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
            m_socket = Socket::CreateSocket (GetNode (), tid);
            InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);

//            NS_LOG_FUNCTION ("InetSocketAddress local" << local.GetIpv4());

            if (m_socket->Bind (local) == -1)
            {
                NS_FATAL_ERROR ("Failed to bind socket");
            }

            m_socket->Listen();

//            NS_LOG_INFO("Echo Server local address:  " << m_local << " port: " << m_port << " bind: " );
        }

        m_socket->SetRecvPktInfo (true);

        m_socket->SetRecvCallback (
                MakeCallback (&MyTcpEchoServer::HandleRead, this));

        m_socket->SetAcceptCallback (
                MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
                MakeCallback (&MyTcpEchoServer::HandleAccept, this));

        m_socket->SetCloseCallbacks(
                MakeCallback(&MyTcpEchoServer::HandleClose, this),
                MakeCallback(&MyTcpEchoServer::HandleClose, this));
    }

    void MyTcpEchoServer::HandleClose(Ptr<Socket> s1)
    {
        NS_LOG_INFO(this);
    }

    void MyTcpEchoServer::HandleAccept (Ptr<Socket> s, const Address& from)
    {
        NS_LOG_FUNCTION (this << s << from);
        //NS_LOG_INFO("ACCEPT IN ECHO SERVER from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
        s->SetRecvCallback (MakeCallback (&MyTcpEchoServer::HandleRead, this));

    }

    void
    MyTcpEchoServer::StopApplication ()
    {
        NS_LOG_FUNCTION (this);

        if (m_socket != 0)
        {
            m_socket->Close ();
            m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
        }
    }


    void
    MyTcpEchoServer::HandleRead (Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION(this << socket);

        Ptr<Packet> packet;
        Address from;
        Address localAddress;

        uint8_t ttl = 0;

        NS_LOG_INFO("--INSIDE MyTcpEchoServer::HandleRead , startingIndex IS : "<< startingIndex << " for client " << +m_serverNo );

        while (packet = socket->RecvFrom (from))
        {
            uint8_t *msg;
            socket->GetSockName(localAddress);

            m_rxTrace(packet);
            m_rxTraceWithAddresses(packet, from, localAddress);
            uint32_t packetSize = packet->GetSize ();

            if (InetSocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time "
                                    << Simulator::Now().As(Time::S) << " server " <<  m_serverNo <<" received "
                                    << packet->GetSize() << " bytes from "
                                    << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                    << InetSocketAddress::ConvertFrom(from).GetPort());
            } else if (Inet6SocketAddress::IsMatchingType(from)) {
                NS_LOG_INFO("At time "
                                    << Simulator::Now().As(Time::S) << " server " <<  m_serverNo <<" received "
                                    << packet->GetSize() << " bytes from "
                                    << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                    << Inet6SocketAddress::ConvertFrom(from).GetPort());
            }

            uint32_t *arr = GetPacketSizes();
            uint32_t expectedSize = arr[4-(startingIndex +1)];

            totalPacketSize = totalPacketSize + packetSize;

            if(totalPacketSize >= expectedSize){

                uint32_t newSize = arr[4-(startingIndex)];

                if (newSize < packetSize){

                    packet->RemoveAtEnd2 ( abs(packetSize - newSize) );

                }else if (newSize > packetSize){

                    packet->AddPaddingAtEnd( abs(packetSize - newSize) );

                }else{

                }
                socket->SendTo(packet, 0, from);
                totalPacketSize = 0;
                startingIndex = startingIndex - 2;
                NS_LOG_INFO("startingIndex updated to: "<< startingIndex << " for server " << m_serverNo << " on port "<< InetSocketAddress::ConvertFrom(from).GetPort() );

                if (InetSocketAddress::IsMatchingType(from)) {
                    NS_LOG_INFO("At time "
                                        << Simulator::Now().As(Time::S) << " server " <<  m_serverNo <<" sent "
                                        << packet->GetSize() << " bytes from "
                                        << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                        << InetSocketAddress::ConvertFrom(from).GetPort());
                } else if (Inet6SocketAddress::IsMatchingType(from)) {
                    NS_LOG_INFO("At time "
                                        << Simulator::Now().As(Time::S) << " server " <<  m_serverNo <<" sent "
                                        << packet->GetSize() << " bytes from "
                                        << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                        << Inet6SocketAddress::ConvertFrom(from).GetPort());
                }

            }

        }
    }

} // Namespace ns3
