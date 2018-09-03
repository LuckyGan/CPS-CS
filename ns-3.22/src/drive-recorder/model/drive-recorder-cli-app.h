#ifndef DRIVE_RECORDER_CLI_APP_H
#define DRIVE_RECORDER_CLI_APP_H

#include "ns3/applications-module.h"
#include "drive-recorder-util.h"

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class ClientApplication : public Application {
public:
  static TypeId GetTypeId ();

  ClientApplication ();
  ClientApplication (Ptr<Node> node);
  virtual ~ClientApplication ();
  
  Ptr<Socket> GetSocket () const;
  
protected:
private:
  virtual void StartApplication ();
  virtual void StopApplication ();
  
  Ptr<Socket> m_socket;
  Address     m_peer;
  bool        m_connected;
  TypeId      m_tid;

private:
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);

private:
  //const Time m_infoTime;    //上传节点信息间隔时间
  int m_velProdInterval;    //节点速度和上传节点信息间隔时间的乘积
  Ptr<NetDevice> m_device;
  Ptr<MobilityModel> m_mobility;

private:
  void DataSend (Ptr<Socket>, uint32_t);
  void SendInfo ();
  void SendReport ();
  int GetReportInterval ();
  int GetInfoInterval ();
  void GetQueryFromSer (Ptr<Socket> socket);
  void SendResponse ();
  std::string GetVideoFromStorage ();
};

}

#endif

