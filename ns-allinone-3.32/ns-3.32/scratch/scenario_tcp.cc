#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/random-variable-stream.h"
#include "ns3/lte-module.h"
#include <ns3/winner-plus-propagation-loss-model.h>
#include <chrono>
#include <iomanip>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <thread>
#include "ns3/flow-monitor-helper.h"
#include "ns3/my-tcp-echo-client.h"
#include "ns3/simulator.h"

//./waf --run "scratch/scenario_tcp --numberOfUEs=10 --mss=200 --verbose=false --simulationTime=1000 --profile=EcRsa" >> scenario_tcp_10UE.log 2>&1 &

using namespace ns3;

uint32_t nClientApps = 0;
int numberOfUEs = 12;
ApplicationContainer clientApps;
ApplicationContainer serverApps;

NS_LOG_COMPONENT_DEFINE ("NbIoTExample");

void checkClients () {

    int numberOfCompleted=0;

    for (int i=0; i<nClientApps; i++) {
        Ptr <MyTcpEchoClient> client1 = DynamicCast<MyTcpEchoClient>(clientApps.Get(i));
        if(client1->GetIsComplete()){
            numberOfCompleted++;
        }
    }

    if (numberOfCompleted == numberOfUEs){
        NS_LOG_INFO ("numberOfCompleted: "<<numberOfCompleted<<" ENDING SIMULATION"<< Simulator::Now().As(Time::S));
        Simulator::Stop ();
        Simulator::Destroy ();
        exit (0);
    }

    Simulator::Schedule (Seconds (1), &checkClients);
}


int main (int argc, char *argv[])
{

    Time::SetResolution (Time::NS);
    std::string simName = "NbIoTExample";
    bool verbose = false;
    double cellsize = 1000; // in meters
    int numberOfRemoteHosts = 1;
    int numberOfENBs = 1;//numberOfUEs/12;
    int simulationTime = 100;
    int mss = 600;
    std::string profile = "EcRsa";

    uint32_t packetSizesEcRsa[4] = {415,2037,80,494};
    uint32_t packetSizesEcEcdsa[4] = {415,1059,80,494};
    uint32_t packetSizesKyberRsa[4] = {1143,2821,80,478};
    uint32_t packetSizesKyberEcdsa[4] = {1143,1843,80,494};
    uint32_t packetSizesKyberFalcon512[4] = {1143,5248,80,494};
    uint32_t packetSizesKyberDilithium[4] = {1143,15296,80,478};
    uint32_t packetSizesKyberSphincs[4] = {1143,25113,80,494};
    uint32_t packetSizes[4] ;

    bool ciot = false;
    bool edt = false;

    CommandLine cmd (__FILE__);
    cmd.AddValue ("numberOfUEs", "numberOfUEs", numberOfUEs);
    cmd.AddValue ("mss", "mss", mss);
    cmd.AddValue ("verbose", "verbose", verbose);
    cmd.AddValue ("simulationTime", "simulationTime in seconds", simulationTime);
    cmd.AddValue ("profile", "profile", profile);

    cmd.Parse (argc, argv);

    if( profile == "EcRsa" || profile == "" ) {
        std::copy(std::begin(packetSizesEcRsa), std::end(packetSizesEcRsa), std::begin(packetSizes));

    }else if (profile == "EcEc"){
        std::copy(std::begin(packetSizesEcEcdsa), std::end(packetSizesEcEcdsa), std::begin(packetSizes));

    }else if (profile == "KyberRsa"){
        std::copy(std::begin(packetSizesKyberRsa), std::end(packetSizesKyberRsa), std::begin(packetSizes));

    }else if (profile == "KyberEc"){
        std::copy(std::begin(packetSizesKyberEcdsa), std::end(packetSizesKyberEcdsa), std::begin(packetSizes));

    }else if (profile == "Falcon"){
        std::copy(std::begin(packetSizesKyberFalcon512), std::end(packetSizesKyberFalcon512), std::begin(packetSizes));

    }else if (profile == "Sphincs"){
        std::copy(std::begin(packetSizesKyberSphincs), std::end(packetSizesKyberSphincs), std::begin(packetSizes));

    }else if (profile == "Dilithium"){
        std::copy(std::begin(packetSizesKyberDilithium), std::end(packetSizesKyberDilithium), std::begin(packetSizes));

    }else {
        std::copy(std::begin(packetSizesEcRsa), std::end(packetSizesEcRsa), std::begin(packetSizes));
    }

    uint32_t packetSize = 0;
    uint32_t maxPacketCount = 1;
    Time interPacketInterval = Seconds (1.);
    Time simTime = Seconds(simulationTime);

    LogComponentEnable ("NbIoTExample", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoServerApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("MyTcpEchoClientApplication", LOG_LEVEL_FUNCTION);
    LogComponentEnable ("MyTcpEchoServerApplication", LOG_LEVEL_FUNCTION);
//    LogComponentEnable ("LteUeMac", LOG_LEVEL_FUNCTION);
//    LogComponentEnable ("LteEnbMac", LOG_LEVEL_FUNCTION);


    if (verbose) {
        LogComponentEnable("LteUeRrc", LOG_LEVEL_INFO);
        LogComponentEnable("LteEnbRrc", LOG_LEVEL_INFO);
        LogComponentEnable("LteEnbMac", LOG_LEVEL_INFO);
        LogComponentEnable("LteUeMac", LOG_LEVEL_INFO);
        LogComponentEnable ("LteUePhy", LOG_LEVEL_INFO);
        LogComponentEnable("LteUeRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbRrc", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteEnbMac", LOG_LEVEL_FUNCTION);
        LogComponentEnable("LteUeMac", LOG_LEVEL_FUNCTION);
        LogComponentEnable ("LteUePhy", LOG_LEVEL_FUNCTION);
    }

    NS_LOG_INFO ("numberOfUEs " << numberOfUEs);
    NS_LOG_INFO ("simulationTime " << simulationTime);
    NS_LOG_INFO ("profile " << profile);
    NS_LOG_INFO ("mss " << mss);

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
    Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));
    Config::SetDefault ("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (false));
    Config::SetDefault ("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue (false));

    Ptr<Node> pgw = epcHelper->GetPgwNode ();
    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create (numberOfRemoteHosts);
    Ptr<Node> remoteHost = remoteHostContainer.Get (0);
    InternetStackHelper internet;
    internet.Install (remoteHostContainer);

    NS_LOG_INFO ("Create Internet.");
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
    p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
    p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
    p2ph.EnablePcapAll(profile +"_"+std::to_string(numberOfUEs) +"_mss"+std::to_string(mss) + "_scenario", true);
    NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
    Address serverAddress = Address(internetIpIfaces.GetAddress (1));
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
    remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


    NS_LOG_INFO ("Set Position");
    NodeContainer enbNodes;
    enbNodes.Create (numberOfENBs);
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
//    pos.SetTypeId ("ns3::UniformDiscPositionAllocator");
    pos.SetTypeId ("ns3::RandomDiscPositionAllocator");
    pos.Set ("X", StringValue (std::to_string(cellsize/2)));
    pos.Set ("Y", StringValue (std::to_string(cellsize/2)));
    pos.Set ("Z", DoubleValue (5)); //UEs at 5 meters
    pos.Set ("Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=30]"));
    Ptr<PositionAllocator> m_position = pos.Create ()->GetObject<PositionAllocator> ();

    for (uint32_t i = 0; i < numberOfUEs; ++i){
        Vector position = m_position->GetNext ();
        positionAllocUe->Add (position);
        std::cout << "position: " << position.x << "," << position.y << "," << position.z << std::endl;
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

    for (uint16_t i = 0; i < numberOfUEs; i++){
        NS_LOG_INFO ("Creating Applications. " << i);

        lteHelper->AttachSuspendedNb(ueLteDevs.Get(i), enbLteDevs.Get(i % numberOfENBs));
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        Ptr<LteUeNetDevice> ueLteDevice = ueLteDevs.Get(i)->GetObject<LteUeNetDevice> ();

        Ptr<LteUeRrc> ueRrc = ueLteDevice->GetRrc();
//        ueRrc->SetAttribute("CIoT-Opt", BooleanValue(false));
//        ueRrc->SetAttribute("EDT", BooleanValue(false));

        ++ulPort;
        MyTcpEchoServerHelper server (serverAddress, ulPort, mss);
        serverApps.Add(server.Install (remoteHost));
        server.SetPacketSizes(serverApps.Get (i),packetSizes);
        server.SetAttribute("ServerNo", UintegerValue(i));

        MyTcpEchoClientHelper ulClient (remoteHostAddr, ulPort);
        ulClient.SetAttribute ("Interval", TimeValue(interPacketInterval));//?
        ulClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));//?
        ulClient.SetAttribute ("PacketSize", UintegerValue(packetSize));//?
        ulClient.SetAttribute ("ClientNo", UintegerValue(i));//?
        ulClient.SetAttribute("MSS", UintegerValue(mss));

        ulClient.SetAttribute ("TagCounter", UintegerValue (sizeof(packetSizes) / sizeof(int)));
        clientApps.Add (ulClient.Install (ueNodes.Get(i)));
        ulClient.SetPacketSizes(clientApps.Get (i),packetSizes);

    }
    nClientApps = clientApps.GetN ();
    Simulator::Schedule (Seconds (0), &checkClients);

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowHelper;
    flowMonitor = flowHelper.InstallAll();

    NS_LOG_INFO ("Start sim with "<<nClientApps<< " client apps." );

//    Config::SetDefault ("ns3::ConfigStore::Filename", StringValue ("output-attributes.txt"));
//    Config::SetDefault ("ns3::ConfigStore::FileFormat", StringValue ("RawText"));
//    Config::SetDefault ("ns3::ConfigStore::Mode", StringValue ("Save"));
//    ConfigStore outputConfig2;
//    outputConfig2.ConfigureDefaults ();
//    outputConfig2.ConfigureAttributes ();

    serverApps.Start (Seconds (1.0));
    serverApps.Stop (simTime);

    clientApps.Start (Seconds (2.0));
    clientApps.Stop (simTime);

    NS_LOG_INFO ("Run Simulation.");
    Simulator::Stop (simTime);
    Simulator::Run ();
    flowMonitor->SerializeToXmlFile(profile +"_"+std::to_string(numberOfUEs) +"_mss"+ std::to_string(mss) +"_flowMonitor.xml", true, true);
//    Simulator::Destroy ();
//    NS_LOG_INFO ("Exiting Simulation at time: "<< Simulator::Now().As(Time::S) );

}

