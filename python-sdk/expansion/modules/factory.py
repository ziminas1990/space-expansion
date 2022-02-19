from typing import Callable, Awaitable, Tuple, Optional, TYPE_CHECKING
import logging

from expansion.transport import ProxyChannel

from .base_module import BaseModule, ModuleType
from .ship import Ship
from .system_clock import SystemClock
from .engine import Engine
from .resource_container import ResourceContainer
from .celestial_scanner import CelestialScanner
from .passive_scanner import PassiveScanner
from .asteroid_miner import AsteroidMiner
from .shipyard import Shipyard
from .blueprints_library import BlueprintsLibrary

ModuleOrError = Tuple[Optional[BaseModule], Optional[str]]
TunnelOrError = Tuple[Optional[ProxyChannel], Optional[str]]
TunnelFactory = Callable[[], Awaitable[TunnelOrError]]
# Type for the coroutine, that returns a tunnel or an error

_logger = logging.getLogger("expansion.modules.factory")


def module_factory(module_type: str,
                   module_name: str,
                   tunnel_factory: TunnelFactory) -> ModuleOrError:
    """Create a module with the specified 'module_type' ans the specified
    'module_name'. The specified 'tunnel_factory' callback will be used
    to open a tunnel to the module and may be called at any time during the
    module's lifecycle.
    """
    if module_type.startswith(ModuleType.SHIP.value):
        return Ship(ship_type=module_type,
                    ship_name=module_name,
                    modules_factory=module_factory,
                    tunnel_factory=tunnel_factory), None
    elif module_type == ModuleType.ENGINE.value:
        return Engine(
            name=module_name,
            tunnel_factory=tunnel_factory), None
    elif module_type == ModuleType.RESOURCE_CONTAINER.value:
        return ResourceContainer(
            name=module_name,
            tunnel_factory=tunnel_factory
        ), None
    elif module_type == ModuleType.CELESTIAL_SCANNER.value:
        return CelestialScanner(
            name=module_name,
            tunnel_factory=tunnel_factory
        ), None
    elif module_type == ModuleType.PASSIVE_SCANNER.value:
        return PassiveScanner(
            name=module_name,
            tunnel_factory=tunnel_factory
        ), None
    elif module_type == ModuleType.ASTEROID_MINER.value:
        return AsteroidMiner(
            name=module_name,
            tunnel_factory=tunnel_factory
        ), None
    elif module_type == ModuleType.SHIPYARD.value:
        return Shipyard(
            tunnel_factory=tunnel_factory,
            name=module_name), None
    elif module_type == ModuleType.SYSTEM_CLOCK.value:
        return SystemClock(
            tunnel_factory=tunnel_factory,
            name=module_name), None
    elif module_type == ModuleType.BLUEPRINTS_LIBRARY.value:
        return BlueprintsLibrary(
            tunnel_factory=tunnel_factory,
            name=module_name), None
    else:
        return None, f"module {module_type} is not supported yet"
