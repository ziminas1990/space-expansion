from typing import Optional, Tuple
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport import IOTerminal
from expansion.types.geometry import Vector

import expansion.utils as utils


class Monitor(IOTerminal):

    class Status(Enum):
        SUCCESS = "success"
        LIMIT_EXCEEDED = "limit exceeded"
        UNEXPECTED_MSG = "unexpected message"
        INVALID_TOKEN = "invalid token"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        def is_ok(self):
            return self == AsteroidMinerI.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IMonitor.Status) -> "Monitor.Status":
            ProtobufStatus = public.IMonitor.Status
            ModuleStatus = Monitor.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.LIMIT_EXCEEDED: ModuleStatus.LIMIT_EXCEEDED,
                ProtobufStatus.UNEXPECTED_MSG: ModuleStatus.UNEXPECTED_MSG,
                ProtobufStatus.INVALID_TOKEN: ModuleStatus.INVALID_TOKEN
            }[status]

    async def subscribe(self) -> Tuple["Monitor.Status", Optional[int]]:
        """Return status and subscribtion token if status us success"""
        request = public.Message()
        request.monitor.subscribe = True
        if not self.send(message=request):
            return None, Monitor.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None, Monitor.Status.RESPONSE_TIMEOUT
        token = get_message_field(response, "monitor.token")
        if token is not None:
            return token, Monitor.Status.SUCCESS
        status = get_message_field(response, "monitor.status")
        if status is not None:
            return None, Monitor.Status.from_protobuf(status)
        return None, Monitor.Status.UNEXPECTED_RESPONSE

    async def unsubscribe(self, token: int) -> "Monitor.Status":
        """Send unsubscribe request with the specified 'token' Return
        operation complete status"""
        request = public.Message()
        request.monitor.unsubscribe = token
        if not self.send(message=request):
            return Monitor.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return Monitor.Status.RESPONSE_TIMEOUT
        status = get_message_field(response, "monitor.status")
        if status is None:
            Monitor.Status.UNEXPECTED_RESPONSE
        return Monitor.Status.from_protobuf(status)
