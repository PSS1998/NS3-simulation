#include <stdio.h>      /* printf, scanf, puts, NULL */
#include <stdlib.h>     /* srand, rand */
#include <time.h>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/config-store.h"
#include <ns3/buildings-helper.h>
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/basic-energy-source.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-radio-energy-model.h"

#include "ns3/random-variable-stream.h"
// #include <ns3/random-variable.h>



using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("CA3");

int main (int argc, char *argv[]) {

  Time simTime = MilliSeconds (1050);
  bool useCa = false;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("useCa", "Whether to use carrier aggregation.", useCa);
  cmd.Parse (argc, argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  // to save a template default attribute file run it like this:
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-simple
  //
  // to load a previously created default attribute file
  // ./waf --command-template="%s --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load --ns3::ConfigStore::FileFormat=RawText" --run src/lte/examples/lena-simple

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  // Parse again so you can override default values from the command line
  cmd.Parse (argc, argv);

  if (useCa)
   {
     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
   }

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();

  // Uncomment to enable logging
//  lteHelper->EnableLogComponents ();

  // Create Nodes: eNodeB and UE
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (5);
  ueNodes.Create (10);

  // Install Mobility Model
  MobilityHelper mobility;
  // Put everybody into a line
  Ptr<ListPositionAllocator> initialAllocEnb = 
    CreateObject<ListPositionAllocator> ();
  for (uint32_t i = 0; i < enbNodes.GetN (); ++i) {
      initialAllocEnb->Add (Vector (i, 0., 0.));
  }
  mobility.SetPositionAllocator(initialAllocEnb);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (enbNodes);
  BuildingsHelper::Install (enbNodes);
  Ptr<ListPositionAllocator> initialAllocUe = 
    CreateObject<ListPositionAllocator> ();
  srand (time(NULL));
  for (uint32_t i = 0; i < ueNodes.GetN (); ++i) {
      double x = ((double) rand() / (RAND_MAX))-0.5;
      double y = ((double) rand() / (RAND_MAX))-0.5;
      initialAllocUe->Add (Vector (x, y, 0.));
  }
  mobility.SetPositionAllocator(initialAllocUe);
  // mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  ObjectFactory pos;
  pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
  pos.Set ("X", StringValue ("ns3::ConstantRandomVariable[Constant=5.0]"));
  pos.Set ("Y", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
  mobility.SetMobilityModel ("ns3::RandomWaypointMobilityModel",
  							 "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"),
  							 "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
  							 "PositionAllocator", PointerValue (taPositionAlloc));
  mobility.Install (ueNodes);
  BuildingsHelper::Install (ueNodes);

  // Create Devices and install them in the Nodes (eNB and UE)
  NetDeviceContainer enbDevs;
  NetDeviceContainer ueDevs;
  // Default scheduler is PF, uncomment to use RR
  //lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");

  enbDevs = lteHelper->InstallEnbDevice (enbNodes);
  ueDevs = lteHelper->InstallUeDevice (ueNodes);

  // Attach a UE to a eNB
  lteHelper->AttachToClosestEnb (ueDevs, enbDevs);

  lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");

  // Activate a data radio bearer
  enum EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
  EpsBearer bearer (q);
  lteHelper->ActivateDataRadioBearer (ueDevs, bearer);
  lteHelper->EnableTraces ();


  // PointToPointHelper pointToPoint;
  // pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  // pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  // NetDeviceContainer enbdevices, uedevices;
  // enbdevices = pointToPoint.Install (enbNodes);
  // uedevices = pointToPoint.Install (ueNodes);

  // InternetStackHelper stack;
  // stack.Install (enbNodes);
  // stack.Install (ueNodes);

  // Ipv4AddressHelper address;
  // address.SetBase ("10.1.1.0", "255.255.255.0");

  // Ipv4InterfaceContainer interfaces = address.Assign (ueNodes);

  // UdpEchoServerHelper echoServer (9);

  // ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  // serverApps.Start (Seconds (1.0));
  // serverApps.Stop (Seconds (10.0));

  // UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
  // echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  // echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  // echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  // clientApps.Start (Seconds (2.0));
  // clientApps.Stop (Seconds (10.0));


  Simulator::Stop (simTime);
  Simulator::Run ();

  // GtkConfigStore config;
  // config.ConfigureAttributes ();

  Simulator::Destroy ();
  
  return 0;

}
