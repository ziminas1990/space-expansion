from typing import Optional, Set, Callable, Awaitable, Tuple
import threading
import asyncio
import time

import expansion.interfaces.public as remote
import expansion.utils as utils

from .base_module import BaseModule


class SystemClock(BaseModule):
    def __init__(self,
                 connection_factory: Callable[[], Awaitable[remote.SystemClockI]],
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(SystemClock))
        self.tick_us: Optional[int] = None
        self.cached_time: Tuple[Optional[int], int] = (None, 0)
        self.time_callback: Set[Callable[[int], None]] = set()
        # A set of a callbacks, which wants to receive timestamps
        self.mutex: threading.Lock = threading.Lock()

    async def time(self) -> Optional[int]:
        async with self._lock_channel() as channel:
            if channel:
                assert isinstance(channel, remote.SystemClockI)
                self.cached_time = (await channel.time(), int(time.monotonic() * 1000))
        return self.cached_time[0]

    def approximate_time(self) -> Optional[int]:
        if self.cached_time[0] is None:
            return None
        dt = int(time.monotonic() * 1000) - self.cached_time[1]
        return self.cached_time[0] + dt

    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'"""
        async with self._lock_channel() as channel:
            if channel is None:
                return None
            assert isinstance(channel, remote.SystemClockI)
            return await channel.wait_until(time=time, timeout=timeout) \
                if channel is not None else None

    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        async with self._lock_channel() as channel:
            if channel is None:
                return None
            assert isinstance(channel, remote.SystemClockI)
            return await channel.wait_for(period_us=period_us, timeout=timeout) \
                if channel is not None else None

    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick long (in microseconds). Once the
        value is retrieved from the server, it will be cached"""
        if self.tick_us is None:
            async with self._lock_channel() as channel:
                if channel is None:
                    return None
                assert isinstance(channel, remote.SystemClockI)
                self.tick_us = await channel.get_generator_tick_us(timeout=timeout)
        return self.tick_us

    def subscribe(self, time_cb: Callable[[int], None]):
        with self.mutex:
            self.time_callback.add(time_cb)
            if len(self.time_callback) == 1:
                asyncio.get_running_loop().create_task(self._time_watcher())

    def unsubscribe(self, time_cb: Callable[[int], None]):
        with self.mutex:
            self.time_callback.remove(time_cb)

    async def _time_watcher(self):
        async with self._lock_channel() as channel:
            assert isinstance(channel, remote.SystemClockI)
            status = await channel.attach_to_generator()
            if status != remote.SystemClockI.Status.GENERATOR_ATTACHED:
                self.logger.error("Can't attach to the system clock's generator!")
                return
            while len(self.time_callback) > 0:
                current_time = await channel.wait_timestamp()
                self.cached_time = (current_time, int(time.monotonic() * 1000))
                if current_time is None:
                    # Ignoring error
                    continue
                for time_cb in self.time_callback:
                    time_cb(current_time)
            await channel.detach_from_generator()
