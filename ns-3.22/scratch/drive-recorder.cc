#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/drive-recorder-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DRIVE_RECORDER");

int
main (int argc, char *argv[])
{
  uint32_t cliCnt = 1;
  CommandLine cmd;
  cmd.AddValue ("cliCnt", "Number of Client Nodes", cliCnt);
  cmd.Parse(argc, argv);
  
  if (cliCnt > 5) {
    std::cout << "too many client nodes.(<=5)" << std::endl;
    return -1;
  }
  
  LogComponentEnable("DRIVE_RECORDER", LOG_LEVEL_INFO);
  LogComponentEnable("DRIVE_RECORDER_UTIL", LOG_LEVEL_INFO);
  LogComponentEnable("ClientApplication", LOG_LEVEL_FUNCTION);
  LogComponentEnable("ServerApplication", LOG_LEVEL_FUNCTION);

  uint32_t serCnt = 1;
  NodeContainer cliNodes, serNodes;
  cliNodes.Create (cliCnt);
  serNodes.Create (serCnt);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));

  std::vector<NetDeviceContainer> p2pDevs(cliCnt);
  for (uint32_t idx = 0; idx < cliCnt; ++idx) {
    p2pDevs[idx] = p2p.Install (NodeContainer (cliNodes.Get (idx), serNodes.Get (0)));
  }

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(100.0),
                                "DeltaY", DoubleValue(100.0),
                                "GridWidth", UintegerValue(10.0),
                                "LayoutType", StringValue("RowFirst"));

  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(cliNodes);
  
  std::vector<Vector> initCliPosition(cliNodes.GetN());
  for (uint32_t idx = 0; idx < cliNodes.GetN(); ++idx) {
    initCliPosition[idx] = Vector (10 * (idx / 3), 10 * (idx % 3), 0);
  }

  for (uint32_t idx = 0; idx < cliNodes.GetN(); ++idx) {
    SetPositionVelocity (cliNodes.Get (idx), initCliPosition[idx], Vector (2, 0, 0));
  }

  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(serNodes);

  SetPosition (serNodes.Get (0), Vector (100, 100, 0));

  p2p.EnablePcapAll ("ap2ser");

  InternetStackHelper stack;
  stack.Install (cliNodes);
  stack.Install (serNodes);

  for (uint32_t idx = 0; idx < cliCnt; ++idx) {
    SetIpv4Address (p2pDevs[idx].Get (0), BuildIpv4Address (10, 1, 1, idx + 2), Ipv4Mask ("/24"));
  }

  for (uint32_t idx = 0; idx < cliCnt; ++idx) {
    SetIpv4Address (p2pDevs[idx].Get (1), Ipv4Address(serIpv4Address.c_str()), Ipv4Mask ("/24"));
  }
  AddressValue serAddress (InetSocketAddress (Ipv4Address(serIpv4Address.c_str()), serPort));
  ServerApplicationHelper serHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address(serIpv4Address.c_str()), serPort));
  serHelper.SetAttribute ("Local", serAddress);
  ApplicationContainer serApp = serHelper.Install (serNodes.Get (0));
  serApp.Start (Seconds (0.));
  //serApp.Stop (Seconds (100.));

  ClientApplicationHelper cliAppHelper;
  cliAppHelper.SetAttribute ("Remote", serAddress);
  ApplicationContainer cliApps = cliAppHelper.Install (cliNodes);
  cliApps.Start (Seconds (5));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  
  AnimationInterface anim ("drive-recorder.xml");

  //Simulator::Stop(Seconds(1500.0));  
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
