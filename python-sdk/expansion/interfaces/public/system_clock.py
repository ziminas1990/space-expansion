from typing import Optional

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.queued_terminal import QueuedTerminal

import expansion.utils as utils


class SystemClock(QueuedTerminal):

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

    async def wait_until(self, time: int, timeout: float):
        """Wait until server time reaches the specified 'time'"""
        request = public.Message()
        request.system_clock.wait_until = time
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")

    async def wait_for(self, period: int, timeout: float):
        """Wait for the specified 'period' microseconds"""
        request = public.Message()
        request.system_clock.wait_for = period
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        return get_message_field(response, "system_clock.ring")