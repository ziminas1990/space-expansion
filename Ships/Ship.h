#pragma once

#include <vector>
#include <memory>

#include <Modules/BaseModule.h>
#include <Modules/Commutator/Commutator.h>
#include <Newton/PhysicalObject.h>
#include <Utils/GlobalContainer.h>

namespace ships
{

class Ship :
    public modules::BaseModule,
    public newton::PhysicalObject,
    public utils::GlobalContainer<Ship>
{
public:
  Ship(std::string const& sShipType, std::string sName, world::PlayerWeakPtr pOwner,
       double weight, double radius);
  ~Ship() override;

  bool loadState(YAML::Node const& source) override;

  bool installModule(modules::BaseModulePtr pModule);

  modules::CommutatorPtr getCommutator() const { return m_pCommutator; }

  // overrides from IProtobufTerminal interface
  void onMessageReceived(uint32_t nSessionId, spex::Message const& message) override;
  void attachToChannel(network::IPlayerChannelPtr pChannel) override;
  void detachFromChannel() override;

  modules::BaseModulePtr getModuleByName(std::string const& sName) const;
    // Return module with the specified 'sName'. Has O(log(N)) complicity.

protected:
  void handleShipMessage(uint32_t nSessionId, spex::IShip const& message) override;
  void handleNavigationMessage(
      uint32_t nSessionId, spex::INavigation const& message) override;

private:
  modules::CommutatorPtr m_pCommutator;
  std::map<std::string, modules::BaseModulePtr> m_Modules;
};

using ShipPtr     = std::shared_ptr<Ship>;
using ShipWeakPtr = std::weak_ptr<Ship>;

} // namespace modules
