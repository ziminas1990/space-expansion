from typing import Optional, Callable, Set
from enum import Enum
import asyncio
import logging

from expansion.transport import IOTerminal
import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field

import expansion.utils as utils


class SystemClock:

    class Status(Enum):
        SUCCESS = "success"
        GENERATOR_ATTACHED = "attached to generator"
        GENERATOR_DETACHED = "detached from generator"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        NO_SUCH_CALLBACK = "no such callback"

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
        if name is None:
            name = utils.generate_name(SystemClock)
        self.logger = logging.getLogger(name)
        self.control_channel: IOTerminal = IOTerminal(f"{name}.C")
        self.generator_channel: IOTerminal = IOTerminal(f"{name}.G")

        self._generator_tick_us: Optional[int] = None
        self._attached_task: Optional[asyncio.Task] = None
        # Task, that receives time updates from server's SystemClock
        self.time_callback: Set[Callable[[int], None]] = set()

    async def time(self, timeout: float = 0.1) -> Optional[int]:
        """Return current server time"""
        request = public.Message()
        request.system_clock.time_req = True
        if not self.control_channel.send(message=request):
            return None
        response = await self.control_channel.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.time")

    async def wait_until(self, time: int, timeout: float) -> Optional[int]:
        """Wait until server time reaches the specified 'time'"""
        request = public.Message()
        request.system_clock.wait_until = time
        if not self.control_channel.send(message=request):
            return None
        response = await self.control_channel.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")

    async def wait_for(self, period_us: int, timeout: float) -> Optional[int]:
        """Wait for the specified 'period' microseconds"""
        request = public.Message()
        request.system_clock.wait_for = period_us
        if not self.control_channel.send(message=request):
            return None
        response = await self.control_channel.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")

    async def get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick in microseconds"""
        if self._generator_tick_us is None:
            self._generator_tick_us = await self._get_generator_tick_us()
        return self._generator_tick_us

    async def attach_to_generator(self, time_cb: Callable[[int], None]) -> Status:
        """Attach the specified 'time_cb' to the clock's generator"""
        if self._attached_task is None:
            status = await self._attach_generator()
            if status != SystemClock.Status.GENERATOR_ATTACHED:
                return status
            self._attached_task = asyncio.get_running_loop().create_task(
                self._generator_task()
            )
        self.time_callback.add(time_cb)
        return SystemClock.Status.SUCCESS

    def detach_from_generator(self, time_cb: Callable[[int], None]) -> Status:
        """Detach the specified 'time_cb' from the clock's generator"""
        try:
            self.time_callback.remove(time_cb)
        except KeyError:
            return SystemClock.Status.NO_SUCH_CALLBACK
        return SystemClock.Status.SUCCESS

    async def _get_generator_tick_us(self, timeout: float = 0.5) -> Optional[int]:
        """Return generator's tick in microseconds"""
        request = public.Message()
        request.system_clock.generator_tick_req = True
        if not self.control_channel.send(message=request):
            return None
        response = await self.control_channel.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.generator_tick_us")

    async def _attach_generator(self, timeout: float = 0.5) -> Status:
        request = public.Message()
        request.system_clock.attach_generator = True
        if not self.generator_channel.send(message=request):
            return SystemClock.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    async def _detach_generator(self, timeout: float) -> Status:
        request = public.Message()
        request.system_clock.detach_generator = True
        if not self.generator_channel.send(message=request):
            return SystemClock.Status.FAILED_TO_SEND_REQUEST
        return await self._receive_generator_status(timeout)

    async def _receive_generator_status(self, timeout: float) -> Status:
        response = await self.generator_channel.wait_message(timeout=timeout)
        if not response:
            return SystemClock.Status.RESPONSE_TIMEOUT

        protobuf_status = get_message_field(response, "system_clock.generator_status")
        return SystemClock.Status.from_protobuf(protobuf_status)

    async def _generator_task(self, timeout: float = 0.5):
        while len(self.time_callback) > 0:
            response = await self.generator_channel.wait_message(timeout=timeout)
            if not response:
                self.logger.warning("Timeout while waiting generator time message")
                # Ignoring the problem
                continue
            time = get_message_field(response, "system_clock.time")
            if time is None:
                # Ignoring unexpected message
                self.logger.warning(f"Got unexpected message:\n{response}")
                continue
            for cb in self.time_callback:
                cb(time)

        await self._detach_generator(timeout)
        self._attached_task = None
