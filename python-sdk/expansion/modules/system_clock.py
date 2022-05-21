from typing import Optional, Set, Callable, Awaitable, AsyncGenerator
import threading
import asyncio
from math import isclose

import expansion.interfaces.rpc as rpc
import expansion.utils as utils
import expansion.types as types

from .base_module import BaseModule, TunnelFactory

ConnectionFactory = Callable[[], Awaitable[rpc.SystemClockI]]


class SystemClock(rpc.SystemClockI, BaseModule):
    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(SystemClock))
        self.tick_us: Optional[int] = None
        self.server_time: types.TimePoint = types.TimePoint(0)
        self.time_callbacks: Set[Callable[[types.TimePoint], None]] = set()
        # A set of a callbacks, which wants to receive timestamps
        self.mutex: threading.Lock = threading.Lock()

        self.cached_time: Callable[[], int] = self.server_time.predict_usec

    @BaseModule.use_session(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=False,
        return_on_cancel=False)
    async def sync(self,
                   timeout: float = 0.5,
                   session: rpc.SystemClock = None) -> bool:
        """Sync local system clock with server"""
        assert session is not None
        ingame_time = await session.time()
        if ingame_time is None:
            return False
        self.server_time.update(ingame_time)

    async def time(self, predict: bool = True, timeout: float = 0.5) -> int:
        """Return ingame time in microseconds and update the cached"""
        await self.sync(timeout)
        return self.server_time.predict_usec() if predict else self.server_time.usec()

    def time_point(self) -> types.TimePoint:
        """Return cached time point.

        Cached timepoint can be used to predict current server time.
        Moreover the returned object will be updated every time when
        SystemClock receives actual timestamp from the server
        """
        return self.server_time

    @BaseModule.use_session(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def wait_until(self,
                         time: int,
                         timeout: float = 0,
                         session: rpc.SystemClock = None) \
            -> Optional[int]:
        """Wait until server time reaches the specified 'time'

        On success update cached time and return cached time, otherwise
        return None"""
        assert session is not None
        if isclose(timeout, 0):
            dt = time - self.server_time.predict_usec()
            timeout = 1.2 * dt / 10**6 if dt > 0.1 else 0.1

        time = await session.wait_until(time=time, timeout=timeout)
        if time:
            self.server_time.update(time)
        return self.server_time.predict_usec()

    @BaseModule.use_session(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def wait_for(self,
                       period_us: int,
                       timeout: float = 0,
                       session: rpc.SystemClock = None) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        assert session is not None
        if isclose(timeout, 0):
            timeout = max(1.2 * period_us / 10 ** 6, 0.1)

        time = await session.wait_for(period_us=period_us, timeout=timeout)
        if time:
            self.server_time.update(time)
        return self.server_time.predict_usec()

    @BaseModule.use_session_for_generators(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=None)
    async def monitor(self,
                      interval_ms: int,
                      session: rpc.SystemClock = None) \
            -> AsyncGenerator[Optional[int], None]:
        """Start monitoring. Will yield current server time every
        'interval_ms' milliseconds
        """
        assert session is not None
        timeout = interval_ms * 10 / 1000

        timestamp = await session.monitoring(interval_ms)
        yield timestamp
        if timestamp is None:
            return
        if self.server_time.usec() < timestamp:
            self.server_time.update(timestamp)

        while timestamp:
            timestamp = await session.wait_timestamp(timeout)
            if self.server_time.usec() < timestamp:
                self.server_time.update(timestamp)
            yield timestamp
