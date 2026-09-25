#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/UnorderedVector.h>
#include <Protocol.pb.h>

namespace modules {

class RCS : public BaseModule, public utils::GlobalObject<RCS>
{
public:
  RCS(std::string&& sName, world::PlayerWeakPtr pOwner, uint32_t maxThrust);

  void proceed(uint32_t nIntervalUs) override;

  bool loadState(YAML::Node const& source) override;
  void onSessionClosed(uint32_t nSessionId) override;

protected:
  // override from BaseModule
  void handleRCSMessage(uint32_t, spex::IRCS const&) override;
  void onInstalled(modules::Ship* pPlatform) override;

  void getSpecification(uint32_t nSessionId) const;
  void setThrust(spex::IRCS::ChangeThrust const& req);
  void getThrust(uint32_t nSessionId) const;
  void monitor(uint32_t nSessionId);

private:
  bool sendThrust(uint32_t nSessionId) const;
  void notifyMonitors();

private:
  size_t   m_nThrustVectorId = size_t(-1);
  uint32_t m_maxThrust       = 0;
  uint32_t m_nTimeLeftUs     = 0;

  utils::UnorderedVector<uint32_t> m_monitoringSessions;
};

} // namespace modules
