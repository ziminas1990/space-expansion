import time
from typing import Optional, Set, Callable, Awaitable, AsyncGenerator, NamedTuple, TYPE_CHECKING
import threading
from math import isclose

import expansion.interfaces.rpc as rpc
import expansion.utils as utils
import expansion.types as types
from expansion.modules import ModuleType

from .base_module import BaseModule, TunnelFactory

if TYPE_CHECKING:
    from expansion.modules import Commutator

ConnectionFactory = Callable[[], Awaitable[rpc.SystemClockI]]


class SystemClock(rpc.SystemClockI, BaseModule):
    def __init__(self,
                 tunnel_factory: TunnelFactory,
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(SystemClock))
        self.tick_us: Optional[int] = None
        # 'server_time' will be used to predict current real(!) time on server
        self.server_time: Optional[types.TimePoint] = None
        # 'ingame_time' will be used to predict current ingame time on server
        self.ingame_time: types.TimePoint = types.TimePoint(0)
        # A set of a callbacks, which wants to receive timestamps
        self.time_callbacks: Set[Callable[[types.TimePoint], None]] = set()

        self.cached_time: Callable[[], int] = self.ingame_time.predict_usec

    @BaseModule.use_session(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=False,
        return_on_cancel=False)
    async def initial_sync(self,
                           timeout: float = 0.5,
                           session: rpc.SystemClock = None):
        started_at = time.monotonic_ns()
        points: [rpc.ServerTimestamp] = []
        for i in range(10):
            timestamp = await session.time()
            if timestamp:
                points.append(timestamp)
        stopped_at = time.monotonic_ns()
        dt_us = (stopped_at - started_at) / 1000
        rtt_us = dt_us / len(points)
        self.server_time = types.TimePoint(points[-1].real_us + round(rtt_us/2))

    @BaseModule.use_session(
        terminal_type=rpc.SystemClock,
        return_on_unreachable=False,
        return_on_cancel=False)
    async def sync(self,
                   timeout: float = 0.5,
                   session: rpc.SystemClock = None) -> bool:
        """Sync local system clock with server"""
        assert session is not None
        timestamp = await session.time()
        if timestamp is None:
            return False
        deviation_us = self.server_time.predict_usec() - timestamp.real_us if self.server_time is not None else 0
        self.ingame_time.update(timestamp.ingame_us + deviation_us)

    async def time(self, predict: bool = True, timeout: float = 0.5) -> int:
        """Return ingame time in microseconds and update the cached"""
        await self.sync(timeout)
        return self.ingame_time.predict_usec() if predict else self.ingame_time.usec()

    def time_point(self) -> types.TimePoint:
        """Return cached time point.

        Cached timepoint can be used to predict current server time.
        Moreover the returned object will be updated every time when
        SystemClock receives actual timestamp from the server
        """
        return self.ingame_time

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
            dt = time - self.ingame_time.predict_usec()
            timeout = 1.2 * dt / 10**6 if dt > 10000 else 0.1

        timestamp = await session.wait_until(time=time, timeout=timeout)
        if timestamp:
            deviation_us = self.server_time.predict_usec() - timestamp.real_us if self.server_time is not None else 0
            self.ingame_time.update(timestamp.ingame_us + deviation_us)
        return self.ingame_time.predict_usec()

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

        timestamp = await session.wait_for(period_us=period_us, timeout=timeout)
        if timestamp:
            deviation_us = self.server_time.predict_usec() - timestamp.real_us if self.server_time is not None else 0
            self.ingame_time.update(timestamp.ingame_us + deviation_us)
        return self.ingame_time.predict_usec()

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
        if timestamp is None:
            yield None
            return

        deviation_us = self.server_time.predict_usec() - timestamp.real_us if self.server_time is not None else 0
        self.ingame_time.update(timestamp.ingame_us + deviation_us)
        yield self.ingame_time.usec()

        while timestamp:
            timestamp = await session.wait_timestamp(timeout)
            if timestamp:
                deviation_us = self.server_time.predict_usec() - timestamp.real_us if self.server_time is not None else 0
                self.ingame_time.update(timestamp.ingame_us + deviation_us)
            yield self.ingame_time.usec()

    @staticmethod
    def find(commutator: "Commutator") -> Optional["SystemClock"]:
        return BaseModule._get_any(
            commutator=commutator,
            type=ModuleType.SYSTEM_CLOCK
        )
