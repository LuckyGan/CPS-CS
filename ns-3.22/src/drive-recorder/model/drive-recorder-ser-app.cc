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
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "drive-recorder-ser-app.h"
#include "drive-recorder-util.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ServerApplication");

NS_OBJECT_ENSURE_REGISTERED (ServerApplication);

TypeId 
ServerApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ServerApplication")
    .SetParent<Application> ()
    .AddConstructor<ServerApplication> ()
    .AddAttribute ("Local",
                   "The Address on which to Bind the rx socket.",
                   AddressValue (),
                   MakeAddressAccessor (&ServerApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol",
                   "The type id of the protocol to use for the rx socket.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&ServerApplication::m_tid),
                   MakeTypeIdChecker ())
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&ServerApplication::m_rxTrace),
                     "ns3::Packet::PacketAddressTracedCallback")
  ;
  return tid;
}

ServerApplication::ServerApplication () {
  //NS_LOG_FUNCTION (this);

  m_socket = 0;
  m_totalRx = 0;
}

ServerApplication::~ServerApplication()
{
  //NS_LOG_FUNCTION (this);
}

uint32_t ServerApplication::GetTotalRx () const {
  //NS_LOG_FUNCTION (this);

  return m_totalRx;
}

Ptr<Socket> ServerApplication::GetListeningSocket (void) const {
  //NS_LOG_FUNCTION (this);

  return m_socket;
}

std::list<Ptr<Socket> > ServerApplication::GetAcceptedSockets (void) const {
  //NS_LOG_FUNCTION (this);

  return m_socketList;
}

void ServerApplication::DoDispose (void) {
  //NS_LOG_FUNCTION (this);

  m_socket = 0;
  m_socketList.clear ();

  // chain up
  Application::DoDispose ();
}


// Application Methods
void ServerApplication::StartApplication () {   // Called at time specified by Start
  //NS_LOG_FUNCTION (this);

  if (!m_socket) {
    m_socket = Socket::CreateSocket (GetNode (), m_tid);
    m_socket->Bind (m_local);
    m_socket->Listen ();
    //m_socket->ShutdownSend ();
/*
    if (addressUtils::IsMulticast (m_local)) {
      Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
      if (udpSocket) {
          // equivalent to setsockopt (MCAST_JOIN_GROUP)
        udpSocket->MulticastJoinGroup (0, m_local);
      } else {
        NS_FATAL_ERROR ("Error: joining multicast on a non-UDP socket");
      }
    }
*/
  }

  //m_socket->SetRecvCallback (MakeCallback (&ServerApplication::HandleRead, this));
  //m_socket->SetAcceptCallback (
    //MakeCallback (&ServerApplication::HandleAcceptTest, this),
    //MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    //MakeCallback (&ServerApplication::HandleAccept, this));

  m_socket->SetAcceptCallback (
    MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&ServerApplication::HandleAccept, this));
  //m_socket->SetCloseCallbacks (
  //  MakeCallback (&ServerApplication::HandlePeerClose, this),
  //  MakeCallback (&ServerApplication::HandlePeerError, this));
}

void ServerApplication::StopApplication () {    // Called at time specified by Stop
  //NS_LOG_FUNCTION (this);

  while(!m_socketList.empty ()) {//these are accepted sockets, close them
    Ptr<Socket> acceptedSocket = m_socketList.front ();
    m_socketList.pop_front ();
    acceptedSocket->Close ();
  }
  if (m_socket) {
    m_socket->Close ();
    m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void ServerApplication::HandleRead (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  while (packet = socket->Recv ()) {
    if (packet->GetSize () == 0) { //EOF
      break;
    }
    uint32_t size = packet->GetSize();
    uint8_t* data = new uint8_t[size];
    packet->CopyData (data, size);
    ByteBuffer buffer (data, size);
    enum cli2SerSendType type = (enum cli2SerSendType)buffer.get<uint16_t>(sizeof (uint16_t));
    if (Cycle == type) {
      Address mac = buffer.get<Address> (sizeof(Address));
      Location loc = buffer.get<Location> (sizeof(Location));
      cliInfo[mac] = std::make_pair<Location, Ptr<Socket> >(loc, socket);
    } else if (Report == type) {
      Address mac = buffer.get<Address> (sizeof (Address));
      Location loc = buffer.get<Location> (sizeof (Location));
      double time = buffer.get<double> (sizeof (double));
      std::string video = buffer.getString (size - sizeof (Address) - sizeof (Location) - sizeof (double));
      std::cout << "report video = " << video << std::endl;

      ReportInfo tmpReportInfo = ReportInfo(loc, time);
      reportInfo.insert(std::make_pair<Address, ReportInfo> (mac, tmpReportInfo));

      std::vector<Ptr<Socket> > sockets = GetNearbyCli (mac, loc);
      std::cout << "sockets size = " << sockets.size() << std::endl;
      SendQueryToCli (sockets, time);

    } else if (Response == type) {
      Address mac = buffer.get<Address> (sizeof (Address));
      std::cout << "response's mac = " << mac << std::endl;

      std::string video = buffer.getString (size - sizeof (Address));
      std::cout << "response video = " << video << std::endl;
    }
    delete data;
    data = NULL;
  }
}


void ServerApplication::HandlePeerClose (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this << socket);
}
 
void ServerApplication::HandlePeerError (Ptr<Socket> socket) {
  //NS_LOG_FUNCTION (this << socket);
}
 

void ServerApplication::HandleAccept (Ptr<Socket> s, const Address& from) {
  //NS_LOG_FUNCTION (this << s << from);

  s->SetRecvCallback (MakeCallback (&ServerApplication::HandleRead, this));
  m_socketList.push_back (s);
  
}

std::vector<Ptr<Socket> > ServerApplication::GetNearbyCli (Address address, Location loc) {
  //NS_LOG_FUNCTION (this);

  std::cout << address << std::endl;
  std::cout << "----cliInfo-------" << std::endl;
  std::map<Address, std::pair<struct Location, Ptr<Socket> > >::iterator iter2 = cliInfo.begin();
  while (iter2 != cliInfo.end()) {
    std::cout << iter2->first << std::endl;
    ++iter2;
  }
  std::cout << "----cliInfo-------" << std::endl;

  std::vector<Ptr<Socket> > ret;
  std::map<Address, std::pair<struct Location, Ptr<Socket> > >::iterator iter = cliInfo.begin ();
  while (iter != cliInfo.end()) {
    double dist = GetDistance (loc, iter->second.first);
    std::cout << "Address(" << iter->first << ") from address(" << address << ") is " << dist << std::endl;  
    if (dist <= nearbyDistance && address != iter->first) {
      ret.push_back (iter->second.second);
    }
    ++iter;
  }
  return ret;
}

void ServerApplication::SendQueryToCli (std::vector<Ptr<Socket> > sockets, double time) {
  //NS_LOG_FUNCTION (this);

  for (int idx = 0; idx < static_cast<int>(sockets.size()); ++idx) {
    ByteBuffer buffer;
    buffer.put<double> (time, sizeof (double));
    std::vector<uint8_t> data = buffer.getVector();
    uint32_t size = buffer.getSize();

    sockets[idx]->Send(&data[0], size, 0);
  }
}

} // Namespace ns3
