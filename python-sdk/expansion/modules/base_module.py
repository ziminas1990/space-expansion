from typing import List, Optional, Any, Callable, ContextManager, Awaitable, Tuple
import contextlib
import logging
from enum import Enum
import time

import expansion.interfaces.rpc as rpc


class ModuleType(Enum):
    SHIP = "Ship/"
    SYSTEM_CLOCK = "SystemClock"
    ENGINE = "Engine"
    RESOURCE_CONTAINER = "ResourceContainer"
    CELESTIAL_SCANNER = "CelestialScanner"
    ASTEROID_MINER = "AsteroidMiner"


class BaseModule:
    def __init__(self,
                 connection_factory: Callable[[], Awaitable[Any]],
                 logging_name: str):
        """Instantiate a module, that is attached on the specified 'port' to the
        specified remote 'commutator'. The specified 'connection_factory' will
        be used to instantiate an object, that implements a module's interface."""
        self.logger = logging.getLogger(logging_name)
        self._connection_factory = connection_factory

        self._channels: List[Any] = []
        # All opened channels, that may be used to send requests

    async def init(self) -> bool:
        return True

    @contextlib.asynccontextmanager
    async def _lock_channel(self) -> ContextManager[Any]:
        """Return an existing available channel or open a new one"""
        channel: Optional[Any] = None
        try:
            try:
                channel = self._channels.pop(0)
            except IndexError:
                # No available channels, trying to open a new one
                channel = await self._connection_factory()
            yield channel
        finally:
            if channel is not None:
                self._channels.append(channel)

    @staticmethod
    def _is_actual(value: Tuple[Optional[Any], int],
                   expiration_time_ms: int,) -> bool:
        return value[0] is not None and \
               (time.monotonic() * 1000 - value[1]) >= expiration_time_ms
