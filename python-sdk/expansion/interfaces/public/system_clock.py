from typing import Optional, Callable
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.queued_terminal import QueuedTerminal

import expansion.utils as utils


class SystemClock(QueuedTerminal):

    class Status(Enum):
        SUCCESS = "success"
        GENERATOR_ATTACHED = "attached to generator"
        GENERATOR_DETACHED = "detached from generator"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        def is_ok(self):
            return self == SystemClock.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IResourceContainer.Status) -> "SystemClock.Status":
            ProtobufStatus = public.ISystemClock.Status
            ModuleStatus = SystemClock.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.GENERATOR_ATTACHED: ModuleStatus.GENERATOR_ATTACHED,
                ProtobufStatus.GENERATOR_DETACHED: ModuleStatus.GENERATOR_DETACHED
            }[status]

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(SystemClock))

    async def time(self, timeout: float = 0.1) -> Optional[int]:
        """Return current server time"""
        request = public.Message()
        request.system_clock.time_req = True
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.time")

    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'"""
        request = public.Message()
        request.system_clock.wait_until = time
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")

    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        request = public.Message()
        request.system_clock.wait_for = period_us
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")

    async def attach_to_generator(self, time_cb: Callable[[int], bool],
                                  timeout: float = 0.5) -> Status:
        """Attach to generator. Call the specified 'time_cb' every time,
        when receives time from the generator. This call won't return
        until 'time_cb' return False"""
        status = await self._attach_generator(timeout)
        if status != SystemClock.Status.GENERATOR_ATTACHED:
            return status

        # receiving notifications
        resume = True
        while resume:
            response = await self.wait_message(timeout=timeout)
            if not response:
                return SystemClock.Status.RESPONSE_TIMEOUT
            time = get_message_field(response, "system_clock.time")
            if time is not None:
                resume = time_cb(time)
            # Ignoring unexpected message

        return await self._detach_generator(timeout)

    async def _attach_generator(self, timeout: float) -> Status:
        request = public.Message()
        request.system_clock.attach_generator = True
        if not self.channel.send(message=request):
            return SystemClock.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    async def _detach_generator(self, timeout: float) -> Status:
        request = public.Message()
        request.system_clock.detach_generator = True
        if not self.channel.send(message=request):
            return SystemClock.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    async def _receive_generator_status(self, timeout: float) -> Status:
        response = await self.wait_message(timeout=timeout)
        if not response:
            return SystemClock.Status.RESPONSE_TIMEOUT

        protobuf_status = get_message_field(response, "system_clock.generator_status")
        return SystemClock.Status.from_protobuf(protobuf_status)