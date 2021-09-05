import logging
from typing import Optional, Tuple, Callable, Any
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport import IOTerminal
import expansion.interfaces.rpc as rpc


class Monitor(IOTerminal):

    class Status(Enum):
        SUCCESS = "success"
        LIMIT_EXCEEDED = "limit exceeded"
        UNEXPECTED_MSG = "unexpected message"
        INVALID_TOKEN = "invalid token"
        CANCELED = "subscription is canceled"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        def is_ok(self):
            return self == Monitor.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IMonitor.Status) -> "Monitor.Status":
            ProtobufStatus = public.IMonitor.Status
            ModuleStatus = Monitor.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.LIMIT_EXCEEDED: ModuleStatus.LIMIT_EXCEEDED,
                ProtobufStatus.UNEXPECTED_MSG: ModuleStatus.UNEXPECTED_MSG,
                ProtobufStatus.INVALID_TOKEN: ModuleStatus.INVALID_TOKEN,
                ProtobufStatus.CANCELED: ModuleStatus.CANCELED
            }[status]

    async def subscribe(self) -> Tuple["Monitor.Status", Optional[int]]:
        """Return status and subscribtion token if status us success"""
        request = public.Message()
        request.monitor.subscribe = True
        if not self.send(message=request):
            return Monitor.Status.FAILED_TO_SEND_REQUEST, None
        response, _ = await self.wait_message()
        if not response:
            return Monitor.Status.RESPONSE_TIMEOUT, None
        token = get_message_field(response, "monitor.token")
        if token is not None:
            return Monitor.Status.SUCCESS, token
        status = get_message_field(response, "monitor.status")
        if status is not None:
            return Monitor.Status.from_protobuf(status), None
        return Monitor.Status.UNEXPECTED_RESPONSE, None

    async def unsubscribe(self, token: int) -> "Monitor.Status":
        """Send unsubscribe request with the specified 'token' Return
        operation complete status"""
        request = public.Message()
        request.monitor.unsubscribe = token
        if not self.send(message=request):
            return Monitor.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message()
        if not response:
            return Monitor.Status.RESPONSE_TIMEOUT
        status = get_message_field(response, "monitor.status")
        if status is None:
            return Monitor.Status.UNEXPECTED_RESPONSE
        return Monitor.Status.from_protobuf(status)

    async def monitor(
            self,
            *,
            ship_state_cb: Optional[Callable[[rpc.ShipState], None]] = None) -> Any:
        while(True):
            indication, timestamp = await self.wait_message(timeout=3)
            if not indication:
                continue

            try:
                interface = indication.WhichOneof('choice')
                message = getattr(indication, interface).WhichOneof('choice')
            except Exception as exc:
                logging.warning(f"{self.__name__}: {exc}")
                continue

            if interface == "ship":
                if message == "state":
                    if ship_state_cb:
                        ship_state_cb(rpc.ShipState.build(
                            indication.ship.state, timestamp))
            elif interface == "monitor":
                if message == "status":
                    if indication.monitor.status == rpc.Monitor.Status.CANCELED:
                        return

