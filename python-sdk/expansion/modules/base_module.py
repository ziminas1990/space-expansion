import asyncio
from typing import List, Optional, Any, Callable, ContextManager, Awaitable, Tuple, Type, Dict, TYPE_CHECKING
import contextlib
import logging
from enum import Enum
import time

from expansion.transport import ProxyChannel, Endpoint

TunnelOrError = Tuple[Optional[ProxyChannel], Optional[str]]
TunnelFactory = Callable[[], Awaitable[TunnelOrError]]

if TYPE_CHECKING:
    from expansion.modules import Commutator


class ModuleType(Enum):
    SHIP = "Ship/"
    SYSTEM_CLOCK = "SystemClock"
    ENGINE = "Engine"
    RESOURCE_CONTAINER = "ResourceContainer"
    CELESTIAL_SCANNER = "CelestialScanner"
    ASTEROID_MINER = "AsteroidMiner"
    SHIPYARD = "Shipyard"


class BaseModule:
    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: str):
        """Instantiate a module, that is attached on the specified 'port' to the
        specified remote 'commutator'. The specified 'connection_factory' will
        be used to instantiate an object, that implements a module's interface."""
        self.name = name
        self.logger = logging.getLogger(name)
        self._tunnel_factory = tunnel_factory
        # All opened channels, that may be used to send requests
        self._sessions: Dict[Type, List[Endpoint]] = {}

    async def init(self) -> bool:
        return True

    @contextlib.asynccontextmanager
    async def rent_session(self,
                           terminal_type: Type,
                           retires: int = 3) -> ContextManager[Optional[Endpoint]]:
        """Rent a session for a simple request/response communications
        """
        terminal: Optional[Endpoint] = None
        try:
            try:
                terminal = self._sessions.setdefault(terminal_type, []).pop(-1)
            except IndexError:
                # No terminals to reuse
                while terminal is None and retires > 0:
                    terminal = await self.open_session(terminal_type)
                    retires -= 1
            finally:
                assert terminal is not None
                yield terminal
        except asyncio.CancelledError as exc:
            # Tunnel could be in unknown state now (may be it is still waiting for
            # answer). So we can't let another client to reuse it
            await terminal.close()
            terminal = None
            raise exc
        finally:
            if terminal:
                self._sessions.setdefault(terminal_type, []).append(terminal)

    @contextlib.asynccontextmanager
    async def open_managed_session(
            self,
            terminal_type: Type,
            retires: int = 3) -> ContextManager[Optional[Endpoint]]:
        """Context manager wrapper of 'open_session'"""
        terminal: Optional[Endpoint] = None
        while terminal is None and retires > 0:
            terminal = await self.open_session(terminal_type)
            retires -= 1
        yield terminal
        if terminal:
            await terminal.close()

    async def open_session(self, terminal_type: Type) -> Optional[Endpoint]:
        """Return an existing available channel or open a new one."""
        tunnel: Optional[ProxyChannel] = None
        error: Optional[str] = None
        # No available tunnels, trying to open a new one
        try:
            tunnel, error = await self._tunnel_factory()
        except Exception as ex:
            print(ex)
        if tunnel is None:
            self.logger.warning(f"Failed to open tunnel for the {terminal_type.__name__}: {error}")
            return None
        # Create terminal and link it with tunnel
        terminal: Optional[Endpoint] = terminal_type()
        assert isinstance(terminal, Endpoint)
        tunnel.attach_to_terminal(terminal)
        terminal.attach_channel(tunnel)
        return terminal

    @staticmethod
    def _is_actual(value: Tuple[Optional[Any], int],
                   expiration_time_ms: int,) -> bool:
        return value[0] is not None and \
               (time.monotonic() * 1000 - value[1]) <= expiration_time_ms

    @staticmethod
    def get_by_name(commutator: "Commutator",
                    type: ModuleType,
                    name: str) -> Optional["BaseModule"]:
        try:
            return commutator.modules[type.value][name]
        except KeyError:
            return None

    @staticmethod
    async def find_best(
            commutator: "Commutator",
            type: ModuleType,
            better_than: Awaitable[Callable[[Any, Any], bool]]) \
            -> Optional["BaseModule"]:
        best: Optional["BaseModule"] = None
        try:
            all = commutator.modules[type.value]
            for candidate in all.values():
                if not best or await better_than(candidate, best):
                    best = candidate
            return best
        except KeyError:
            return None
