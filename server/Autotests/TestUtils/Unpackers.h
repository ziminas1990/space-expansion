#pragma once

#include <Protocol.pb.h>

namespace autotests {

template<typename MessageType> struct Unpacker;

template<>
struct Unpacker<spex::ISessionControl> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kSession; }

  inline static
  const spex::ISessionControl& unpack(const spex::Message& message) {
    return message.session();
  }
};

template<>
struct Unpacker<spex::IRootSession> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kRootSession; }

  inline static
  const spex::IRootSession& unpack(const spex::Message& message) {
    return message.root_session();
  }
};

template<>
struct Unpacker<spex::IAccessPanel> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kAccessPanel; }

  inline static
  const spex::IAccessPanel& unpack(const spex::Message& message) {
    return message.accesspanel();
  }
};

template<>
struct Unpacker<spex::ICommutator> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kCommutator; }

  inline static
  const spex::ICommutator& unpack(const spex::Message& message) {
    return message.commutator();
  }
};

template<>
struct Unpacker<spex::IShip> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kShip; }

  inline static
  const spex::IShip& unpack(const spex::Message& message) {
    return message.ship();
  }
};

template<>
struct Unpacker<spex::INavigation> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kNavigation; }

  inline static
  const spex::INavigation& unpack(const spex::Message& message) {
    return message.navigation();
  }
};

template<>
struct Unpacker<spex::IEngine> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kEngine; }

  inline static
  const spex::IEngine& unpack(const spex::Message& message) {
    return message.engine();
  }
};

template<>
struct Unpacker<spex::IPassiveScanner> {
  inline static
  spex::Message::ChoiceCase choice() { return spex::Message::kPassiveScanner; }

  inline static
  const spex::IPassiveScanner& unpack(const spex::Message& message) {
    return message.passive_scanner();
  }
};

template<>
struct Unpacker<spex::ICelestialScanner> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kCelestialScanner;
  }

  inline static
  const spex::ICelestialScanner& unpack(const spex::Message& message) {
    return message.celestial_scanner();
  }
};

template<>
struct Unpacker<spex::IAsteroidScanner> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kAsteroidScanner;
  }

  inline static
  const spex::IAsteroidScanner& unpack(const spex::Message& message) {
    return message.asteroid_scanner();
  }
};

template<>
struct Unpacker<spex::IResourceContainer> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kResourceContainer;
  }

  inline static
  const spex::IResourceContainer& unpack(const spex::Message& message) {
    return message.resource_container();
  }
};

template<>
struct Unpacker<spex::IAsteroidMiner> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kAsteroidMiner;
  }

  inline static
  const spex::IAsteroidMiner& unpack(const spex::Message& message) {
    return message.asteroid_miner();
  }
};

template<>
struct Unpacker<spex::IBlueprintsLibrary> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kBlueprintsLibrary;
  }

  inline static
  const spex::IBlueprintsLibrary& unpack(const spex::Message& message) {
    return message.blueprints_library();
  }
};

template<>
struct Unpacker<spex::IShipyard> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kShipyard;
  }

  inline static
  const spex::IShipyard& unpack(const spex::Message& message) {
    return message.shipyard();
  }
};

template<>
struct Unpacker<spex::IGame> {
  inline static
  spex::Message::ChoiceCase choice() {
    return spex::Message::kGame;
  }

  inline static
  const spex::IGame& unpack(const spex::Message& message) {
    return message.game();
  }
};

}  // namespace autotests
