from typing import Optional, Set, Callable, Awaitable
import threading
import asyncio

import expansion.interfaces.rpc as rpc
import expansion.utils as utils
import expansion.types as types

from .base_module import BaseModule

ConnectionFactory = Callable[[], Awaitable[rpc.SystemClockI]]


class SystemClock(BaseModule):
    def __init__(self,
                 connection_factory: ConnectionFactory,
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(SystemClock))
        self.tick_us: Optional[int] = None
        self.server_time: types.TimePoint = types.TimePoint(0)
        self.time_callback: Set[Callable[[types.TimePoint], None]] = set()
        # A set of a callbacks, which wants to receive timestamps
        self.mutex: threading.Lock = threading.Lock()

    async def sync(self) -> bool:
        """Sync local system clock with server"""
        async with self._lock_channel() as channel:
            if not channel:
                return False
            assert isinstance(channel, rpc.SystemClockI)
            ingame_time = await channel.time()
            if ingame_time is None:
                return False
            self.server_time.update(ingame_time)

    async def time(self) -> types.TimePoint:
        """Update the cached time and return it"""
        await self.sync()
        return self.server_time

    def cached_time(self) -> types.TimePoint:
        """Return cached ingame time

        Note: this function doesn't request server time. It returns
        just a cached time. Cached time still can predict server's
        time but the longer at has not been synced, the bigger
        could be prediction error"""
        return self.server_time

    async def wait_until(self, time: int, timeout: float) \
            -> Optional[types.TimePoint]:
        """Wait until server time reaches the specified 'time'

        On success update cached time and return cached time, otherwise
        return None"""
        async with self._lock_channel() as channel:
            if channel is None:
                return None
            assert isinstance(channel, rpc.SystemClockI)
            time = await channel.wait_until(time=time, timeout=timeout)
            if time:
                self.server_time.update(time)
            return self.server_time

    async def wait_for(self, period_us: int, timeout: float) -> Optional[types.TimePoint]:
        """Wait for the specified 'period' microseconds"""
        async with self._lock_channel() as channel:
            if channel is None:
                return None
            assert isinstance(channel, rpc.SystemClockI)
            time = await channel.wait_for(period_us=period_us, timeout=timeout)
            if time:
                self.server_time.update(time)
            return self.server_time

    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick long (in microseconds). Once the
        value is retrieved from the server, it will be cached"""
        if self.tick_us is None:
            async with self._lock_channel() as channel:
                if channel is None:
                    return None
                assert isinstance(channel, rpc.SystemClockI)
                self.tick_us = await channel.get_generator_tick_us(timeout=timeout)
        return self.tick_us

    def subscribe(self, time_cb: Callable[[types.TimePoint], None]):
        with self.mutex:
            self.time_callback.add(time_cb)
            if len(self.time_callback) == 1:
                asyncio.get_running_loop().create_task(self._time_watcher())

    def unsubscribe(self, time_cb: Callable[[int], None]):
        with self.mutex:
            self.time_callback.remove(time_cb)

    async def _time_watcher(self):
        async with self._lock_channel() as channel:
            assert isinstance(channel, rpc.SystemClockI)
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
