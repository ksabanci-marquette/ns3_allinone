#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/lte-module.h"
#include <ns3/winner-plus-propagation-loss-model.h>
//#include "ns3/gtk-config-store.h"
#include <chrono>
#include <iomanip>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <thread>
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NbIoTExample");

// ./waf --run "scratch/scenario_tcp --numberOfUEs=20 --verbose=false --simulationTime=101"
int main (int argc, char *argv[])
{

    Time::SetResolution (Time::NS);
    std::string simName = "NbIoTExample";
    bool verbose = false;
    double cellsize = 1000; // in meters
    int numberOfUEs = 12;
    int numberOfRemoteHosts = 12;
    int numberOfENBs = 1;//numberOfUEs/12;
    int simulationTime = 100;

    bool ciot = false;
    bool edt = false;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("numberOfUEs", "numberOfUEs", numberOfUEs);
    cmd.AddValue ("verbose", "verbose", verbose);
    cmd.AddValue ("simulationTime", "simulationTime in seconds", simulationTime);
    cmd.AddValue ("numberOfRemoteHosts", "numberOfRemoteHosts", numberOfRemoteHosts);
    cmd.Parse (argc, argv);


    uint32_t packetSizes[5] = {1872,8298,80,478,54};
    uint32_t packetSize = 0;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds (1.);
    Time simTime = Seconds(simulationTime);


    LogComponentEnable ("NbIoTExample", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoServerApplication", LOG_LEVEL_INFO);

    if (verbose) {
        LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);
        LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
        LogComponentEnable("LteEnbMac", LOG_LEVEL_INFO);
        LogComponentEnable("LteUeMac", LOG_LEVEL_INFO);
        LogComponentEnable("LteUeRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbMac", LOG_LEVEL_FUNCTION);
    }

    NS_LOG_INFO ("numberOfUEs " << numberOfUEs);
    NS_LOG_INFO ("simulationTime " << simulationTime);
    NS_LOG_INFO ("numberOfRemoteHosts " << numberOfRemoteHosts);


    NS_LOG_INFO ("Create LTE.");

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
    lteHelper->SetEpcHelper (epcHelper);
    lteHelper->EnableRrcLogging ();
    lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
    lteHelper->SetUeAntennaModelType ("ns3::IsotropicAntennaModel");
    lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::WinnerPlusPropagationLossModel"));
    lteHelper->SetPathlossModelAttribute ("HeightBasestation", DoubleValue (50));
    lteHelper->SetPathlossModelAttribute ("Environment", EnumValue (UMaEnvironment));
    lteHelper->SetPathlossModelAttribute ("LineOfSight", BooleanValue (false));



    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (numberOfRemoteHosts);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install (remoteHostContainer);


    // Create the Internet
    NS_LOG_INFO ("Create Internet.");
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    // to do
    // set mtu to 1500
    // sent packages with real fragmented sizes
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
    Address serverAddress = Address(internetIpIfaces.GetAddress (1));
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


    NS_LOG_INFO ("Set Position");
    NodeContainer enbNodes;
    enbNodes.Create (numberOfENBs);
    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
    positionAlloc->Add (Vector (cellsize/2, cellsize/2, 25)); //last 25 parameter is probably height of tower?

    MobilityHelper mobilityEnb;
    mobilityEnb.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobilityEnb.SetPositionAllocator(positionAlloc);
    mobilityEnb.Install(enbNodes);


    NodeContainer ueNodes;
    ueNodes.Create (numberOfUEs);
    Ptr<ListPositionAllocator> positionAllocUe = CreateObject<ListPositionAllocator> ();


    ObjectFactory pos;
    pos.SetTypeId ("ns3::UniformDiscPositionAllocator");
    pos.Set ("X", StringValue (std::to_string(cellsize/2)));
    pos.Set ("Y", StringValue (std::to_string(cellsize/2)));
    pos.Set ("Z", DoubleValue (5)); //UEs at 5 meters
    pos.Set ("rho", DoubleValue (cellsize/2));
    Ptr<PositionAllocator> m_position = pos.Create ()->GetObject<PositionAllocator> ();

    for (uint32_t i = 0; i < numberOfUEs; ++i){
        Vector position = m_position->GetNext ();
        positionAllocUe->Add (position);
        //std::cout << "a," << position.x << "," << position.y << "," << position.z << std::endl;
    }

    NS_LOG_INFO ("Set Mobility.");
    MobilityHelper mobilityUe;
    mobilityUe.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityUe.SetPositionAllocator(positionAllocUe);
    mobilityUe.Install (ueNodes);


    // Install LTE Devices to the nodes
    NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
    NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

    // Install the IP stack on the UEs
    internet.Install (ueNodes);
    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

    // Assign IP address to UEs, and install applications
    for (uint32_t u = 0; u < ueNodes.GetN (); ++u){
        Ptr<Node> ueNode = ueNodes.Get (u);
        // Set the default gateway for the UE
        Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
        ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }


    // Install and start applications on UEs and remote host
    uint16_t ulPort = 2000;
    //uint16_t dlPort = 1000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    //lteHelper->Attach(ueLteDevs, enbLteDevs.Get(0));

// Set up the data transmission for the Pre-Run
    for (uint16_t i = 0; i < numberOfUEs; i++){
        NS_LOG_INFO ("Creating Applications. " << i);

        lteHelper->AttachSuspendedNb(ueLteDevs.Get(i), enbLteDevs.Get(i % numberOfENBs));
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();

        Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();

        if(ciot == true){
            //std::cout << "ciot" << std::endl;
            ueRrc->SetAttribute("CIoT-Opt", BooleanValue(true));
        }
        else{
            ueRrc->SetAttribute("CIoT-Opt", BooleanValue(false));
        }
        if(edt == true){
            //std::cout << "EDT" << std::endl;
            ueRrc->SetAttribute("EDT", BooleanValue(true));
        }
        else{
            ueRrc->SetAttribute("EDT", BooleanValue(false));
        }


        ++ulPort;
        //UdpEchoServerHelper server (ulPort);
        MyTcpEchoServerHelper server (serverAddress, ulPort);
//        serverApps.Add(server.Install (remoteHost));
        serverApps.Add(server.Install ( remoteHostContainer.Get (i)));
        server.SetPacketSizes(serverApps.Get (i),packetSizes);
        server.SetAttribute("ServerNo", UintegerValue(i));

//        MyTcpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        MyTcpEchoClientHelper ulClient (internetIpIfaces.GetAddress (i+1), ulPort);
        ulClient.SetAttribute ("Interval", TimeValue(interPacketInterval));//?
        ulClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));//?
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetSize));//?
        ulClient.SetAttribute ("ClientNo", UintegerValue(i));//?

        ulClient.SetAttribute ("TagCounter", UintegerValue (sizeof(packetSizes) / sizeof(int)));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));
        ulClient.SetPacketSizes(clientApps.Get (i),packetSizes);

    }

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start Applications.");

    serverApps.Start (Seconds (1.0));
    serverApps.Stop (simTime);

    clientApps.Start (Seconds (2.0));
    clientApps.Stop (simTime);

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (simTime);
    Simulator::Run ();
    //flowMonitor->SerializeToXmlFile("flowMonitor.xml", true, true);
    Simulator::Destroy ();
    NS_LOG_INFO ("Exiting Simulation at time: "<< Simulator::Now().As(Time::S) );

}

