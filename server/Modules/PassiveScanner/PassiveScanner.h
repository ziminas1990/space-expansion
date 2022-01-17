#pragma once

#include <memory>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/YamlForwardDeclarations.h>
#include <Protocol.pb.h>
#include <Geometry/Point.h>

namespace newton {
class PhysicalObject;
}

namespace modules {

class PassiveScanner :
    public BaseModule,
    public utils::GlobalObject<PassiveScanner>
{
public:
  PassiveScanner(std::string&& sName,
                 world::PlayerWeakPtr pOwner,
                 uint32_t nMaxScanningRadiusKm,
                 uint32_t nMaxUpdateTimeMs);

  void reset();

  void proceed(uint32_t nIntervalUs) override;

  // Override from BaseModule->IProtobufTerminal
  bool openSession(uint32_t) override { return true; }
  void onSessionClosed(uint32_t nSessionId) override;

private:
  void handlePassiveScannerMessage(
      uint32_t nSessionId, spex::IPassiveScanner const& message) override;

  void handleMonitorReq(uint32_t nSessionId);

  void sendSpecification(uint32_t nSessionId);
  void sendMonitorAck(uint32_t nSessionId, bool status);

  void proceedGlobalScan();

  std::pair<double, uint64_t> getDistanceAndUpdateTime(
      const newton::PhysicalObject& other, uint64_t now) const;

private:
  uint32_t m_nMaxScanningRadius;
  uint32_t m_nMaxUpdateTimeUs;

  uint64_t                m_nLastGlobalUpdateUs;
  std::array<uint32_t, 8> m_nMonitoringSessions;

  struct DetectedItem {
    uint64_t m_nWhenToUpdate;
    uint32_t m_nObjectId;

    bool operator<(const DetectedItem& other) const {
      return m_nWhenToUpdate < other.m_nWhenToUpdate;
    }
  };
  std::vector<DetectedItem> m_detectedObjects;
};

using PassiveScannerPtr = std::shared_ptr<PassiveScanner>;

} // namespace modules
