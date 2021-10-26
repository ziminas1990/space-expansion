from typing import Optional
from enum import Enum
import logging
from abc import ABC, abstractmethod

from expansion.transport import IOTerminal, Channel
import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field

import expansion.utils as utils


class SystemClockI(ABC):
    class Status(Enum):
        SUCCESS = "success"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        NO_SUCH_CALLBACK = "no such callback"
        CHANNEL_CLOSED = "channel closed"

        def is_success(self):
            return self == SystemClockI.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IResourceContainer.Status) -> "SystemClockI.Status":
            ProtobufStatus = public.ISystemClock.Status
            ModuleStatus = SystemClockI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
            }[status]

    @abstractmethod
    async def time(self, timeout: float = 0.1) -> Optional[int]:
        """Return current server time"""
        pass

    @abstractmethod
    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'

        Return actual server's time"""
        pass

    @abstractmethod
    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds

        Return actual server's time"""
        pass


class SystemClock(SystemClockI, IOTerminal):
    def __init__(self, name: Optional[str] = None, trace_mode: bool = False):
        super().__init__(name=name, trace_mode=trace_mode)
        if name is None:
            name = utils.generate_name(SystemClock)
        self.logger = logging.getLogger(name)

    @Channel.return_on_close(None)
    async def time(self, timeout: float = 0.1) -> Optional[int]:
        """Return current server time"""
        request = public.Message()
        request.system_clock.time_req = True
        if not self.send(message=request):
            return None
        return await self.wait_timestamp()

    @Channel.return_on_close(None)
    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'

        Return actual server's time"""
        request = public.Message()
        request.system_clock.wait_until = time
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, ["system_clock", "ring"])

    @Channel.return_on_close(None)
    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds

        Return actual server's time"""
        request = public.Message()
        request.system_clock.wait_for = period_us
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, ["system_clock", "ring"])

    @Channel.return_on_close(None)
    async def monitoring(self,
                         interval_ms: int,
                         timeout: float = 0.5) -> Optional[int]:
        """Start monitoring in this session and return current time.
        Server will send current timestamp in this session every 'interval_ms'
        milliseconds. You may call 'wait_timestamp()' with
        timeout=interval_ms*5 in order to receive timestamps.
        Note: the only way to stop monitoring is to close the session.
        """
        request = public.Message()
        request.system_clock.monitor = interval_ms
        if not self.send(message=request):
            return None
        return await self.wait_timestamp()

    @Channel.return_on_close(None)
    async def wait_timestamp(self, timeout: float = 0.5) -> Optional[int]:
        """Wait for a 'time' message, that carries current system clock's time"""
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            self.logger.warning("Timeout while waiting timestamp")
            return None
        return get_message_field(response, ["system_clock", "time"])
