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
        GENERATOR_ATTACHED = "attached to generator"
        GENERATOR_DETACHED = "detached from generator"
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
                ProtobufStatus.GENERATOR_ATTACHED: ModuleStatus.GENERATOR_ATTACHED,
                ProtobufStatus.GENERATOR_DETACHED: ModuleStatus.GENERATOR_DETACHED
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

    @abstractmethod
    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick in microseconds"""
        pass

    @abstractmethod
    async def attach_to_generator(self, timeout: float = 0.5) -> "SystemClockI.Status":
        """Send 'attach_generator' request and wait for status response"""
        pass

    @abstractmethod
    async def detach_from_generator(self, timeout: float = 5) -> "SystemClockI.Status":
        """Send 'detach_generator' request and wait for status response"""
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
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.time")

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
        return get_message_field(response, "system_clock.ring")

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
        return get_message_field(response, "system_clock.ring")

    @Channel.return_on_close(None)
    async def wait_timestamp(self, timeout: float = 0.5) -> Optional[int]:
        """Wait for a 'time' message, that carries current system clock's time"""
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            self.logger.warning("Timeout while waiting generator time message")
            return None
        return get_message_field(response, "system_clock.time")

    @Channel.return_on_close(None)
    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick in microseconds"""
        request = public.Message()
        request.system_clock.generator_tick_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.generator_tick_us")

    @Channel.return_on_close(SystemClockI.Status.CHANNEL_CLOSED)
    async def attach_to_generator(self, timeout: float = 0.5) -> SystemClockI.Status:
        """Send 'attach_generator' request and wait for status response"""
        request = public.Message()
        request.system_clock.attach_generator = True
        if not self.send(message=request):
            return SystemClockI.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    @Channel.return_on_close(SystemClockI.Status.CHANNEL_CLOSED)
    async def detach_from_generator(self, timeout: float = 5) -> SystemClockI.Status:
        """Send 'detach_generator' request and wait for status response"""
        request = public.Message()
        request.system_clock.detach_generator = True
        if not self.send(message=request):
            return SystemClockI.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    @Channel.return_on_close(SystemClockI.Status.CHANNEL_CLOSED)
    async def _receive_generator_status(self, timeout: float) -> SystemClockI.Status:
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return SystemClockI.Status.RESPONSE_TIMEOUT

        protobuf_status = get_message_field(response, "system_clock.generator_status")
        return SystemClockI.Status.from_protobuf(protobuf_status)
