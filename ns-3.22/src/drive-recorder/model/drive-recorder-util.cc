#include "drive-recorder-util.h"

namespace ns3{

  NS_LOG_COMPONENT_DEFINE("DRIVE_RECORDER_UTIL");

  Ipv4Address BuildIpv4Address (const uint32_t byte0, const uint32_t byte1, const uint32_t byte2, const uint32_t byte3) {
    std::ostringstream oss;
    oss << byte0 << "." << byte1 << "." << byte2 << "." << byte3;
    return Ipv4Address (oss.str().c_str());
  }

  void SetIpv4Address (const Ptr<NetDevice> device, const Ipv4Address address, const Ipv4Mask mask) {
    Ptr<Node> node = device->GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    int32_t interface = ipv4->GetInterfaceForDevice(device);

    if (-1 == interface)
      interface = ipv4->AddInterface(device);

    ipv4->AddAddress(interface, Ipv4InterfaceAddress(address, mask));
    ipv4->SetMetric(interface, 1);
    ipv4->SetForwarding(interface, true);
    ipv4->SetUp(interface);
  }
  
  Ipv4Address GetIpv4Address (const Ptr<NetDevice> device) {
    Ptr<Node> node = device->GetNode();
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    int32_t interface = ipv4->GetInterfaceForDevice(device);

    return ipv4->GetAddress(interface, 0).GetLocal();
  }

  void SetPosition(const Ptr<Node> node, const Vector& position) {
    Ptr<MobilityModel> m = node->GetObject<MobilityModel>();
    m->SetPosition(position);
  }

  void SetPositionVelocity (const Ptr<Node> node, const Vector& position, const Vector& velocity) {
    Ptr<ConstantVelocityMobilityModel> m = node->GetObject<ConstantVelocityMobilityModel>();
    SetPosition(node, position);
    m->SetVelocity(velocity);
  }

  double GetDistance (struct Location loc1, struct Location loc2) {
    return sqrt ((loc1.m_x - loc2.m_x) * (loc1.m_x - loc2.m_x) + (loc1.m_y - loc2.m_y) * (loc1.m_y - loc2.m_y));
  }
}
