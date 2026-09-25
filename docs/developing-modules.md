# Adding a module

A module is equipment installed on a ship. This guide shows how to add one,
using `AsteroidScanner` as the example.

## Add the interface to Protocol.proto

Describe the messages the client and the server exchange in
`server/Protocol.proto`. Add one top-level message for the interface. The
asteroid scanner message is `IAsteroidScanner`:

```protobuf
message IAsteroidScanner {

  enum Status {
    IN_PROGRESS      = 0;
    SCANNER_BUSY     = 1;
    ASTEROID_TOO_FAR = 2;
  }

  message Specification {
    uint32 max_distance     = 1;
    uint32 scanning_time_ms = 2;
  }

  message ScanResult {
    uint32 asteroid_id       = 1;
    double weight            = 2;
    double metals_percent    = 3;
    double ice_percent       = 4;
    double silicates_percent = 5;
  }

  oneof choice {
    bool   specification_req = 1;
    uint32 scan_asteroid     = 2;

    Specification specification     = 21;
    Status        scanning_status   = 22;
    ScanResult    scanning_finished = 23;
  }
}
```

Commands use field numbers starting at 1. Replies use field numbers starting
at 21.

Add the interface to the top-level `Message` oneof. `asteroid_scanner` is
field 20:

```protobuf
message Message {
  uint32 tunnelId  = 1;
  uint64 timestamp = 2;

  oneof choice {
    // ...
    IAsteroidScanner asteroid_scanner = 20;
    // ...
  }
}
```

The server build compiles `Protocol.proto`. Generated C++ types are in the
`spex` namespace.

## Dispatch the interface from BaseModule

`BaseModule::handleMessage` routes each `Message` to a virtual handler. Declare
the handler in `server/Modules/BaseModule.h`:

```cpp
virtual void handleAsteroidScannerMessage(
    uint32_t, spex::IAsteroidScanner const&) {}
```

Call it from the switch in `server/Modules/BaseModule.cpp`:

```cpp
case spex::Message::kAsteroidScanner: {
  handleAsteroidScannerMessage(
      nSessionId, message.asteroid_scanner());
  return;
}
```

A module that does not implement the interface keeps the empty handler.

## Create the module files

Create a directory under `server/Modules/` named after the module, for example
`server/Modules/AsteroidScanner/`, with two files:

- `AsteroidScanner.h` declares the class
- `AsteroidScanner.cpp` implements it

`server/CMakeLists.txt` picks up every `Modules/*.cpp` through
`file(GLOB_RECURSE ...)`. A new `.cpp` does not need a CMake edit.

A standard manager has no files of its own. It is declared by a macro in
`server/Modules/Managers.h`. Write a separate header only when the manager
adds stages of its own, as `ResourceContainerManager` does.

## Declare the class

In namespace `modules`, inherit `BaseModule` and
`utils::GlobalObject<AsteroidScanner>`:

```cpp
class AsteroidScanner :
    public BaseModule,
    public utils::GlobalObject<AsteroidScanner>
{
  // ...
};
```

`BaseModule` (`server/Modules/BaseModule.h`) is the base of every module.
`utils::GlobalObject` (`server/Utils/GlobalContainer.h`) registers the instance
in `utils::GlobalContainer<AsteroidScanner>`. The manager walks that container
to find every instance.

In the `.cpp` file, outside any namespace, define the container's static data.
The macro argument includes the namespace:

```cpp
DECLARE_GLOBAL_CONTAINER_CPP(modules::AsteroidScanner);
```

Add the module header to `server/Modules/All.h`. Add forward declarations to
`server/Modules/Fwd.h`:

```cpp
MODULE_FWD_DECLARATION(AsteroidScanner)
MODULE_MANAGER_FWD_DECLARATION(AsteroidScanner)
```

The second macro declares `AsteroidScannerManager` and
`AsteroidScannerManagerPtr`.

## Constructor

`BaseModule` takes the module type, the instance name, the owner, and an
optional blueprint name:

```cpp
BaseModule(std::string sModuleType,
           std::string moduleName,
           world::PlayerWeakPtr pOwner,
           std::string sBlueprintName = {});
```

The type string is the name published on the commutator and the class name in
blueprint YAML. For this module it is `"AsteroidScanner"`.

`GlobalObject` reserves an id and stores a null pointer. The derived
constructor replaces that pointer with `this`:

```cpp
AsteroidScanner::AsteroidScanner(
    std::string&& sName, world::PlayerWeakPtr pOwner,
    uint32_t nMaxDistance, uint32_t nScanningTimeMs)
  : BaseModule("AsteroidScanner", std::move(sName), std::move(pOwner)),
    m_nMaxDistance(nMaxDistance),
    m_nScanningTimeMs(nScanningTimeMs)
{
  GlobalObject<AsteroidScanner>::registerSelf(this);
}
```

Fixed characteristics, such as `max_distance` and `scanning_time_ms`, are
constructor arguments. The blueprint supplies them when it builds the module.

## Loading the initial state

When a module has state in the world file, override:

```cpp
virtual bool loadState(YAML::Node const& source);
```

The node is YAML. Return `false` when the node is missing or invalid. The
default implementation returns `true`.

`AsteroidScanner` has no saved state, so it leaves `loadState` unchanged.
`RCS` overrides it to restore the current thrust vector. `ResourceContainer`
overrides it to restore the resources stored in the container.

Call the base implementation first:

```cpp
bool RCS::loadState(YAML::Node const& source)
{
  if (!BaseModule::loadState(source))
    return false;
  geometry::Vector& thrust =
      getPlatform()->getExternalForce_NoSync(m_nThrustVectorId);
  return thrust.load(source);
}
```

`Ship::loadState` calls `loadState` on each installed module while the world
is loaded.

## Handling messages

Override the handler declared on `BaseModule`:

```cpp
void handleAsteroidScannerMessage(
    uint32_t nTunnelId, spex::IAsteroidScanner const& message) override;
```

The manager does not run the handler when the datagram arrives. The message
stays in the module buffer until the manager's next activation, so it can wait
up to `nCooldown` microseconds of ingame time.

Build a `spex::Message` and send it with:

```cpp
bool sendToClient(uint32_t nSessionId, spex::Message&& message) const;
```

```cpp
spex::Message response;
spex::IAsteroidScanner::Specification* pBody =
    response.mutable_asteroid_scanner()->mutable_specification();
pBody->set_max_distance(m_nMaxDistance);
pBody->set_scanning_time_ms(m_nScanningTimeMs);
sendToClient(nTunnelId, std::move(response));
```

## Work that takes ingame time

Override `proceed` when a command takes ingame time:

```cpp
virtual void proceed(uint32_t nIntervalUs);
```

`nIntervalUs` is the microseconds of ingame time since this manager last ran.
`AsteroidScanner` overrides `proceed` because a scan takes time. Leave it
unchanged when every command finishes inside the message handler. The default
body calls `switchToIdleState()`.

The manager calls `proceed` only for modules in the active state. Enter that
state with `switchToActiveState()` and leave it with `switchToIdleState()`.

`switchToActiveState()` marks the module as activating. After the message
stage, the manager puts it on the busy list and calls `onActivated()`. Later
activations call `proceed` until `switchToIdleState()` marks the module as
deactivating. The manager then drops it and calls `onDeactivated()`.

## Create the manager

A manager gives the module time on the conveyor. Without a registered manager
the module never handles messages and never runs `proceed`.

The usual manager is `modules::CommonModulesManager`. It inherits
`conveyor::IAbstractLogic` and has two stages:

1. `handleBufferedMessages()` on every live instance
2. `proceed()` on instances that are active

It finds instances through `utils::GlobalContainer<ModuleType>`, so the module
must inherit `BaseModule` and `utils::GlobalObject<ModuleType>`.

```cpp
template <typename ModuleType, Cooldown nCooldown = Cooldown::eDefault>
class CommonModulesManager;
```

`ModuleType` is the module class, for example `AsteroidScanner`. `nCooldown`
is the pause between activations, in microseconds of ingame time.

### nCooldown

The manager skips conveyor cycles until `nCooldown` microseconds of ingame
time have passed. Messages that arrive during the pause stay buffered.

A smaller pause wakes the manager more often and costs more CPU. A larger
pause delays replies and coarsens logic that depends on `nIntervalUs`. Choose
the pause from the module's own timing. `AsteroidScanner` uses
`150000 + 13 * 11`.

### Declare a standard manager

Add an enumerator to `enum class Cooldown` in
`server/Modules/CommonModulesManager.h`. The header has two enums: the game
enum, and a second enum under `AUTOTESTS_MODE`. Add the name to both.

Game values use `interval + 11 * prime` so that managers wake on different
ticks:

```cpp
eAsteroidScanner = 150000 + 13 * 11,
```

The autotest enum usually sets the pause to `0`, so tests do not wait. Keep a
non-zero autotest pause only when a test depends on it, as `ePassiveScanner`
and `eResourceContainer` do.

Declare the manager in `server/Modules/Managers.h`:

```cpp
DECLARE_DEFAULT_MODULE_MANAGER(AsteroidScanner)
```

The macro expands to a class that inherits
`CommonModulesManager<AsteroidScanner, Cooldown::eAsteroidScanner>`.

### Why the pause is an enumerator

Equal pauses, and pauses that are multiples of one another, wake many
managers on the same rare ticks and leave the ticks between them idle.
Distinct values of the form `interval + 11 * prime` spread those wakes
across ticks.

### A manager with extra stages

When the two built-in stages are not enough, subclass `CommonModulesManager`
and skip the macro. `ResourceContainerManager` adds one stage by overriding
`getStagesCount`, `prepareAdditionalStage`, and `proceedAdditionalStage`.
Include that header from `Managers.h`.

## Register the manager

Add a member next to the other managers in `server/SystemManager.h`:

```cpp
modules::AsteroidScannerManagerPtr m_pAsteroidScannerManager;
```

Create it in `SystemManager::createAllComponents()`:

```cpp
m_pAsteroidScannerManager =
    std::make_shared<modules::AsteroidScannerManager>();
```

Attach it to the conveyor in `SystemManager::linkComponents()`:

```cpp
m_pConveyor->addLogicToChain(m_pAsteroidScannerManager);
```

`SystemManager.cpp` already includes `Modules/Managers.h`, which declares the
standard managers.

The conveyor runs one logic at a time, in `addLogicToChain` order. Inside a
single stage the main thread and the slave threads call `proceed` together.
`CommonModulesManager` hands each instance to one of those threads, so several
instances of the same module run in parallel while every other module type
waits.

## Reading the world

### The platform

`installOn` stores the platform and then calls `onInstalled`. Override
`onInstalled` for work that needs the platform, such as allocating a force
slot on the ship. The platform is a `modules::Ship`:

```cpp
modules::Ship*       getPlatform();
modules::Ship const* getPlatform() const;
```

The ship exposes position, velocity, and the rest of its physical state.
`AsteroidScanner` measures distance from `getPlatform()->getPosition()`.

### Other objects

Live objects of a type `T` are stored in `utils::GlobalContainer<T>`. The
static accessors are:

- `Size()` — length of the instance vector, including empty slots
- `Instance(uint32_t nInstanceId)` — the object at that index, or `nullptr`
  when the slot is empty
- `AllInstancies()` — that vector; entries may be `nullptr`
- `Empty()` — true when no live object is registered

Indexing is O(1). `Instance` asserts that the id is inside the vector.
Creating or destroying objects while another thread reads the container is a
race.

`world::AsteroidsContainer` is `utils::GlobalContainer<world::Asteroid>`.
`AsteroidScanner` checks `Size()` and then calls `Instance()`.

A type that inherits `utils::GlobalObject` for more than one type is listed in
each of those containers. `world::Asteroid` inherits `newton::PhysicalObject`,
and `PhysicalObject` is a `GlobalObject<PhysicalObject>`, so each asteroid is
present both as an `Asteroid` and as a `PhysicalObject`.

## Threads

Message handling and `proceed` run on several threads, and only for one module
type at a time. While `AsteroidScanner` instances run, other managers are not
inside `proceed`.

That split is enough for logic that only reads shared objects.
`AsteroidScanner` reads asteroids and does not write them, so its instances
share the asteroids without locks. Prefer logic in which one thread does not
block on another.

## Create a blueprint

A blueprint builds a module with fixed characteristics. Saved state, such as
the current thrust, is applied later by `loadState`. The asteroid scanner
blueprint is `server/Blueprints/Modules/AsteroidScannerBlueprint.h`.

The class is in namespace `blueprints` and inherits `blueprints::BaseBlueprint`.
After `load` returns, the blueprint is immutable.

Override `build`, `load`, and `dump`:

```cpp
modules::BaseModulePtr build(
    std::string sName, world::PlayerWeakPtr pOwner) const override;

bool load(YAML::Node const& data) override;

void dump(YAML::Node& out) const override;
```

`build` constructs the module. `load` reads the YAML parameters. Call
`BaseBlueprint::load` first: it reads the required `expenses` map. `dump`
writes those fields back; call `BaseBlueprint::dump` first.

```cpp
bool load(YAML::Node const& data) override
{
  return BaseBlueprint::load(data)
      && utils::YamlReader(data)
             .read("max_scanning_distance", m_nMaxScanningDistance)
             .read("scanning_time_ms", m_nScanningTimeMs);
}
```

```cpp
modules::BaseModulePtr build(
    std::string sName, world::PlayerWeakPtr pOwner) const override
{
  return std::make_shared<modules::AsteroidScanner>(
      std::move(sName), std::move(pOwner),
      m_nMaxScanningDistance, m_nScanningTimeMs);
}
```

Blueprint YAML is grouped by module class and blueprint type:

```yaml
Blueprints:
  Modules:
    AsteroidScanner:
      ancient-nordic-scanner:
        max_scanning_distance: 1000
        scanning_time_ms: 100
        expenses:
          labor: 100
```

The class name, `AsteroidScanner`, is the string passed to
`BlueprintsFactory::make` and the type string passed to `BaseModule`.

## Register the blueprint

`BlueprintsLibrary::loadModulesBlueprints` calls `BlueprintsFactory::make` for
each blueprint. Add a branch in `server/Blueprints/BlueprintFactory.cpp` and
include the blueprint header there:

```cpp
} else if (sModuleType == "AsteroidScanner") {
  pBlueprint = std::make_shared<AsteroidScannerBlueprint>();
}
```

`make` then calls `load` on that object. A header-only blueprint does not need
a `.cpp`: `BlueprintFactory.cpp` is already part of the build.

## Checklist

1. `Protocol.proto`: the interface message and a field on `Message`.
2. `BaseModule`: the virtual handler and the `handleMessage` case.
3. `Modules/<Name>/`: the class, `GlobalObject`, and
   `DECLARE_GLOBAL_CONTAINER_CPP`.
4. `Modules/Fwd.h`, `Modules/All.h`, and both `Cooldown` enums.
5. `Managers.h`: `DECLARE_DEFAULT_MODULE_MANAGER`, or a custom manager.
6. `SystemManager`: the member, `createAllComponents`, and `addLogicToChain`.
7. A blueprint class and a branch in `BlueprintsFactory::make`.
8. For a player-facing module, a row in
   [the module table](API/modules_table.md) and an API page next to it.
   The TypeScript client follows
   [typescript-sdk/AGENTS.md](../typescript-sdk/AGENTS.md).
