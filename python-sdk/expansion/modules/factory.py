from typing import Callable, Awaitable, Tuple, Optional

from expansion.transport import ProxyChannel, SessionsMux
import expansion.interfaces.rpc as rpc

from .base_module import BaseModule, ModuleType
from .ship import Ship
from .system_clock import SystemClock
from .engine import Engine
from .resource_container import ResourceContainer
from .passive_scanner import PassiveScanner
from .asteroid_miner import AsteroidMiner
from .shipyard import Shipyard
from .blueprints_library import BlueprintsLibrary
from .messanger import Messanger

ModuleOrError = Tuple[Optional[BaseModule], Optional[str]]
TunnelOrError = Tuple[Optional[ProxyChannel], Optional[str]]
TunnelFactory = Callable[[], Awaitable[TunnelOrError]]
# Type for the coroutine, that returns a tunnel or an error


def module_factory(module_info: rpc.ModuleInfo,
                   session_mux: SessionsMux,
                   tunnel_factory: TunnelFactory) -> ModuleOrError:
    """Create a module described by 'module_info'. The specified
    'tunnel_factory' callback will be used to open a tunnel to the module
    and may be called at any time during the module's lifecycle.
    """
    module_type = module_info.type
    module_name = module_info.name
    if module_type == ModuleType.SHIP.value:
        return Ship(name=module_name,
                    ship_class=module_info.blueprint_name,
                    session_mux=session_mux,
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
    elif module_type == ModuleType.MESSANGER.value:
        return Messanger(
            tunnel_factory=tunnel_factory,
            name=module_name
        ), None
    else:
        return None, f"module {module_type} is not supported yet"
