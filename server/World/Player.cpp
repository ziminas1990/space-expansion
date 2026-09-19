#include "Player.h"
#include "Network/Fwd.h"
#include <Utils/YamlReader.h>
#include <Utils/Roman.h>
#include <Utils/StringUtils.h>
#include <yaml-cpp/yaml.h>

#include <cstdint>
#include <unordered_set>

#include <Modules/BlueprintsStorage/BlueprintsStorage.h>
#include <Modules/Commutator/Commutator.h>
#include <Modules/Messanger/Messanger.h>
#include <Modules/Ship/Ship.h>
#include <Network/ProtobufChannel.h>
#include <Network/UdpSocket.h>
#include <Network/SessionMux.h>
#include <Modules/SystemClock/SystemClock.h>
#include <Modules/Game/Game.h>
#include <Network/UdpSocket.h>
#include <Network/SessionMux.h>
#include <Blueprints/Ships/ShipBlueprint.h>

namespace world
{

namespace {

bool isAttachedShipNameTaken(const modules::Commutator &commutator,
                             const std::string &sName) {
  for (const modules::BaseModulePtr &pModule : commutator.getAllModules()) {
    if (!pModule || pModule->isDestroyed()) {
      continue;
    }
    if (pModule->getModuleType() != modules::Ship::TypeName()) {
      continue;
    }
    if (pModule->getModuleName() == sName) {
      return true;
    }
  }
  return false;
}

void ensureUniqueName(const modules::Commutator &commutator,
                      modules::Ship &ship) {
  const std::string &sRequested = ship.getModuleName();
  if (!isAttachedShipNameTaken(commutator, sRequested)) {
    return;
  }

  std::unordered_set<std::string> occupied;
  for (const modules::BaseModulePtr &pModule : commutator.getAllModules()) {
    if (!pModule || pModule->isDestroyed()) {
      continue;
    }
    if (pModule->getModuleType() != modules::Ship::TypeName()) {
      continue;
    }
    occupied.insert(pModule->getModuleName());
  }

  const char chSeparator =
      sRequested.find(' ') != std::string::npos ? ' ' : '-';
  for (unsigned n = 1;; ++n) {
    std::string sCandidate = sRequested;
    sCandidate.push_back(chSeparator);
    if (n < 1000) {
      sCandidate += utils::toRoman(static_cast<uint16_t>(n));
    } else {
      sCandidate += std::to_string(n);
    }
    if (occupied.find(sCandidate) == occupied.end()) {
      ship.changeModuleName(std::move(sCandidate));
      return;
    }
  }
}

} // namespace

class RootSession : public network::IPlayerTerminal  {
private:
  network::SessionMuxWeakPtr m_pSessionMuxWeakPtr;
  modules::CommutatorWeakPtr m_pRootCommutatorWeakPtr;

  // Why do we need mutex here? If two messages are sent in different UDP
  // connections, they can be handled in different threads. Since
  // RootSession handles messages immediatelly (it doesn't have any buffers),
  // races are possible here.
  utils::Mutex               m_mutex;

public:
  RootSession(network::SessionMuxPtr pSessionMux,
              modules::CommutatorPtr pRootCommutator)
    : m_pSessionMuxWeakPtr(pSessionMux)
    , m_pRootCommutatorWeakPtr(pRootCommutator)
  {}

  // Overrides of IPlayerTerminal
  bool canOpenSession() const override { return true; }
  void openSession(uint32_t) override {}

  void onMessageReceived(uint32_t nSessionId, spex::Message const& message) override {
    if (message.choice_case() == spex::Message::kRootSession) {
      switch(message.root_session().choice_case()) {
        case spex::IRootSession::kNewCommutatorSession: {
          open_commutator_session(nSessionId);
          return;
        }
        default: {
          return;
        }
      }
    }
  }

  void onSessionClosed(uint32_t) override {}
  void attachToChannel(network::IPlayerChannelPtr) override {
    assert(!"Operation makes no sense: RootSession always uses SessionMux as"
           " a channel");
  }
  void detachFromChannel() override {
    assert(!"Operation makes no sense");
  }

private:
  void open_commutator_session(uint32_t nSessionId)
  {
    std::lock_guard<utils::Mutex> guard(m_mutex);
    network::SessionMuxPtr pSessionMux = m_pSessionMuxWeakPtr.lock();
    modules::CommutatorPtr pCommutator = m_pRootCommutatorWeakPtr.lock();

    const uint32_t nChildSessionId =
      pSessionMux->createSession(nSessionId, pCommutator);

    spex::Message message;
    spex::IRootSession* pBody = message.mutable_root_session();
    pBody->set_commutator_session(nChildSessionId);
    pSessionMux->asChannel()->send(nSessionId, std::move(message));
  }
};


Player::Player(std::string&& sLogin,
               blueprints::BlueprintsLibrary&& blueprints)
  : m_sLogin(std::move(sLogin)),
    m_pSessionMux(std::make_shared<network::SessionMux>()),
    m_pRootCommutator(std::make_shared<modules::Commutator>(m_pSessionMux)),
    m_pRootSession(std::make_shared<RootSession>(m_pSessionMux, m_pRootCommutator)),
    m_blueprints(std::move(blueprints))
{
  m_linker.link(m_pSessionMux, m_pRootCommutator);
}

PlayerPtr Player::load(
    std::string sLogin,
    blueprints::BlueprintsLibrary blueprints,
    YAML::Node const& state)
{
  // Can't use 'std::make_shared' here since Player's constructor is private
  PlayerPtr pPlayer =
      std::shared_ptr<Player>(
        new Player(std::move(sLogin), std::move(blueprints)));

  if (!utils::YamlReader(state).read("password", pPlayer->m_sPassword)) {
    assert(!"Password is not specified");
    return PlayerPtr();
  }

  pPlayer->m_pSystemClock =
      std::make_shared<modules::SystemClock>("SystemClock", pPlayer);
  pPlayer->m_pBlueprintsExplorer = std::make_shared<modules::BlueprintsStorage>(pPlayer);
  pPlayer->m_pMessanger = std::make_shared<modules::Messanger>("Messanger", pPlayer);
  pPlayer->m_pGame = std::make_shared<modules::Game>("Game", pPlayer);

  pPlayer->m_linker.attachModule(pPlayer->m_pRootCommutator, pPlayer->m_pSystemClock);
  pPlayer->m_linker.attachModule(pPlayer->m_pRootCommutator,
                                 pPlayer->m_pBlueprintsExplorer);
  pPlayer->m_linker.attachModule(pPlayer->m_pRootCommutator, pPlayer->m_pMessanger);
  pPlayer->m_linker.attachModule(pPlayer->m_pRootCommutator, pPlayer->m_pGame);

  YAML::Node const& shipsState = state["ships"];
  if (!shipsState.IsDefined()) {
    // Player has no ships (proval!)
    return pPlayer;
  }

  for (auto const& kv : shipsState) {
    std::string sShipTypeAndName = kv.first.as<std::string>();
    std::string sShipName;
    std::string sShipType;
    utils::StringUtils::split('/', sShipTypeAndName, sShipType, sShipName);
    assert(!sShipName.empty() && !sShipType.empty());

    blueprints::BaseBlueprintPtr pShipBlueprint =
        pPlayer->m_blueprints.getBlueprint(
          blueprints::BlueprintName("Ship", sShipType));
    assert(pShipBlueprint);
    if (!pShipBlueprint)
      return PlayerPtr();

    modules::ShipPtr pShip =
        std::static_pointer_cast<modules::Ship>(
          pShipBlueprint->build(std::move(sShipName), pPlayer));
    assert(pShip);
    if (!pShip) {
      return PlayerPtr();
    }

    if (!pShip->loadState(kv.second)) {
      assert(!"Failed to load ship");
      return PlayerPtr();
    }

    pPlayer->onNewShip(std::move(pShip));
  }

  return pPlayer;
}

PlayerPtr Player::makeDummy(std::string sLogin)
{
  // Can't use 'std::make_shared' here since Player's constructor is private
  return std::shared_ptr<Player>(
        new Player(std::move(sLogin), blueprints::BlueprintsLibrary()));
}

uint32_t Player::onNewConnection(uint32_t nConnectionId)
{
  // Each UDP connection starts with a root session, that is attached to
  // 'RootSession' handler.
  return m_pSessionMux->addConnection(nConnectionId, m_pRootSession);
}

uint32_t Player::onNewShip(modules::ShipPtr pShip)
{
  if (!pShip) {
    return modules::Commutator::invalidSlot();
  }

  if (pShip->getModuleName().empty()) {
    pShip->changeModuleName(pShip->getBlueprintName());
  }

  std::lock_guard<utils::Mutex> guard(m_addNewShipMutex);
  ensureUniqueName(*m_pRootCommutator, *pShip);
  return m_linker.attachModule(m_pRootCommutator, pShip);
}

void Player::attachToUdpSocket(network::UdpSocketPtr pSocket)
{
  m_pUdpChannel = pSocket;
  if (!m_pProtobufChannel) {
    m_pProtobufChannel = std::make_shared<network::PlayerChannel>();
    m_linker.link(m_pProtobufChannel, m_pSessionMux->asTerminal());
  }
  m_linker.link(m_pUdpChannel, m_pProtobufChannel);
  // Add a custom unlinker logic, that closes all active connections, otherwise
  // SessionMux will raise an assert error in it's descructor
  m_linker.addCustomUnlinker([this]() {
    m_pSessionMux->markAllConnectionsAsClosed();
  });
}

uint32_t Player::TestAccessor::setMessanger(modules::MessangerPtr pMessanger) const {
  assert(!hasMessanger());
  player.m_pMessanger = pMessanger;
  return player.m_linker.attachModule(player.m_pRootCommutator, pMessanger);
}

uint32_t Player::TestAccessor::setGame(modules::GamePtr pGame) const {
  assert(!hasGame());
  player.m_pGame = pGame;
  return player.m_linker.attachModule(player.m_pRootCommutator, pGame);
}

} // namespace world
