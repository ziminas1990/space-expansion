#include "PassiveScanner.h"

#include <Utils/Clock.h>
#include <Newton/PhysicalObject.h>
#include <Ships/Ship.h>
#include <World/CelestialBodies/Asteroid.h>
#include <World/Grid.h>
#include <Utils/YamlReader.h>
#include <Modules/CommonModulesManager.h>

#include <math.h>

DECLARE_GLOBAL_CONTAINER_CPP(modules::PassiveScanner);

namespace modules
{

static void getTypeAndId(const newton::PhysicalObject* pObject,
                         spex::ObjectType&             eType,
                         uint32_t&                     nId) {
  switch(pObject->getType()) {
    case world::ObjectType::eAsteroid:
      eType = spex::ObjectType::OBJECT_ASTEROID;
      nId = static_cast<const world::Asteroid*>(pObject)->getAsteroidId();
      return;
    case world::ObjectType::eShip:
      eType = spex::ObjectType::OBJECT_SHIP;
      nId = static_cast<const ships::Ship*>(pObject)->getShipId();
      return;
    case world::ObjectType::eTotalObjectsTypes:
      assert(false);
    case world::ObjectType::eUnspecified:
    case world::ObjectType::ePhysicalObject:
    case world::ObjectType::eUnknown:
      eType = spex::ObjectType::OBJECT_UNKNOWN;
      break;
  }
  eType = spex::ObjectType::OBJECT_UNKNOWN;
  nId   = 0;
}

PassiveScanner::PassiveScanner(
    std::string&&        sName,
    world::PlayerWeakPtr pOwner,
    uint32_t             nMaxScanningRadiusKm,
    uint32_t             nMaxUpdateTimeMs)
  : BaseModule("PassiveScanner", std::move(sName), pOwner)
  , m_nMaxScanningRadius(nMaxScanningRadiusKm * 1000)
  , m_nMaxUpdateTimeUs(nMaxUpdateTimeMs * 1000)
  , m_nLastGlobalUpdateUs(0)
{
  GlobalObject<PassiveScanner>::registerSelf(this);
  m_nSessions.fill(0);
}

void PassiveScanner::proceed(uint32_t)
{
  const uint64_t nowUs = utils::GlobalClock::now();

  if (nowUs - m_nLastGlobalUpdateUs > m_nMaxUpdateTimeUs) {
    proceedGlobalScan();
    m_nLastGlobalUpdateUs = nowUs;
  }

  if (m_detectedObjects.empty()) {
    return;
  }

  // Collecting objects, that should be sent to client in this update
  // Send not more then 16 objects at a time
  const size_t updateLimit          = 16;
  size_t       totalObjectsToUpdate = 0;
  std::array<const newton::PhysicalObject*, updateLimit> objectsToUpdate;

  while (m_detectedObjects.front().m_nWhenToUpdate <= nowUs
         && totalObjectsToUpdate < updateLimit) {
    DetectedItem& item = m_detectedObjects.front();
    const newton::PhysicalObject* pObject =
        utils::GlobalContainer<newton::PhysicalObject>::Instance(
          item.m_nObjectId);
    if (pObject) {
      const auto [distance, updateTime] =
          getDistanceAndUpdateTime(*pObject, nowUs);
      item.m_nWhenToUpdate = updateTime;
      // Move element to keep vector sorted (one iteration of bubble sort)
      for (size_t i = 1; i < m_detectedObjects.size(); ++i) {
        if (m_detectedObjects[i - 1] < m_detectedObjects[i]) {
          break;
        } else {
          std::swap(m_detectedObjects[i - 1], m_detectedObjects[i]);
        }
      }

      if (distance < m_nMaxScanningRadius) {
        objectsToUpdate[totalObjectsToUpdate++] = pObject;
      } else {
        // Object is out of range
        m_detectedObjects.erase(m_detectedObjects.begin());
      }
    } else {
      // Remove item from array
      m_detectedObjects.erase(m_detectedObjects.begin());
    }
  }

  // Send updates to clients
  if (totalObjectsToUpdate) {
    spex::Message message;
    spex::IPassiveScanner::Update* update =
        message.mutable_passive_scanner()->mutable_update();
    for (const newton::PhysicalObject* pObject: objectsToUpdate) {
      spex::IPassiveScanner::ObjectData* pData = update->add_update();
      spex::ObjectType eType;
      uint32_t         nObjectId;
      getTypeAndId(pObject, eType, nObjectId);
      pData->set_object_type(eType);
      pData->set_id(nObjectId);
      pData->set_x(pObject->getPosition().x);
      pData->set_y(pObject->getPosition().y);
      pData->set_vx(static_cast<float>(pObject->getVelocity().getX()));
      pData->set_vy(static_cast<float>(pObject->getVelocity().getY()));
      pData->set_r(static_cast<float>(pObject->getRadius()));
    }

    for (const uint32_t nSession: m_nSessions) {
      if (nSession) {
        sendToClient(nSession, message);
      }
    }
  }
}

bool PassiveScanner::openSession(uint32_t nSessionId)
{
  for (size_t i = 0, end = m_nSessions.size(); i < end; ++i) {
    if (m_nSessions[i] == 0) {
      m_nSessions[i] = nSessionId;
      return true;
    }
  }
  return false;
}

void PassiveScanner::onSessionClosed(uint32_t nSessionId)
{
  bool lHasActiveSessions = false;
  for (size_t i = 0, end = m_nSessions.size(); i < end; ++i) {
    if (m_nSessions[i] == nSessionId) {
      m_nSessions[i] = 0;
    } else if (m_nSessions[i] != 0) {
      lHasActiveSessions = true;
    }
  }

  if (!lHasActiveSessions) {
    switchToIdleState();
  }
}

void PassiveScanner::handlePassiveScannerMessage(
    uint32_t nSessionId, const spex::IPassiveScanner &message)
{
  switch(message.choice_case()) {
    case spex::IPassiveScanner::kMonitor: {
      switchToActiveState();
      sendMonitorAck(nSessionId);
      return;
    }
    case spex::IPassiveScanner::kSpecificationReq: {
      sendSpecification(nSessionId);
      return;
    }
    default:
      assert("Unexpected choice case" == nullptr);
      return;
  }
}

void PassiveScanner::sendSpecification(uint32_t nSessionId)
{
  spex::Message message;
  spex::IPassiveScanner::Specification* spec =
      message.mutable_passive_scanner()->mutable_specification();
  spec->set_scanning_radius_km(m_nMaxScanningRadius / 1000);
  spec->set_max_update_time_ms(m_nMaxUpdateTimeUs / 1000);
  sendToClient(nSessionId, message);
}

void PassiveScanner::sendMonitorAck(uint32_t nSessionId)
{
  spex::Message message;
  message.mutable_passive_scanner()->set_monitor_ack(true);
  sendToClient(nSessionId, message);
}

void PassiveScanner::proceedGlobalScan()
{
  const double scanningAreaSize  = 2 * m_nMaxScanningRadius;
  const double scanningRadiusSqr = m_nMaxScanningRadius * m_nMaxScanningRadius;

  const world::Grid*     pGrid       = world::Grid::getGlobal();
  const geometry::Point& position    = getPlatform()->getPosition();
  const uint64_t         nowUs       = utils::GlobalClock::now();

  // Should sort 'm_detectedObjects' by objectsId in order to use binary
  // search
  auto compareById = [](const DetectedItem& left, const DetectedItem& right) {
    return left.m_nObjectId < right.m_nObjectId;
  };
  std::sort(m_detectedObjects.begin(), m_detectedObjects.end(), compareById);

  std::vector<DetectedItem> oldData = std::move(m_detectedObjects);
  std::vector<DetectedItem> newData;
  newData.reserve(oldData.capacity());

  world::Grid::iterator itCell = pGrid->range(
        position.x,
        position.y,
        scanningAreaSize,
        scanningAreaSize);

  for (world::Grid::iterator end = pGrid->end(); itCell != end; ++itCell) {
    for (uint32_t nObjectId: itCell->getObjects().data()) {
      const newton::PhysicalObject* pObject =
          utils::GlobalContainer<newton::PhysicalObject>::Instance(nObjectId);

      if (pObject &&
          position.distanceSqr(pObject->getPosition()) < scanningRadiusSqr) {
        newData.push_back(DetectedItem{
                            getDistanceAndUpdateTime(*pObject, nowUs).second,
                            nObjectId
                          });
      }
    }
  }
  std::sort(newData.begin(), newData.end(), compareById);

  // Now 'm_detectedObjects' has a new set of objects and 'oldData' has a
  // previous one. Both are sorted by Id. We should copy update time from
  // 'oldData' items to corresponding 'newData' items.
  // Since both are sorted, we may use kind of 'merge sorting' approach
  size_t       i    = 0;  // iterate through oldData
  const size_t iEnd = oldData.size();
  size_t       j    = 0;  // iterate through newData
  const size_t jEnd = newData.size();

  while(i != iEnd || j != jEnd) {
    const DetectedItem& oldItem = oldData[i];
    DetectedItem&       newItem = newData[j];
    if (oldItem.m_nObjectId == newItem.m_nObjectId) {
      newItem.m_nWhenToUpdate = oldItem.m_nWhenToUpdate;
      ++i;
      ++j;
    } else if (oldItem.m_nObjectId < newItem.m_nObjectId) {
      ++i;
    } else {
      ++j;
    }
  }

  // 'm_detectedObjects' should be sorted by update time
  std::sort(newData.begin(), newData.end());
  m_detectedObjects = std::move(newData);
}

std::pair<double, uint64_t> PassiveScanner::getDistanceAndUpdateTime(
    const newton::PhysicalObject &other, uint64_t now) const
{
  const geometry::Point& position      = getPlatform()->getPosition();
  geometry::Point        otherPosition = other.getPosition();

  const double distance = otherPosition.distance(position);
  uint64_t     dtUs     = static_cast<uint64_t>(
        m_nMaxUpdateTimeUs * sqrt(distance / m_nMaxScanningRadius));

  // Predict where object will be at 'now + dtUs'
  const double dtSec = dtUs / 1000000.0;
  otherPosition.translate(other.getVelocity() * dtSec);

  // Recalculating update time
  const double predictedDistance = otherPosition.distance(position);
  dtUs = static_cast<uint64_t>(
        m_nMaxUpdateTimeUs * sqrt(predictedDistance / m_nMaxScanningRadius));
  if (dtUs < static_cast<uint64_t>(Cooldown::ePassiveScanner)) {
    dtUs = static_cast<uint64_t>(Cooldown::ePassiveScanner);
  }
  return std::make_pair(distance, now + dtUs);
}

} // namespace modules
