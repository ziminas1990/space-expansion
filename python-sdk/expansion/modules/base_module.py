import asyncio
from typing import List, Optional, Any, Callable, ContextManager, Awaitable, Tuple, Type, Dict, TYPE_CHECKING
import contextlib
import logging
from enum import Enum
import time

if TYPE_CHECKING:
    from decorator import decorator
else:
    def decorator(func):
        return func

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

    class UnreachableError(Exception):
        """Exception occurs if BaseModule can't open session to
        the remote side"""
        pass

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

    async def open_session(self,
                           terminal_type: Type,
                           retries: int = 3) -> Optional[Endpoint]:
        """Return an existing available channel or open a new one."""
        # No available tunnels, trying to open a new one
        for attempt in range(retries):
            tunnel: Optional[ProxyChannel] = None
            error: Optional[str] = None
            tunnel, error = await self._tunnel_factory()
            if tunnel is None:
                self.logger.warning(
                    f"Failed to open tunnel for the {terminal_type.__name__} "
                    f"({attempt + 1} / {retries}: {error}")
                continue
            # Create terminal and link it with tunnel
            terminal: Optional[Endpoint] = terminal_type()
            assert isinstance(terminal, Endpoint)
            tunnel.attach_to_terminal(terminal)
            terminal.attach_channel(tunnel)
            return terminal
        # All attempts failed
        return None

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
            better_than: Callable[[Any, Any], Awaitable[bool]]) \
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

    @staticmethod
    def use_session(
            terminal_type: Type,
            *,
            return_on_unreachable: Any,
            retires: int = 3,
            return_on_cancel: Optional[Any] = None,
            exclusive: bool = False):
        """Get an available session with the specified
        'terminal_type' from a sessions pool and pass it to the
        underlying function. If there are no available session
        in the pool, make up to 'retires' attempts to open a
        new one. If all attempts fail, return 'return_on_unreachable'
        value.
        If 'exclusive' flag is NOT set to True, then
        session should be returned back to a sessions pool for
        further reuse. If 'exclusive' flag is True, OR any
        exception occurs during underlying call, then session
        should be closed and never reused.
        If CancelError exception arises and 'return_on_cancel' is
        not None, the return 'return_on_cancel' value. Otherwise,
        re-raise the exception.
        If any other exception occurs, it should be re raised as
        well.
        If 'session' argument is passed to wrapped call by user,
        than just pass it to underlying call and do nothing more.
        """

        def _decorator(func):
            @decorator
            async def _wrapper(*args, **kwargs):
                if "session" in kwargs:
                    # Session is already provided by user, so just pass it
                    # to wrapped call
                    return await func(*args, **kwargs)

                self: BaseModule = args[0]
                session: Optional[Endpoint] = None
                try:
                    session = self._sessions.setdefault(terminal_type, []).pop(-1)
                except IndexError:
                    # No sessions to reuse, open a new one
                    session = await self.open_session(terminal_type, retires)
                if session is None:
                    return return_on_unreachable

                # Pass session to wrapped function
                try:
                    result = await func(*args, **kwargs, session=session)
                    # Everything fine, so we can put session to pool
                    # for further reuse
                    if not exclusive:
                        self._sessions.setdefault(terminal_type, [])\
                            .append(session)
                    return result
                except asyncio.CancelledError as e:
                    # Something went wrong, so we can't afford to reuse this
                    # session anymore
                    await session.close()
                    if return_on_cancel is not None:
                        return return_on_cancel
                    else:
                        raise e
                except Exception as e:
                    # Something went wrong, so we can't afford to reuse this
                    # session anymore
                    await session.close()
                    raise e
            return _wrapper
        return _decorator
