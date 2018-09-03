#ifndef DRIVE_RECORDER_UTIL_H
#define DRIVE_RECORDER_UTIL_H

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"

namespace ns3 {

  const uint16_t serPort = 8080;
  const std::string serIpv4Address = "10.1.1.1";
  const double nearbyDistance = 11.0;
  enum cli2SerSendType {Cycle, Report, Response};

typedef struct Location {
  double m_x;
  double m_y;
  Location ()
    : m_x (0),
      m_y (0) {
  }
  Location (double x, double y)
    : m_x (x),
      m_y (y) {
  }
}Location;

typedef struct ReportInfo {
  struct Location m_loc;
  double m_time;
  ReportInfo ()
    : m_loc (),
      m_time (0) {
  }
  ReportInfo (struct Location loc, double time)
    : m_loc (loc),
      m_time (time) {
  }
}ReportInfo;

class ByteBuffer {
public:

  ByteBuffer (int size) {
    buffer = std::vector<uint8_t> ();
    buffer.reserve(size);
    clear();
  }

  ByteBuffer () {
    buffer = std::vector<uint8_t> ();
    clear();
  }

  ByteBuffer (uint8_t *arr, uint32_t size) {
    buffer = std::vector<uint8_t> ();
    if (NULL == arr) {
      buffer.reserve (size);
      clear ();
    } else {
      buffer.reserve (size);
      clear ();
      for (uint idx = 0; idx < size; ++idx) {
        put<uint8_t> (arr[idx]);
      }
      pos = 0;
    }
  }

  template<typename T> void put (T data) {
    uint32_t dataSize = sizeof(data);

    if (buffer.size() < pos + dataSize) {
      buffer.resize (pos + dataSize);
    }

    memcpy (&buffer[pos], (uint8_t*)&data, dataSize);
    pos += dataSize;
  }

  template<typename T> void put (T data, uint32_t size) {
    buffer.resize (pos + size);
    memcpy (&buffer[pos], (uint8_t*)&data, size);
    pos += size;
  }
  
  void putString (std::string str) {
    uint32_t size = strlen (str.c_str()) + 1;
    buffer.resize (pos + size);
    memcpy (&buffer[pos], (uint8_t*)str.c_str(), size);
    pos += size;
  }
  
  template<typename T> T get () {
    pos += sizeof(T);
    return *((T*) &buffer[pos - sizeof(T)]);
  }
  
  template<typename T> T get (uint32_t size) {
    pos += size;
    return *((T*) &buffer[pos - size]);
  }

  std::string getString (uint32_t size) {
    pos += size;
    return std::string((const char*)&buffer[pos - size]);
  }
  
  std::vector<uint8_t> getVector () {
    return buffer;
  } 

  uint32_t getSize () {
    return buffer.size();
  }

  ~ByteBuffer () {
  }
private:
  std::vector<uint8_t> buffer;
  uint32_t pos;
  
  void clear () {
    pos = 0;
    buffer.clear();
  }
};


  Ipv4Address BuildIpv4Address (const uint32_t byte0, const uint32_t byte1, const uint32_t byte2, const uint32_t byte3);
  void SetIpv4Address (const Ptr<NetDevice> device, const Ipv4Address address, const Ipv4Mask mask);
  Ipv4Address GetIpv4Address (const Ptr<NetDevice> device);

  void SetPosition (const Ptr<Node> node, const Vector& position);
  void SetPositionVelocity (const Ptr<Node> node, const Vector& position, const Vector& velocity);
  
  double GetDistance (struct Location loc1, struct Location loc2);

}

#endif /* DRIVE_RECORDER_UTIL_H */
