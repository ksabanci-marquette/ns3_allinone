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

#ifndef MY_TCP_ECHO_SERVER_H
#define MY_TCP_ECHO_SERVER_H
#include <map>
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/traced-callback.h"

namespace ns3 {

    class Socket;
    class Packet;

/**
 * \ingroup applications
 * \defgroup tcpecho TcpEcho
 */

/**
 * \ingroup tcpecho
 * \brief A Tcp Echo server
 *
 * Every packet received is sent back.
 */
    class MyTcpEchoServer : public Application
    {
    public:
        static TypeId GetTypeId (void);
        MyTcpEchoServer ();
        virtual ~MyTcpEchoServer ();

        void SetPacketSizes(uint32_t * sizes);

        uint32_t * GetPacketSizes();

        void SetServerNo(uint8_t serverNo);

        void SetMSS (uint32_t mss);

    protected:
        virtual void DoDispose (void);

    private:

        virtual void StartApplication (void);
        virtual void StopApplication (void);

        void HandleRead (Ptr<Socket> socket);
        void HandleAccept (Ptr<Socket> socket, const Address& from);
        bool HandleAcceptRequest (Ptr<Socket> socket, const Address& from);
        void HandleClose (Ptr<Socket> socket);

        uint16_t m_port;
        Ptr<Socket> m_socket;
        Address m_local;
        std::map<Ptr<Socket>, Address> m_conn;
        uint8_t m_tagCounter;
        uint32_t * m_PacketSizes;
        /// Callbacks for tracing the packet Rx events
        TracedCallback<Ptr<const Packet> > m_rxTrace;

        /// Callbacks for tracing the packet Rx events, includes source and destination addresses
        TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
        uint32_t m_maxSegmentSize = 600; // bytes
        uint32_t totalPacketSize = 0; // bytess
        int startingIndex;
        int m_serverNo;

    };

} // namespace ns3

#endif /* TCP */
