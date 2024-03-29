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
#include <Autotests/ClientSDK/Modules/ClientMessanger.h>
#include <Autotests/ClientSDK/RootSession.h>
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

template<typename ServerModule, typename ClientModule>
struct ModuleBind {
  std::shared_ptr<ServerModule> m_pRemote;
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

  static client::RootSessionPtr connect(ModulesTestFixture& env,
                                        uint32_t            nConnectionId);

  static client::ClientCommutatorPtr
  openCommutatorSession(ModulesTestFixture& env,
                        client::RootSessionPtr pRootSession);

  static ShipBinding spawnShip(
      ModulesTestFixture&         env,
      client::ClientCommutatorPtr pClientCommutator,
      const geometry::Point       position,
      const ShipParams&           params,
      world::PlayerPtr            pOwner = nullptr)
  {
    pOwner = pOwner ? pOwner : env.m_pPlayer;

    // Spawn a ship on server side and attach it to player's commutator:
    modules::ShipPtr pShip = std::make_shared<modules::Ship>(
      params.shipType(), params.shipName(), pOwner, params.weight(),
      params.radius());
    pShip->moveTo(position);

    const uint32_t nSlotId = pOwner->onNewShip(pShip);

    // Create a ship on client side and connect it with server side
    client::ShipPtr pShipCtrl =
      std::make_shared<client::Ship>(env.m_pRouter);

    pShipCtrl->attachToChannel(pClientCommutator->openSession(nSlotId));
    return { pShip, nSlotId, pShipCtrl };
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

    return {pEngine, nSlotId, pEngineCtrl};
  }

  static void createMessangerModule(
    ModulesTestFixture& env,
    world::PlayerPtr    pOwner = nullptr)
  {
    pOwner = pOwner ? pOwner : env.m_pPlayer;

    // Messanger should be created on "server" side and attached to player's
    // commutator
    modules::MessangerPtr pMessanger = std::make_shared<modules::Messanger>(
      "Messanger", pOwner
    );

    env.m_pPlayer->testAccess().setMessanger(pMessanger);
    // Now messanger can be reached using client commutator
    // (use Helper::getMessanger() call)
  }

  static client::MessangerPtr getMessanger(client::ClientCommutatorPtr pCommutator)
  {
    client::Router::SessionPtr pSession = Helper::openSession(pCommutator, "Messanger");

    if (pSession) {
      client::MessangerPtr pMessanger = std::make_shared<client::Messanger>();
      pMessanger->attachToChannel(pSession);
      return pMessanger;
    }

    return client::MessangerPtr();
  }

  static client::Router::SessionPtr openSession(
    client::ClientCommutatorPtr pCommutator, std::string_view sModuleName)
  {
    client::ModulesList modules;

    if (pCommutator->getAttachedModulesList(modules)) {
      for (const client::ModuleInfo& attachedModule : modules) {
        if (attachedModule.sModuleName == sModuleName) {
          return pCommutator->openSession(attachedModule.nSlotId);
        }
      }
    }

    return client::Router::SessionPtr();
  }

};

} // namespace autotests