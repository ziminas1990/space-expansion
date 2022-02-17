from typing import Optional
from enum import Enum

from expansion.transport import IOTerminal
import expansion.api as api
from expansion.api.utils import get_message_field


class Status(Enum):
    MODE_UNKNOWN = 0
    MODE_REAL_TIME = 1
    MODE_DEBUG = 2
    MODE_TERMINATED = 3
    CLOCK_IS_BUSY = 3
    INTERNAL_ERROR = 4

    @staticmethod
    def from_protobuf(mode: api.admin.SystemClock.Status):
        try:
            return {
                api.admin.SystemClock.Status.MODE_REAL_TIME: Status.MODE_REAL_TIME,
                api.admin.SystemClock.Status.MODE_DEBUG: Status.MODE_DEBUG,
                api.admin.SystemClock.Status.MODE_TERMINATED: Status.MODE_TERMINATED,
                api.admin.SystemClock.Status.CLOCK_IS_BUSY: Status.CLOCK_IS_BUSY,
                api.admin.SystemClock.Status.INTERNAL_ERROR: Status.INTERNAL_ERROR
            }[mode]
        except KeyError:
            return Status.MODE_UNKNOWN


class SystemClock(IOTerminal):
    def __init__(self, name: str, *args, **kwargs):
        super(SystemClock, self).__init__(name=name, *args, **kwargs)

    async def get_time(self, timeout_sec: float = 0.1) -> Optional[int]:
        """Request current in-game time. Return number of microseconds
        since the game has been started"""
        message = api.admin.Message()
        message.system_clock.time_req = True
        if not self.send(message):
            return None
        return await self._await_time(timeout_sec=timeout_sec)

    async def get_mode(self, timeout_sec: float = 0.1) -> Optional[Status]:
        """Request current clock mode"""
        message = api.admin.Message()
        message.system_clock.mode_req = True
        if not self.send(message):
            return None
        return await self._await_status(timeout_sec=timeout_sec)

    async def switch_to_real_time(self, timeout_sec: float = 0.1) -> bool:
        """Switch clock to the real time mode (if clock is in debug mode)"""
        message = api.admin.Message()
        message.system_clock.switch_to_real_time = True
        if not self.send(message):
            return False
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_REAL_TIME

    async def switch_to_debug_mode(self, timeout_sec: float = 0.1) -> bool:
        """Switch clock to the debug mode (if clock is in real time mode)"""
        message = api.admin.Message()
        message.system_clock.switch_to_debug_mode = True
        if not self.send(message):
            return False
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_DEBUG

    async def terminate(self, timeout_sec: float = 0.1) -> bool:
        """Terminate system clock (server will stop)"""
        message = api.admin.Message()
        message.system_clock.terminate = True
        if not self.send(message):
            return False
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_TERMINATED

    async def set_tick_duration(self, tick_duration_usec: int, timeout_sec: float = 0.1) -> bool:
        """Set one tick duration to the specified 'tick_duration_usec'
        microseconds (if clock is in debug mode)."""
        message = api.admin.Message()
        message.system_clock.tick_duration_us = tick_duration_usec
        if not self.send(message):
            return False
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_DEBUG

    async def proceed_ticks(self, ticks: int, timeout_sec: float) -> (bool, int):
        """Proceed the specified number of 'ticks' of in-game time (if clock
        is in debug mode).
        Return (True, current_time) after proceed is done. Otherwise return
        (False, None)
        """
        message = api.admin.Message()
        message.system_clock.proceed_ticks = ticks
        if not self.send(message):
            return False, 0
        time: Optional[int] = await self._await_time(timeout_sec=timeout_sec)
        return time is not None, time

    async def _await_status(self, timeout_sec: float = 0.1) -> Optional[Status]:
        response, _ = await self.wait_message(timeout=timeout_sec)
        status = get_message_field(response, ["system_clock", "status"])
        return Status.from_protobuf(mode=status) if status is not None else None

    async def _await_time(self, timeout_sec: float = 0.1) -> Optional[int]:
        response, _ = await self.wait_message(timeout=timeout_sec)
        time = get_message_field(response, ["system_clock", "now"])
        if time is None:
            return None
        return time

