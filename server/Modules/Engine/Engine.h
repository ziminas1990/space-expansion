#pragma once

#include <memory>
#include <Newton/PhysicalObject.h>
#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/UnorderedVector.h>
#include <Protocol.pb.h>

namespace modules {

class Engine : public BaseModule, public utils::GlobalObject<Engine>
{
public:
  Engine(std::string&& sName, world::PlayerWeakPtr pOwner, uint32_t maxThrust);

  void proceed(uint32_t nIntervalUs);

  bool loadState(YAML::Node const& source) override;
  void onSessionClosed(uint32_t nSessionId) override;

protected:
  // override from BaseModule
  void handleEngineMessage(uint32_t, spex::IEngine const&) override;
  void onInstalled(modules::Ship* pPlatform) override;

  void getSpecification(uint32_t nSessionId) const;
  void setThrust(spex::IEngine::ChangeThrust const& req);
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
