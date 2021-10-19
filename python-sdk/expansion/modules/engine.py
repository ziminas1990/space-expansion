from typing import Optional, Tuple
import time

from expansion.interfaces.rpc import EngineI, EngineSpec
import expansion.utils as utils
from expansion.types import Vector

from .base_module import BaseModule, TunnelFactory


class Engine(BaseModule):
    class Cache:
        def __init__(self):
            self.specification: Optional[EngineSpec] = None
            self.thrust: Tuple[Optional[Vector], int] = (None, 0)

    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(Engine))
        self.cache = Engine.Cache()

    @BaseModule.use_session(
        terminal_type=EngineI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_specification(
            self,
            timeout: float = 0.5,
            reset_cached=False,
            session: Optional[EngineI] = None) -> Optional[EngineSpec]:
        assert session is not None
        if reset_cached:
            self.cache.specification = None
        if self.cache.specification:
            return self.cache.specification
        self.cache.specification = \
            await session.get_specification(timeout=timeout)
        return self.cache.specification

    @BaseModule.use_session(
        terminal_type=EngineI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_thrust(self,
                         timeout: float = 0.5,
                         expiring_ms: int = 100,
                         session: Optional[EngineI] = None) \
            -> Optional[Vector]:
        """Return current engine thrust"""
        assert session is not None
        if not self._is_actual(self.cache.thrust, expiring_ms):
            self.cache.thrust = await session.get_thrust(timeout=timeout), \
                                time.monotonic() * 1000
        return self.cache.thrust[0]

    @BaseModule.use_session(
        terminal_type=EngineI,
        return_on_unreachable=False,
        return_on_cancel=False)
    async def set_thrust(self,
                         thrust: Vector,
                         at: int = 0,
                         duration_ms: int = 0,
                         session: Optional[EngineI] = None) -> bool:
        """Set engine thrust to the specified 'thrust' for the
        specified 'duration_ms' milliseconds. If 'duration_ms' is 0, then
        the thrust will be set until another command.
        Function doesn't await any acknowledgement or response.
        Return true if a request has been sent
        """
        assert session is not None
        return session.set_thrust(thrust=thrust, at=at, duration_ms=duration_ms)
