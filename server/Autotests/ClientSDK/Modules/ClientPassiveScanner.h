#pragma once

#include <memory>
#include <Protocol.pb.h>
#include <Autotests/ClientSDK/Interfaces.h>
#include <Autotests/ClientSDK/ClientBaseModule.h>
#include <Modules/BaseModule.h>
#include <World/ObjectTypes.h>
#include <Geometry/Point.h>
#include <Geometry/Vector.h>

namespace autotests { namespace client {

class ClientPassiveScanner : public ClientBaseModule
{
public:
  struct ObjectData {
    world::ObjectType m_eType;
    uint32_t          m_nObjectId;
    geometry::Point   m_position;
    geometry::Vector  m_velocity;
    double            m_raduis;
  };

  struct Specification {
    uint32_t m_nScanningRadiusKm;
    uint32_t m_nMaxUpdateTimeMs;
  };

public:
  ClientPassiveScanner();

  bool sendSpecificationReq();
  bool sendMonitor();

  bool waitSpecification(Specification& spec);
  bool waitMonitorAck();
  bool waitUpdate(std::vector<ObjectData>& update);

  bool pickUpdate(std::vector<ObjectData>& update);
  // Store to 'update' only updates, that are already in receive
  // buffer (do not wait). Return true if there is at least one update,
  // otherwise return false.
};

using ClientPassiveScannerPtr = std::shared_ptr<ClientPassiveScanner>;

}} // namespace autotests::client
