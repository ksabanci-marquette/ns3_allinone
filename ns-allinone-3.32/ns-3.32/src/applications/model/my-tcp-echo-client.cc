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
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "my-tcp-echo-client.h"
#include "ns3/tag.h"
//#include "mytag.h"
#include "TimestampTag.h"
#include  "string.h"
#include "ns3/config-store-module.h"
#include "ns3/core-module.h"
#include <thread>
#include <iostream>
#include <chrono>

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("MyTcpEchoClientApplication");
    NS_OBJECT_ENSURE_REGISTERED (MyTcpEchoClient);

    TypeId
    MyTcpEchoClient::GetTypeId (void)
    {
        static TypeId tid = TypeId ("ns3::MyTcpEchoClient")
                .SetParent<Application> ()
                .AddConstructor<MyTcpEchoClient> ()
                .AddAttribute ("MaxPackets",
                               "The maximum number of packets the application will send",
                               UintegerValue (100),
                               MakeUintegerAccessor (&MyTcpEchoClient::m_count),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("Interval",
                               "The time to wait between packets",
                               TimeValue (Seconds (1.0)),
                               MakeTimeAccessor (&MyTcpEchoClient::m_interval),
                               MakeTimeChecker ())
                .AddAttribute ("RemoteAddress",
                               "The destination Ipv4Address of the outbound packets",
                               AddressValue (),
                               MakeAddressAccessor (&MyTcpEchoClient::m_peerAddress),
                               MakeAddressChecker ())
                .AddAttribute ("RemotePort",
                               "The destination port of the outbound packets",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoClient::m_peerPort),
                               MakeUintegerChecker<uint16_t> ())
                .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                               UintegerValue (100),
                               MakeUintegerAccessor (&MyTcpEchoClient::SetDataSize,
                                                     &MyTcpEchoClient::GetDataSize),
                               MakeUintegerChecker<uint32_t> ())
                .AddAttribute ("TagCounter","Number of RTTs",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoClient::SetTagCounter),
                               MakeUintegerChecker<uint8_t> ())
                .AddAttribute ("ClientNo","ClientNo",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoClient::SetClientNo),
                               MakeUintegerChecker<uint8_t> ())
                .AddAttribute ("MSS","MSS",
                               UintegerValue (0),
                               MakeUintegerAccessor (&MyTcpEchoClient::SetMSS),
                               MakeUintegerChecker<uint32_t> ())
                .AddTraceSource ("Tx", "A new packet is created and is sent",
                                 MakeTraceSourceAccessor (&MyTcpEchoClient::m_txTrace),
                                 "ns3::Packet::TracedCallback")
                .AddTraceSource ("Rx", "A packet has been received",
                                 MakeTraceSourceAccessor (&MyTcpEchoClient::m_rxTrace),
                                 "ns3::Packet::TracedCallback")
                .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                                 MakeTraceSourceAccessor (&MyTcpEchoClient::m_txTraceWithAddresses),
                                 "ns3::Packet::TwoAddressTracedCallback")
                .AddTraceSource ("RxWithAddresses", "A packet has been received",
                                 MakeTraceSourceAccessor (&MyTcpEchoClient::m_rxTraceWithAddresses),
                                 "ns3::Packet::TwoAddressTracedCallback")
        ;
        return tid;
    }


    TypeId MyTcpEchoClient::GetInstanceTypeId() const {
        return MyTcpEchoClient::GetTypeId(); }


    MyTcpEchoClient::MyTcpEchoClient ()
    {
        NS_LOG_FUNCTION (this);
        m_sent = 0;
        m_socket = 0;
        m_sendEvent = EventId ();
        m_data = 0;
        m_dataSize = 1000;
    }

    MyTcpEchoClient::~MyTcpEchoClient()
    {
        NS_LOG_FUNCTION (this);
        m_socket = 0;

        delete [] m_data;
        m_data = 0;
        m_dataSize = 1000;
        m_tagCounter = 0;
    }

    void
    MyTcpEchoClient::SetTagCounter (uint8_t tagCounter)
    {
        NS_LOG_FUNCTION (this << "tag Counter "<< tagCounter);
        m_tagCounter = tagCounter;
    }

    void
    MyTcpEchoClient::SetClientNo(uint8_t clientNo)
    {
        NS_LOG_FUNCTION (this << "clientNo: "<< clientNo);
        m_clientNo = clientNo;
    }

    void
    MyTcpEchoClient::SetIsComplete (){
        isComplete=true;
    }

    bool MyTcpEchoClient::GetIsComplete(){
        return isComplete;
    }


    void
    MyTcpEchoClient::SetRemote (Address ip, uint16_t port)
    {
        NS_LOG_FUNCTION (this);
        m_peerAddress = ip;
        m_peerPort = port;
    }

    void
    MyTcpEchoClient::SetRemote (Address addr)
    {
        NS_LOG_FUNCTION (this << addr);
        m_peerAddress = addr;
    }

    void
    MyTcpEchoClient::DoDispose (void)
    {
        NS_LOG_FUNCTION (this);
        Application::DoDispose ();
    }



    void
    MyTcpEchoClient::StartApplication (void)
    {
        startingIndex = 4;
        NS_LOG_FUNCTION (this);
        NS_LOG_FUNCTION ("client MSS" << m_maxSegmentSize);

        Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (m_maxSegmentSize));
        Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));


        if (m_socket == 0)
        {
            TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
            m_socket = Socket::CreateSocket (GetNode (), tid);
            if (m_socket->Bind () == -1)
            {
                NS_FATAL_ERROR ("Failed to bind socket");
            }
            m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }

        m_socket->SetRecvCallback (MakeCallback (&MyTcpEchoClient::HandleRead, this));

        ScheduleTransmit (Seconds (0.));
    }

    void
    MyTcpEchoClient::StopApplication ()
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
    MyTcpEchoClient::SetDataSize (uint32_t dataSize)
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
    MyTcpEchoClient::GetDataSize (void) const
    {
        NS_LOG_FUNCTION (this);
        return m_size;
    }

    void
    MyTcpEchoClient::SetFill (std::string fill)
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
    MyTcpEchoClient::SetFill (uint8_t fill, uint32_t dataSize)
    {
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
    MyTcpEchoClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
    {
        if (dataSize != m_dataSize)
        {
            delete [] m_data;
            m_data = new uint8_t [dataSize];
            m_dataSize = dataSize;
        }

        if (fillSize >= dataSize)
        {
            memcpy (m_data, fill, dataSize);
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




    void MyTcpEchoClient::SetPacketSizes(uint32_t * sizes){
        m_PacketSizes = sizes;
    }

    void MyTcpEchoClient::SetMSS(uint32_t mss){
        NS_LOG_FUNCTION (this<< mss);
        m_maxSegmentSize = mss;
    }


    uint32_t * MyTcpEchoClient::GetPacketSizes(){

        return m_PacketSizes;
    }

    void
    MyTcpEchoClient::ScheduleTransmit (Time dt)
    {
        NS_LOG_FUNCTION (this << dt);
        m_sendEvent = Simulator::Schedule (dt, &MyTcpEchoClient::Send, this);
    }

    void
    MyTcpEchoClient::Send (void)
    {
        NS_LOG_FUNCTION_NOARGS ();

        NS_ASSERT (m_sendEvent.IsExpired ());

        Ptr<Packet> p;
        uint32_t *arr = GetPacketSizes();
        //uint32_t newSize = arr[4-(+startingIndex)];
        uint32_t newSize = arr[4-(startingIndex)];
        startingIndex = startingIndex -2;

        if (m_dataSize)
        {
            p = Create<Packet> (m_data, m_dataSize);
        }
        else
        {
            p = Create<Packet> (newSize);
        }
        Address localAddress;
        m_socket->GetSockName (localAddress);
        m_txTrace (p);

        if (Ipv4Address::IsMatchingType (m_peerAddress))
        {
            m_txTraceWithAddresses (p, localAddress, InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
        else if (Ipv6Address::IsMatchingType (m_peerAddress))
        {
            m_txTraceWithAddresses (p, localAddress, Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
        }

        m_socket->Send (p);
        ++m_sent;

        if (Ipv4Address::IsMatchingType (m_peerAddress))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" sent " << p->GetSize() << " bytes to " <<
                                    Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
        }
        else if (Ipv6Address::IsMatchingType (m_peerAddress))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" sent "<< p->GetSize() << " bytes to " <<
                                    Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
        }
        else if (InetSocketAddress::IsMatchingType (m_peerAddress))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" sent " << p->GetSize() << " bytes to " <<
                                    InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
        }
        else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
        {
            NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" sent " << p->GetSize() << " bytes to " <<
                                    Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
        }

        if (m_sent < m_count)
        {
            ScheduleTransmit (m_interval);
        }
    }


    void
    MyTcpEchoClient::HandleRead (Ptr<Socket> socket)
    {
        NS_LOG_FUNCTION (this << socket);
        Ptr<Packet> packet;
        Address from;
        Address localAddress;
        TimestampTag timestampTag;

        NS_LOG_INFO("INSIDE MyTcpEchoClient::HandleRead , startingIndex IS : "<< startingIndex << " for client " << +m_clientNo );

        while ((packet = socket->RecvFrom (from)))
        {
            if (InetSocketAddress::IsMatchingType (from))
            {
                NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" received " << packet->GetSize () << " bytes from " <<
                                        InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                                        InetSocketAddress::ConvertFrom (from).GetPort ());
            }
            else if (Inet6SocketAddress::IsMatchingType (from))
            {
                NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client " << +m_clientNo <<" received "  << packet->GetSize () << " bytes from " <<
                                        Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                                        Inet6SocketAddress::ConvertFrom (from).GetPort ());
            }
            socket->GetSockName (localAddress);
            m_rxTrace (packet);
            m_rxTraceWithAddresses (packet, from, localAddress);

            uint8_t ttl = 0;
            uint32_t packetSize = packet->GetSize ();
            uint32_t *arr = GetPacketSizes();

            //uint32_t expectedSize = arr[4-(+startingIndex +1)];
            uint32_t expectedSize = arr[4-(startingIndex +1)];
            totalPacketSize = totalPacketSize + packetSize;

            if(totalPacketSize >= expectedSize){
                //uint32_t newSize = arr[4-(+startingIndex)];
                uint32_t newSize = arr[4-(startingIndex)];

                if (newSize < packetSize){
//                    NS_LOG_INFO("newSize < expectedSize==> packet->GetSize(): "<<packet->GetSize() << " newSize: " << +newSize );
//                    NS_LOG_INFO("packet->RemoveAtEnd2 abs(packet->GetSize() - +newSize) "<< abs(packet->GetSize() - +newSize));
                    packet->RemoveAtEnd2 ( abs(packetSize - +newSize) );



                }else if (newSize > packetSize){
//                    NS_LOG_INFO("newSize > expectedSize==> packet->GetSize(): "<<packet->GetSize() << " newSize: " << +newSize );
//                    NS_LOG_INFO(" packet->AddPaddingAtEnd abs(packet->GetSize() - +newSize) "<< abs(packet->GetSize() - +newSize));
                    packet->AddPaddingAtEnd( abs(packetSize - +newSize) );

                }else{

                }

                startingIndex = startingIndex - 2;
                NS_LOG_INFO("startingIndex updated to: "<<startingIndex << " for client " << +m_clientNo );

                if (startingIndex < 0) {
//                    NS_LOG_INFO("ttl is 0 client " << +m_clientNo << " aborting packet cycle! at" << Simulator::Now().GetNanoSeconds());
//                    NS_LOG_INFO("delay for client: "<< +m_clientNo << " "<< Simulator::Now().GetNanoSeconds() - Seconds(2.0).GetNanoSeconds());
                    std::cout << "ttl is 0 client " << +m_clientNo << " aborting packet cycle! at" << Simulator::Now().GetNanoSeconds() << std::endl;
                    std::cout << "delay for client: "<< +m_clientNo << " "<< Simulator::Now().GetNanoSeconds() - Seconds(2.0).GetNanoSeconds() << std::endl;

                    m_socket -> Close();
                    SetIsComplete();
                    return;
                }else {
                    socket->SendTo(packet, 0, from);
                    totalPacketSize = 0;
                }

                if (InetSocketAddress::IsMatchingType(from)) {
                    NS_LOG_INFO("At time "
                                        << Simulator::Now().As(Time::S) << " client " << +m_clientNo <<" sent "
                                        << packet->GetSize() << " bytes from "
                                        << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                        << InetSocketAddress::ConvertFrom(from).GetPort());
                } else if (Inet6SocketAddress::IsMatchingType(from)) {
                    NS_LOG_INFO("At time "
                                        << Simulator::Now().As(Time::S) <<  " client " << +m_clientNo <<" sent "
                                        << packet->GetSize() << " bytes from "
                                        << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                        << Inet6SocketAddress::ConvertFrom(from).GetPort());
                }

            }



        }
    }

} // Namespace ns3
