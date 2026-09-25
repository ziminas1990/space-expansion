#pragma once

#include <Modules/BaseModule.h>
#include <Utils/GlobalContainer.h>
#include <Utils/UnorderedVector.h>
#include <Protocol.pb.h>

namespace modules {

class HoverEngine : public BaseModule, public utils::GlobalObject<HoverEngine>
{
public:
  HoverEngine(std::string&& sName, world::PlayerWeakPtr pOwner,
              uint32_t maxThrust);

  void proceed(uint32_t nIntervalUs) override;
  void onSessionClosed(uint32_t nSessionId) override;

protected:
  void handleHoverEngineMessage(
      uint32_t nSessionId, spex::IHoverEngine const& message) override;
  void onInstalled(modules::Ship* pPlatform) override;

  void getSpecification(uint32_t nSessionId) const;
  void setThrust(spex::IHoverEngine::ChangeThrust const& req);
  void monitor(uint32_t nSessionId);

private:
  bool sendThrust(uint32_t nSessionId) const;
  void notifyMonitors();
  void applyThrust();

private:
  size_t   m_nForceId      = size_t(-1);
  uint32_t m_maxThrust     = 0;
  uint32_t m_thrust        = 0;
  uint32_t m_nTimeLeftUs   = 0;

  utils::UnorderedVector<uint32_t> m_monitoringSessions;
};

} // namespace modules
