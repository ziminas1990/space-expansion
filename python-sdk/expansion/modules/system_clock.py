from typing import Optional, Set, Callable, Awaitable
import threading
import asyncio

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
                         logging_name=name or utils.generate_name(SystemClock))
        self.tick_us: Optional[int] = None
        self.server_time: types.TimePoint = types.TimePoint(0)
        self.time_callback: Set[Callable[[types.TimePoint], None]] = set()
        # A set of a callbacks, which wants to receive timestamps
        self.mutex: threading.Lock = threading.Lock()

        self.cached_time: Callable[[], int] = self.server_time.now

    async def sync(self, timeout: float = 0.5) -> bool:
        """Sync local system clock with server"""
        async with self.rent_session(rpc.SystemClock) as channel:
            if not channel:
                return False
            ingame_time = await channel.time()
            if ingame_time is None:
                return False
            self.server_time.update(ingame_time)

    async def time(self, predict: bool = True, timeout: float = 0.5) -> int:
        """Update the cached time and return it"""
        await self.sync(timeout)
        return self.server_time.now(predict=predict)

    def time_point(self) -> types.TimePoint:
        """Return cached time point.

        Cached timepoint can be used to predict current server time.
        Moreover the returned object will be updated every time when
        SystemClock receives actual timestamp from the server
        """
        return self.server_time

    async def wait_until(self, time: int, timeout: float) \
            -> Optional[int]:
        """Wait until server time reaches the specified 'time'

        On success update cached time and return cached time, otherwise
        return None"""
        async with self.rent_session(rpc.SystemClock) as channel:
            if channel is None:
                return None
            time = await channel.wait_until(time=time, timeout=timeout)
            if time:
                self.server_time.update(time)
            return self.server_time.now()

    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        async with self.rent_session(rpc.SystemClock) as channel:
            if channel is None:
                return None
            time = await channel.wait_for(period_us=period_us, timeout=timeout)
            if time:
                self.server_time.update(time)
            return self.server_time.now()

    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick long (in microseconds). Once the
        value is retrieved from the server, it will be cached"""
        if self.tick_us is None:
            async with self.rent_session(rpc.SystemClock) as channel:
                if channel is None:
                    return None
                self.tick_us = await channel.get_generator_tick_us(timeout=timeout)
        return self.tick_us

    async def attach_to_generator(self, timeout: float = 0.5) -> "SystemClockI.Status":
        """Send 'attach_generator' request and wait for status response"""
        assert False, "Not supported on this level! Use 'subscribe()' instead!"

    async def detach_from_generator(self, timeout: float = 5) -> "SystemClockI.Status":
        """Send 'detach_generator' request and wait for status response"""
        assert False, "Not supported on this level! Use 'Unsubscribe()' instead!"

    def subscribe(self, time_cb: Callable[[types.TimePoint], None]):
        with self.mutex:
            self.time_callback.add(time_cb)
            if len(self.time_callback) == 1:
                asyncio.get_running_loop().create_task(self._time_watcher())

    def unsubscribe(self, time_cb: Callable[[types.TimePoint], None]):
        with self.mutex:
            self.time_callback.remove(time_cb)

    async def _time_watcher(self):
        async with self.rent_session(rpc.SystemClock) as channel:
            status = await channel.attach_to_generator()
            if status != rpc.SystemClockI.Status.GENERATOR_ATTACHED:
                self.logger.error("Can't attach to the system clock's generator!")
                return
            while len(self.time_callback) > 0:
                current_time = await channel.wait_timestamp()
                if current_time is None:
                    # Ignoring error
                    continue
                self.server_time.update(current_time)
                for time_cb in self.time_callback:
                    time_cb(self.server_time)
            await channel.detach_from_generator()
