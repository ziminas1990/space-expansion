#include "ClientPassiveScanner.h"
#include <Protocol.pb.h>

namespace autotests { namespace client {

world::ObjectType convert(spex::ObjectType type) {
  switch(type) {
    case spex::ObjectType::OBJECT_SHIP:
      return world::ObjectType::eShip;
    case spex::ObjectType::OBJECT_ASTEROID:
      return world::ObjectType::eAsteroid;
    case spex::ObjectType::OBJECT_UNKNOWN:
      return world::ObjectType::ePhysicalObject;
    default:
      assert("Not supported" == nullptr);
      return world::ObjectType::eUnknown;
  }
}

ClientPassiveScanner::ClientPassiveScanner()
{}

bool ClientPassiveScanner::sendSpecificationReq()
{
  spex::Message message;
  message.mutable_passive_scanner()->set_specification_req(true);
  return send(message);
}

bool ClientPassiveScanner::sendMonitor()
{
  spex::Message message;
  message.mutable_passive_scanner()->set_monitor(true);
  return send(message);
}

bool ClientPassiveScanner::waitSpecification(
    ClientPassiveScanner::Specification &spec)
{
  spex::IPassiveScanner message;
  if (!wait(message)) {
    return false;
  }
  if (message.choice_case() != spex::IPassiveScanner::kSpecification) {
    return false;
  }

  spec.m_nScanningRadiusKm = message.specification().scanning_radius_km();
  spec.m_nMaxUpdateTimeMs  = message.specification().max_update_time_ms();
  return true;
}

bool ClientPassiveScanner::waitMonitorAck()
{
  spex::IPassiveScanner message;
  return wait(message)
      && message.choice_case() == spex::IPassiveScanner::kMonitorAck;
}

bool ClientPassiveScanner::waitUpdate(std::vector<ObjectData> &update)
{
  update.clear();

  spex::IPassiveScanner message;
  if (!wait(message)) {
    return false;
  }
  if (message.choice_case() != spex::IPassiveScanner::kUpdate) {
    return false;
  }

  for (const spex::IPassiveScanner::ObjectData& item:
       message.update().update()) {
    ObjectData itemData{
      convert(item.object_type()),
      item.id(),
      geometry::Point(item.x(), item.y()),
      geometry::Vector(static_cast<double>(item.vx()),
                       static_cast<double>(item.vy())),
      static_cast<double>(item.r())
    };
    update.push_back(itemData);
  }
  return true;
}

}} // namespace autotests::client
