from typing import Optional
import logging

from expansion.interfaces.public import (
    Ship,
    Commutator,
    ModuleInfo,
    Engine,
    SystemClock,
    CelestialScanner,
    ResourceContainer,
    AsteroidMiner
)


async def find_module(type: str, name: str, commutator: Commutator) -> Optional[ModuleInfo]:
    all_modules = await commutator.get_all_modules()
    if not all_modules:
        return None
    # Linear search is ok, because a number of modules is usually limited
    for desired_module_info in all_modules:
        if desired_module_info.type == type and desired_module_info.name == name:
            return desired_module_info
    return None


async def connect_to_ship(ship_type: str,
                          name: str,
                          commutator: Commutator,
                          owner_name: Optional[str] = None) -> Optional[Ship]:
    """Find a ship with the specified 'ship_type' and 'name', attached
    to the specified 'commutator'. The optionally specified
    'owner_name' will be used, to assign name to the ship, otherwise the
    commutator's name will be used instead.
    """
    candidate = await find_module(type=f"Ship/{ship_type}",
                                  name=name,
                                  commutator=commutator)
    if not candidate:
        return None

    if not owner_name:
        owner_name = commutator.get_name()
    ship: Ship = Ship(ship_name=f"{owner_name}::ship_{name}")

    error = await commutator.open_tunnel(candidate.slot_id, ship)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the ship '{name}' on the '{commutator.get_name()}': "
            f"{error}")
        return None
    return ship


async def connect_to_engine(name: str, ship: Ship,
                            owner_name: Optional[str] = None) -> Optional[Engine]:
    """Find engine with the specified 'name' on the specified 'ship' and return
    engine instance. The optionally specified 'owner_name' will be used, to
    assign name to the module, otherwise the ship's name will be used instead.
    """
    candidate = await find_module(type="Engine", name=name, commutator=ship.get_commutator())
    if not candidate:
        return None

    if not owner_name:
        owner_name = ship.get_name()
    engine: Engine = Engine(name=f"{owner_name}::engine_{name}")

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, engine)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the engine '{name}' on the ship '{ship.get_name()}': "
            f"{error}")
        return None
    return engine


async def connect_to_celestial_scanner(
        name: str,
        ship: Ship,
        owner_name: Optional[str] = None) -> Optional[CelestialScanner]:
    """Find celestial scanner with the specified 'name' on the specified 'ship'
    and return scanner instance. The optionally specified 'owner_name' will be
    used, to assign name to the module, otherwise the ship's name will be used
    instead.
    """
    candidate = await find_module(type="CelestialScanner",
                                  name=name,
                                  commutator=ship.get_commutator())
    if not candidate:
        return None

    if not owner_name:
        owner_name = ship.get_name()
    scanner: CelestialScanner = CelestialScanner(name=f"{owner_name}::scaner_{name}")

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, scanner)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the celestial scaner '{name}' on the ship '{ship.get_name()}': "
            f"{error}")
        return None
    return scanner


async def connect_to_system_clock(commutator: Commutator) -> Optional[SystemClock]:
    """Find and open tunnel to the SystemClock instance"""
    candidate = await find_module(type="SystemClock", name="SystemClock", commutator=commutator)
    if not candidate:
        return None

    system_clock: SystemClock = SystemClock("SystemClock")

    error = await commutator.open_tunnel(candidate.slot_id, system_clock.control_channel)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to open control channel to the system clock!': {error}")
        return None

    error = await commutator.open_tunnel(candidate.slot_id, system_clock.generator_channel)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to open generator channel to the system clock!': {error}")
        system_clock.control_channel.detach()
        return None
    return system_clock


async def connect_to_resource_container(
        name: str,
        ship: Ship,
        owner_name: Optional[str] = None) -> Optional[ResourceContainer]:
    """Find a resource container with the specified 'name' on the specified 'ship'
    and return scanner instance. The optionally specified 'owner_name' will be
    used, to assign name to the module, otherwise the ship's name will be used
    instead.
    """
    candidate = await find_module(type="ResourceContainer",
                                  name=name,
                                  commutator=ship.get_commutator())
    if not candidate:
        return None

    if not owner_name:
        owner_name = ship.get_name()
    container: ResourceContainer = ResourceContainer(name=f"{owner_name}::cargo_{name}")

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, container)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the resource container '{name}' on the ship "
            f"'{ship.get_name()}': {error}")
        return None
    return container


async def connect_to_asteroid_miner(
        name: str,
        ship: Ship,
        owner_name: Optional[str] = None) -> Optional[AsteroidMiner]:
    """Find a asteroid miner with the specified 'name' on the specified 'ship'
    and return scanner instance. The optionally specified 'owner_name' will be
    used, to assign name to the module, otherwise the ship's name will be used
    instead.
    """
    module_type_name = "AsteroidMiner"
    candidate = await find_module(type=module_type_name,
                                  name=name,
                                  commutator=ship.get_commutator())
    if not candidate:
        return None

    if not owner_name:
        owner_name = ship.get_name()
    miner: AsteroidMiner = AsteroidMiner(name=f"{owner_name}::miner_{name}")

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, miner.control_channel)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the {module_type_name} '{name}' on the ship "
            f"'{ship.get_name()}': {error}")
        return None

    error = await ship.get_commutator().open_tunnel(candidate.slot_id, miner.mining_channel)
    if error:
        logging.getLogger(__name__).warning(
            f"Failed to connect to the {module_type_name} '{name}' on the ship "
            f"'{ship.get_name()}': {error}")
        miner.control_channel.socket.close()
        return None
    return miner
