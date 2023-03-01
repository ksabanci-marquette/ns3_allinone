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

//./waf --run "scratch/scenario --numberOfUEs=100 --verbose=false --simulationTime=10000" >> scenario_udp_100UE.log 2>&1 &

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NbIoTExample");

int main (int argc, char *argv[])
{
    std::string simName = "test";
    double cellsize = 1000; // in meters
    uint32_t numberOfUEs = 10;//12;
    int numberOfRemoteHosts = 1;
    int numberOfENBs = 1;//numberOfUEs/12;
    bool verbose = false;
    int simulationTime = 100;


    bool ciot = false;
    bool edt = false;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("numberOfUEs", "numberOfUEs", numberOfUEs);
    cmd.AddValue ("verbose", "verbose", verbose);
    cmd.AddValue ("simulationTime", "simulationTime in seconds", simulationTime);

    cmd.Parse (argc, argv);

    uint32_t packetSizes[10] = {0, 0, 0, 0, 1872, 8298, 80, 532, 0, 0 };
    uint32_t packetSize = 0;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds (1.);
    Time simTime = Seconds(simulationTime);

    LogComponentEnable ("NbIoTExample", LOG_LEVEL_ALL);
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
    LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);
    LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
    LogComponentEnable("LteEnbMac", LOG_LEVEL_INFO);
    LogComponentEnable("LteUeMac", LOG_LEVEL_INFO);

    if (verbose) {
        LogComponentEnable("LteUeRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbMac", LOG_LEVEL_FUNCTION);
    }


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
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    // to do
    // set mtu to 1500
    // sent packages with real fragmented sizes
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (9000));

    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


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
        UdpEchoServerHelper server (ulPort);
        serverApps.Add(server.Install (remoteHost));
        server.SetPacketSizes(serverApps.Get (i),packetSizes);


        UdpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue(interPacketInterval));//?
        ulClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));//?
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetSize));//?

        ulClient.SetAttribute ("TagCounter", UintegerValue (sizeof(packetSizes) / sizeof(int)));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));
        ulClient.SetPacketSizes(clientApps.Get (i),packetSizes);

    }

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    serverApps.Start (Seconds (1.0));
    serverApps.Stop (simTime);

    clientApps.Start (Seconds (2.0));
    clientApps.Stop (simTime);


    Simulator::Stop (simTime);
    Simulator::Run ();

    flowMonitor->SerializeToXmlFile("flowMonitor.xml", true, true);

    Simulator::Destroy ();

}

