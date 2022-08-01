from typing import Optional, List, Tuple

from .base_module import ModuleType
from .commutator import Commutator
from .ship import Ship
from .engine import Engine, EngineSpec
from .system_clock import SystemClock
from .resource_container import ResourceContainer
from .celestial_scanner import CelestialScanner
from .asteroid_miner import AsteroidMiner


def get_system_clock(commutator: Commutator) -> Optional[SystemClock]:
    try:
        system_clock = commutator.modules[ModuleType.SYSTEM_CLOCK.value]
    except KeyError:
        return None
    assert len(system_clock) == 1
    for value in system_clock.values():
        assert isinstance(value, SystemClock)
        return value
    return None


def get_ship(commutator: Commutator, ship_type: str, ship_name: str) -> Optional[Ship]:
    if not ship_type.startswith(ModuleType.SHIP.value):
        ship_type = f"{ModuleType.SHIP.value}{ship_type}"

    try:
        ship = commutator.modules[ship_type][ship_name]
        assert isinstance(ship, Ship)
        return ship
    except KeyError:
        return None


def get_engine(commutator: Commutator, name: str) -> Optional[Engine]:
    try:
        engine = commutator.modules[ModuleType.ENGINE.value][name]
        assert isinstance(engine, Engine)
        return engine
    except KeyError:
        return None


def get_all_engines(commutator: Commutator) -> List[Engine]:
    try:
        engines = commutator.modules[ModuleType.ENGINE.value]
        return [engine for engine in engines.values()
                if isinstance(engine, Engine)]
    except KeyError:
        return []


async def get_most_powerful_engine(commutator: Commutator) -> Optional[Engine]:
    choice: Optional[Tuple[Engine, EngineSpec]] = None

    for engine in get_all_engines(commutator):
        engine_spec = await engine.get_specification()
        if not engine_spec:
            continue
        if not choice or choice[1].max_thrust < engine_spec.max_thrust:
            choice = (engine, engine_spec)
    return choice[0] if choice else None


def get_cargo(commutator: Commutator, name: str) -> Optional[ResourceContainer]:
    try:
        cargo = commutator.modules[ModuleType.RESOURCE_CONTAINER.value][name]
        assert isinstance(cargo, ResourceContainer)
        return cargo
    except KeyError:
        return None


def get_celestial_scanner(commutator: Commutator, name: str) -> Optional[CelestialScanner]:
    try:
        device = commutator.modules[ModuleType.CELESTIAL_SCANNER.value][name]
        assert isinstance(device, CelestialScanner)
        return device
    except KeyError:
        return None


def get_any_celestial_scanner(commutator: Commutator) -> Optional[CelestialScanner]:
    try:
        scanners = commutator.modules[ModuleType.CELESTIAL_SCANNER.value]
        for scanner in scanners.values():
            assert isinstance(scanner, CelestialScanner)
            return scanner
        return None
    except KeyError:
        return None


def get_asteroid_miner(commutator: Commutator, name: str) -> Optional[AsteroidMiner]:
    try:
        device = commutator.modules[ModuleType.ASTEROID_MINER.value][name]
        assert isinstance(device, AsteroidMiner)
        return device
    except KeyError:
        return None
