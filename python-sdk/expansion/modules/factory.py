from typing import Callable, Awaitable, Any, Tuple, Optional, Type
import logging

from expansion.interfaces.public.commutator import Tunnel
import expansion.interfaces.public as rpc
from expansion.transport import Terminal

from .base_module import BaseModule, ModuleType
from .ship import Ship
from .system_clock import SystemClock
from .engine import Engine
from .resource_container import ResourceContainer
from .celestial_scanner import CelestialScanner
from .asteroid_miner import AsteroidMiner

ModuleOrError = Tuple[Optional[BaseModule], Optional[str]]
TunnelOrError = Tuple[Optional[Tunnel], Optional[str]]
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
                    connection_factory=_make_connection_factory(
                        tunnel_factory=tunnel_factory,
                        terminal_type=rpc.ShipI
                    )), None

    elif module_type == ModuleType.ENGINE.value:
        return Engine(
            name=module_name,
            connection_factory=_make_connection_factory(
                tunnel_factory=tunnel_factory,
                terminal_type=rpc.EngineI)
        ), None

    elif module_type == ModuleType.RESOURCE_CONTAINER.value:
        return ResourceContainer(
            name=module_name,
            connection_factory=_make_connection_factory(
                tunnel_factory=tunnel_factory,
                terminal_type=rpc.ResourceContainerI)
        ), None

    elif module_type == ModuleType.CELESTIAL_SCANNER.value:
        return CelestialScanner(
            name=module_name,
            connection_factory=_make_connection_factory(
                tunnel_factory=tunnel_factory,
                terminal_type=rpc.CelestialScannerI)
        ), None

    elif module_type == ModuleType.ASTEROID_MINER.value:
        return AsteroidMiner(
            name=module_name,
            connection_factory=_make_connection_factory(
                tunnel_factory=tunnel_factory,
                terminal_type=rpc.AsteroidMinerI)
        ), None

    elif module_type == ModuleType.SYSTEM_CLOCK.value:
        return SystemClock(
            connection_factory=_make_connection_factory(
                tunnel_factory=tunnel_factory,
                terminal_type=rpc.SystemClockI
            ),
            name=module_name), None
    else:
        return None, f"module {module_type} is not supported yet"


def _make_connection_factory(tunnel_factory: TunnelFactory,
                             terminal_type: Type) -> Callable[[], Awaitable[Any]]:
    """This function returns a coroutine, that may be used to open a
    connection to the module with the specified 'terminal_type' using
    the specified 'tunnel_factory'"""
    async def _impl():
        tunnel, error = await tunnel_factory()
        if error is not None:
            _logger.warning(f"Failed to open tunnel to the {terminal_type.__name__}")
            return None

        terminal = terminal_type()
        assert isinstance(terminal, Terminal)

        tunnel.attach_to_terminal(terminal)
        terminal.attach_channel(tunnel)
        return terminal
    return _impl
