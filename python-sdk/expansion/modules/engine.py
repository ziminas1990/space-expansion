from typing import Optional, Callable, Awaitable, Tuple
import time

from expansion.interfaces.rpc import EngineI, EngineSpec
import expansion.utils as utils
from expansion.types import Vector

from .base_module import BaseModule


class Engine(BaseModule):
    class Cache:
        def __init__(self):
            self.specification: Optional[EngineSpec] = None
            self.thrust: Tuple[Optional[Vector], int] = (None, 0)

    def __init__(self,
                 connection_factory: Callable[[], Awaitable[EngineI]],
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(Engine))
        self.cache = Engine.Cache()

    async def get_specification(self, timeout: float = 0.5, reset_cached=False) \
            -> Optional[EngineSpec]:
        if reset_cached:
            self.cache.specification = None
        if self.cache.specification:
            return self.cache.specification
        async with self._lock_channel() as channel:
            assert isinstance(channel, EngineI)  # For type hinting
            self.cache.specification = await channel.get_specification(timeout=timeout)
        return self.cache.specification

    async def get_thrust(self,
                         timeout: float = 0.5,
                         expiring_ms: int = 100) -> Optional[Vector]:
        """Return current engine thrust"""
        if not self._is_actual(self.cache.thrust, expiring_ms):
            async with self._lock_channel() as channel:
                assert isinstance(channel, EngineI)  # For type hinting
                self.cache.thrust = await channel.get_thrust(timeout=timeout), \
                                    time.monotonic() * 1000
        return self.cache.thrust[0]

    async def set_thrust(self, thrust: Vector, duration_ms: int = 0) -> bool:
        """Set engine thrust to the specified 'thrust' for the
        specified 'duration_ms' milliseconds. If 'duration_ms' is 0, then
        the thrust will be set until another command.
        Function doesn't await any acknowledgement or response.
        Return true if a request has been sent
        """
        async with self._lock_channel() as channel:
            assert isinstance(channel, EngineI)  # For type hinting
            return channel.set_thrust(thrust=thrust, duration_ms=duration_ms)
            # Should we update self.cache.thrust here?
