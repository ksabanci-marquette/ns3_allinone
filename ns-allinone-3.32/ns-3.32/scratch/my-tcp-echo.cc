/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//       =================
//              LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues 
// - Tracing of queues and packet receptions to file "udp-echo.tr"

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include <thread>
#include <iostream>
#include <chrono>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyTcpEchoExample");

int
main (int argc, char *argv[])
{
    LogComponentEnable ("MyTcpEchoExample", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable ("MyTcpEchoServerApplication", LOG_LEVEL_ALL);
    //LogComponentEnable ("MyTag", LOG_LEVEL_ALL);

    //LogComponentEnable ("Socket", LOG_LEVEL_ALL);
    //LogComponentEnable  ("Address", LOG_LEVEL_FUNCTION);
    //LogComponentEnable  ("Ipv4Address", LOG_LEVEL_FUNCTION);


    bool useV6 = false;
    Address serverAddress;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("useIpv6", "Use Ipv6", useV6);
    cmd.Parse (argc, argv);
    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO ("Create nodes.");
    NodeContainer n;
    n.Create (4);

    InternetStackHelper internet;
    internet.Install (n);

    NS_LOG_INFO ("Create channels.");
    //
    // Explicitly create the channels required by the topology (shown above).
    //
    CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
    csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
    csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    NetDeviceContainer d = csma.Install (n);

    //
    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO ("Assign IP Addresses.");
    if (useV6 == false)
    {
        Ipv4AddressHelper ipv4;
        ipv4.SetBase ("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer i = ipv4.Assign (d);
        serverAddress = Address(i.GetAddress (1));
    }
    else
    {
        Ipv6AddressHelper ipv6;
        ipv6.SetBase ("2001:0000:f00d:cafe::", Ipv6Prefix (64));
        Ipv6InterfaceContainer i6 = ipv6.Assign (d);
        serverAddress = Address(i6.GetAddress (1,1));
    }

    NS_LOG_INFO ("Create Applications.");

    uint32_t packetSizes[4] = {1872,8298,80,478};

    uint16_t port = 9;  // well-known echo port number
    MyTcpEchoServerHelper server (serverAddress,port);
    ApplicationContainer apps = server.Install (n.Get (1));
    server.SetPacketSizes(apps.Get (0),packetSizes);
    apps.Start (Seconds (1.0));
    apps.Stop (Seconds (100.0));

    uint32_t packetSize = 0;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds (1.);
    MyTcpEchoClientHelper client (serverAddress, port);
    client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
    client.SetAttribute ("Interval", TimeValue (interPacketInterval));
    client.SetAttribute ("PacketSize", UintegerValue (packetSize));

    NS_LOG_INFO("sizeof(packetSizes) : " << sizeof(packetSizes) / sizeof(int));
    client.SetAttribute ("TagCounter", UintegerValue (sizeof(packetSizes) / sizeof(int)));
    ApplicationContainer apps2  = client.Install (n.Get (0));

    NS_LOG_INFO ( apps.GetN());
    client.SetPacketSizes(apps2.Get (0),packetSizes);
    //std::this_thread::sleep_for( std::chrono::seconds(2) );
    apps2.Start (Seconds (2.0));
    apps2.Stop (Seconds (102.0));
    //client.SetFill (apps2.Get (0), "Hello World");

    //AsciiTraceHelper ascii;
    //csma.EnableAsciiAll (ascii.CreateFileStream ("my-tcp-echo.tr"));
    csma.EnablePcapAll ("my-tcp-echo", true);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO ("Run Simulation.");
    Simulator::Run ();
    Simulator::Destroy ();
    NS_LOG_INFO ("Done.");
}


