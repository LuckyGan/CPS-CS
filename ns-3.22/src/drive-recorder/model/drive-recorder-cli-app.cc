#include "drive-recorder-cli-app.h"

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "drive-recorder-util.h"
#include "ns3/wifi-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ClientApplication");

NS_OBJECT_ENSURE_REGISTERED (ClientApplication);

TypeId ClientApplication::GetTypeId () {
  static TypeId tid = TypeId ("ns3::ClientApplication")
    .SetParent<Application> ()
    .AddConstructor<ClientApplication> ()
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&ClientApplication::m_peer),
                   MakeAddressChecker ());
  return tid;
}

ClientApplication::ClientApplication (Ptr<Node> node) {
  //NS_LOG_FUNCTION (this);
}

ClientApplication::ClientApplication ()
  : m_socket (0),
    m_connected (false),
    m_tid (TypeId::LookupByName ("ns3::TcpSocketFactory")),
    m_velProdInterval (5) {
  //NS_LOG_FUNCTION (this);
}

ClientApplication::~ClientApplication () {
  //NS_LOG_FUNCTION (this);

  m_socket = 0;
}

Ptr<Socket> ClientApplication::GetSocket () const {
  //NS_LOG_FUNCTION (this);

  return m_socket;
}

void ClientApplication::StartApplication () {
  //NS_LOG_FUNCTION (this);

  m_device = GetNode ()->GetDevice (0);
  m_mobility = GetNode ()->GetObject<MobilityModel> ();

  m_socket = Socket::CreateSocket (GetNode (), m_tid);
  m_socket->Bind ();
  m_socket->Connect (m_peer);

  m_socket->SetConnectCallback (
    MakeCallback (&ClientApplication::ConnectionSucceeded, this),
    MakeCallback (&ClientApplication::ConnectionFailed, this));
  
  m_socket->SetSendCallback (
    MakeCallback (&ClientApplication::DataSend, this));

  m_socket->SetRecvCallback (
    MakeCallback (&ClientApplication::GetQueryFromSer, this));

}

void ClientApplication::StopApplication () {
  //NS_LOG_FUNCTION (this);

  if (m_socket != 0) {
    m_socket->Close ();
    m_connected = false;
  }
}

/*
 * 上传节点信息
 */
void ClientApplication::SendInfo () {
  //NS_LOG_FUNCTION (this);

  if (m_connected) {
    ByteBuffer buffer;
    buffer.put<uint16_t>(Cycle, sizeof (uint16_t));
    buffer.put<Address>(m_device->GetAddress(), sizeof (Address));
  
    Location loc = Location (m_mobility->GetPosition().x, m_mobility->GetPosition().y);
    buffer.put<Location>(loc, sizeof (Location));
    std::vector<uint8_t> data = buffer.getVector();
    uint32_t size = buffer.getSize();
    std::cout << "size = " << size << std::endl;

    int actual = m_socket->Send(&data[0], size, 0);
    std::cout << "send " << actual << " bytes, expected " << size << " bytes" << std::endl;
  }
}

/*
 * 上传报告信息
 */
void ClientApplication::SendReport () {
  //NS_LOG_FUNCTION (this);

  if (m_connected) {
    ByteBuffer buffer;
    buffer.put<uint16_t> (Report, sizeof (uint16_t));
    buffer.put<Address> (m_device->GetAddress(), sizeof (Address));

    Location loc = Location (m_mobility->GetPosition().x, m_mobility->GetPosition().y);
    buffer.put<Location>(loc, sizeof (Location));

    buffer.put<double> (Simulator::Now().GetSeconds(), sizeof (double));

    buffer.putString (std::string ("I am report video!"));
    std::vector<uint8_t> data = buffer.getVector();
    uint32_t size = buffer.getSize();
    std::cout << "size = " << size << std::endl;

    int actual = m_socket->Send(&data[0], size, 0);
    std::cout << "ReportInfo: send " << actual << " bytes, expected " << size << " bytes" << std::endl;
  }

}

void ClientApplication::ConnectionSucceeded (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this << socket);

  m_connected = true;
}

void ClientApplication::ConnectionFailed (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this << socket);

}

int ClientApplication::GetReportInterval () {
  //NS_LOG_FUNCTION (this);

  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (static_cast<int>(GetNode()->GetId() * Simulator::Now().GetSeconds()));//使运行标识与Node和Time相关联
  
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (1.0));
  x->SetAttribute ("Max", DoubleValue (10.0));
  
  int ret = x->GetInteger ();
  return ret;
}

int ClientApplication::GetInfoInterval () {
  //NS_LOG_FUNCTION (this);

  int ret = m_velProdInterval / (m_mobility->GetVelocity().x + m_mobility->GetVelocity().y + m_mobility->GetVelocity().z);
  return ret;
}

void ClientApplication::GetQueryFromSer (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this);

  Ptr<Packet> packet;
  while (packet = socket->Recv ()) {
    if (0 == packet->GetSize ()) { //EOF
      break;
    }

    uint32_t size = packet->GetSize();
    uint8_t* data = new uint8_t[size];
    packet->CopyData (data, size);
    ByteBuffer buffer (data, size);
    double time = buffer.get<double> ();
    
    std::cout << "get query time = " << time << std::endl;
    SendResponse ();
    delete data;
    data = NULL;
  }
}

void ClientApplication::SendResponse () {
  //NS_LOG_FUNCTION (this);
  
  if (m_connected) {
    ByteBuffer buffer;
    buffer.put<uint16_t> (Response, sizeof (uint16_t));
    buffer.put<Address> (m_device->GetAddress(), sizeof (Address));
    buffer.putString (std::string("I am response video!"));

    std::vector<uint8_t> data = buffer.getVector();
    uint32_t size = buffer.getSize();
    std::cout << "size = " << size << std::endl;

    int actual = m_socket->Send(&data[0], size, 0);
    std::cout << "Response: send " << actual << " bytes, expected " << size << " bytes" << std::endl;
  }
}

void ClientApplication::DataSend (Ptr<Socket>, uint32_t) {
  //NS_LOG_FUNCTION (this);

  if (m_connected) {
    //Simulator::Schedule (Seconds(GetInfoInterval()), &ClientApplication::SendInfo, this);
    Simulator::Schedule (Seconds(GetReportInterval()), &ClientApplication::SendReport, this);
  }
}

std::string ClientApplication::GetVideoFromStorage () {
  
}

}
