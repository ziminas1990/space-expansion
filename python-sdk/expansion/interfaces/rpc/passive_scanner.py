from typing import Optional, NamedTuple, List, Tuple
from enum import Enum

import expansion.protocol as protocol
import expansion.protocol.Protocol_pb2 as public
from expansion.transport import IOTerminal, Channel
import expansion.utils as utils
import expansion.types as types


class Specification(NamedTuple):
    scanning_radius_km: int
    max_update_time_ms: int


class PassiveScannerI(IOTerminal):

    class Status(Enum):
        SUCCESS = "success"
        MONITORING_FAILED = "monitoring failed"

        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        CHANNEL_CLOSED = "channel closed"
        CANCELED = "operation canceled"

        def is_success(self):
            return self == PassiveScannerI.Status.SUCCESS

        def is_timeout(self):
            return self == PassiveScannerI.Status.RESPONSE_TIMEOUT

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(PassiveScannerI))
        self.specification: Optional[Specification] = None

    @Channel.return_on_close(None)
    async def get_specification(self, timeout: float = 0.5)\
            -> Optional[Specification]:
        request = public.Message()
        request.passive_scanner.specification_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        spec = protocol.get_message_field(response, ["passive_scanner", "specification"])
        if not spec:
            return None
        return Specification(scanning_radius_km=spec.scanning_radius_km,
                             max_update_time_ms=spec.max_update_time_ms)

    @Channel.return_on_close(Status.CHANNEL_CLOSED)
    def start_monitoring(self, timeout: float = 0.5) -> Status:
        request = public.Message()
        request.passive_scanner.monitor = True
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return PassiveScannerI.Status.RESPONSE_TIMEOUT
        ack = protocol.get_message_field(response,
                                         ["passive_scanner", "monitor_ack"])
        return PassiveScannerI.Status.SUCCESS \
            if ack else PassiveScannerI.Status.MONITORING_FAILED

    @Channel.return_on_close((Status.MONITORING_FAILED, list()))
    def wait_update(self, timeout: float = 1) \
            -> Tuple[Status, List[types.PhysicalObject]]:
        response = await self.wait_message(timeout=timeout)
        if not response:
            return PassiveScannerI.Status.SUCCESS, list()
        update = protocol.get_message_field(
            response, ["passive_scanner", "update"])
        if not update:
            return PassiveScannerI.Status.UNEXPECTED_RESPONSE, list()

        return PassiveScannerI.Status.SUCCESS, [
            types.PhysicalObject(
                object_type=types.ObjectType.from_protobuf(item.object_type),
                object_id=item.id,
                position=types.Position(
                    x=item.x,
                    y=item.y,
                    velocity=types.Vector(x=item.vx, y=item.vy)
                ),
                radius=item.r
            )
            for item in update.items
        ]
