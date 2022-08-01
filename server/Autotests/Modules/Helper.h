#pragma once

#include <algorithm>

#include <Modules/All.h>
#include <World/Player.h>
#include <Autotests/TestUtils/Connector.h>
#include <Autotests/Modules/ModulesTestFixture.h>
#include <Autotests/ClientSDK/Router.h>
#include <Autotests/ClientSDK/Modules/ClientShip.h>
#include <Autotests/ClientSDK/Modules/ClientEngine.h>
#include <Autotests/ClientSDK/Modules/ClientCommutator.h>
#include <Utils/Linker.h>

#define DECLARE_ATTRIBUTE(Container, AttrType, AttrName, DefaultValue) \
  private: AttrType m_##AttrName = DefaultValue; \
  public: Container& AttrName(const AttrType& value) { \
    m_##AttrName = value; \
    return *this; \
  } \
  public: const AttrType& AttrName() const { \
    return m_##AttrName; \
  }


namespace autotests {

struct Connection {
  uint32_t                    m_nRootSessionId = 0;
  client::ClientCommutatorPtr m_pCommutatorCtrl;

  client::ClientCommutatorPtr operator->() const {
    return m_pCommutatorCtrl;
  }
};

template<typename ServerModule, typename ClientModule>
struct ModuleBind {
  std::shared_ptr<ServerModule> m_pRemote;
  modules::CommutatorPtr        m_pCommutator;
  uint32_t                      m_nSlotId;
  std::shared_ptr<ClientModule> m_pClient;

  std::shared_ptr<ClientModule> operator->() {
    return m_pClient;
  }
};

using ShipBinding = ModuleBind<modules::Ship, client::Ship>;
using EngineBinding = ModuleBind<modules::Engine, client::Engine>;

struct Helper {

  struct ShipParams {
    DECLARE_ATTRIBUTE(ShipParams, std::string, shipType, "SomeType");
    DECLARE_ATTRIBUTE(ShipParams, std::string, shipName, "SomeShip");
    DECLARE_ATTRIBUTE(ShipParams, double,      weight,   200000);
    DECLARE_ATTRIBUTE(ShipParams, double,      radius,   10);
  };

  struct EngineParams {
    DECLARE_ATTRIBUTE(EngineParams, std::string, name,     "SomeEngine");
    DECLARE_ATTRIBUTE(EngineParams, uint32_t,    maxThrust, 100000);
  };

  static Connection connect(ModulesTestFixture& env,
                            uint32_t            nConnectionId);

  static ShipBinding spawnShip(ModulesTestFixture&   env,
                               const Connection&     connection,
                               world::PlayerPtr      pOwner,
                               const geometry::Point position,
                               const ShipParams& params)
  {
    // Spawn a ship and attach it to commutator:
    modules::ShipPtr pShip = std::make_shared<modules::Ship>(
      params.shipType(), params.shipName(), pOwner, params.weight(),
      params.radius());
    pShip->moveTo(position);

    modules::CommutatorPtr pCommutator = pOwner->getCommutator();
    const uint32_t         nSlotId     = pCommutator->attachModule(pShip);
    
    // Create a ship on client side and connect it with server side
    client::ShipPtr pShipCtrl =
      std::make_shared<client::Ship>(env.m_pRouter);

    pShipCtrl->attachToChannel(connection->openSession(nSlotId));
    return {pShip, pCommutator, nSlotId, pShipCtrl};
  }

  static EngineBinding spawnEngine(ShipBinding ship, const EngineParams& params)
  {
    modules::EnginePtr pEngine = std::make_shared<modules::Engine>(
      std::string(params.name()),
      ship.m_pRemote->getOwner(),
      params.maxThrust()
    );

    modules::CommutatorPtr pCommutator = ship.m_pRemote->getCommutator();
    const uint32_t         nSlotId     = ship.m_pRemote->installModule(pEngine);
    client::EnginePtr      pEngineCtrl = std::make_shared<client::Engine>();
    pEngineCtrl->attachToChannel(ship->openSession(nSlotId));

    return {pEngine, pCommutator, nSlotId, pEngineCtrl};
  }

};

} // namespace autotests