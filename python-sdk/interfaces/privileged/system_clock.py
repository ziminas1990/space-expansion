from typing import Optional
from enum import Enum

from transport.channel import Channel
import protocol.Privileged_pb2 as privileged
from protocol.utils import get_message_field


class Status(Enum):
    UNKNOWN = 0
    MODE_REAL_TIME = 1
    MODE_DEBUG = 2
    MODE_TERMINATED = 3
    CLOCK_IS_BUSY = 3
    INTERNAL_ERROR = 4

    @staticmethod
    def from_protobuf(mode: privileged.SystemClock.Status):
        try:
            return {
                privileged.SystemClock.Status.MODE_REAL_TIME: Status.MODE_REAL_TIME,
                privileged.SystemClock.Status.MODE_DEBUG: Status.MODE_DEBUG,
                privileged.SystemClock.Status.MODE_TERMINATED: Status.MODE_TERMINATED,
                privileged.SystemClock.Status.CLOCK_IS_BUSY: Status.CLOCK_IS_BUSY,
                privileged.SystemClock.Status.INTERNAL_ERROR: Status.INTERNAL_ERROR
            }[mode]
        except KeyError:
            return Status.MODE_UNKNOWN


class SystemClock:
    def __init__(self):
        self._channel: Optional[Channel] = None
        self._token: int = 0

    def attach_to_channel(self, channel: Channel, token: int):
        self._channel = channel
        self._token = token

    def _send_message(self, message: privileged.Message):
        message.token = self._token
        self._channel.send(message)

    async def get_time(self, timeout_sec: float = 0.01) -> Optional[int]:
        """Request current in-game time. Return number of microseconds
        since the game has been started"""
        message = privileged.Message()
        message.system_clock.time_req = True
        self._send_message(message)

        return await self._await_time(timeout_sec=timeout_sec)

    async def get_mode(self, timeout_sec: float = 0.01) -> Optional[Status]:
        """Request current clock mode"""
        message = privileged.Message()
        message.system_clock.mode_req = True
        self._send_message(message)
        return await self._await_status(timeout_sec=timeout_sec)

    async def switch_to_real_time(self, timeout_sec: float = 0.05) -> bool:
        """Switch clock to the real time mode (if clock is in debug mode)"""
        message = privileged.Message()
        message.system_clock.switch_to_real_time = True
        self._send_message(message)
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_REAL_TIME

    async def switch_to_debug_mode(self, timeout_sec: float = 0.05) -> bool:
        """Switch clock to the debug mode (if clock is in real time mode)"""
        message = privileged.Message()
        message.system_clock.switch_to_debug_mode = True
        self._send_message(message)
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_DEBUG

    async def terminate(self, timeout_sec: float = 0.05) -> bool:
        """Terminate system clock (server will stop)"""
        message = privileged.Message()
        message.system_clock.terminate = True
        self._send_message(message)
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_TERMINATED

    async def set_tick_duration(self, tick_duration_usec: int, timeout_sec: float = 0.01) -> bool:
        """Set one tick duration to the specified 'tick_duration_usec'
        microseconds (if clock is in debug mode)."""
        message = privileged.Message()
        message.system_clock.tick_duration_us = tick_duration_usec
        self._send_message(message)
        return await self._await_status(timeout_sec=timeout_sec) == Status.MODE_DEBUG

    async def proceed_ticks(self, ticks: int, timeout_sec: float) -> (bool, int):
        """Proceed the specified number of 'ticks' of in-game time (if clock
        is in debug mode).
        Return (True, current_time) after proceed is done. Otherwise return
        (False, None)
        """
        message = privileged.Message()
        message.system_clock.proceed_ticks = ticks
        self._send_message(message)
        time: Optional[int] = await self._await_time(timeout_sec=timeout_sec)
        return time is not None, time

    async def _await_status(self, timeout_sec: float = 0.05) -> Optional[Status]:
        response = await self._channel.receive(timeout=timeout_sec)
        status = get_message_field(response, ["system_clock", "status"])
        return Status.from_protobuf(mode=status) if status is not None else None

    async def _await_time(self, timeout_sec: float = 0.05) -> Optional[int]:
        response = await self._channel.receive(timeout=timeout_sec)
        time = get_message_field(response, ["system_clock", "now"])
        return time

