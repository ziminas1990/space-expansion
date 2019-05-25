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
  Ship(std::string const& sShipType, double weight);
  ~Ship() override;

  void installModule(modules::BaseModulePtr pModule);

  // overrides from IProtobufTerminal interface
  void onMessageReceived(uint32_t nSessionId, spex::Message const& message) override;

protected:
  void handleNavigationMessage(uint32_t nSessionId,
                               spex::INavigation const& message) override;

private:
  modules::CommutatorPtr m_pCommutator;
  std::vector<modules::BaseModulePtr> m_Modules;
};

using ShipPtr     = std::shared_ptr<Ship>;
using ShipWeakPtr = std::weak_ptr<Ship>;

} // namespace modules
