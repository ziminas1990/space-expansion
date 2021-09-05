from typing import Optional, NamedTuple, Callable
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
import expansion.protocol as protocol
from expansion.transport import IOTerminal
from expansion import utils


class Specification(NamedTuple):
    labor_per_sec: float


class ShipyardI(IOTerminal):
    class Status(Enum):
        SUCCESS = "success"
        INTERNAL_ERROR = "internal error"
        BUILD_STARTED = "build started"
        BUILD_IN_PROGRESS = "build in progress"
        BUILD_COMPLETE = "build complete"
        BUILD_CANCELED = "build canceled"
        BUILD_FROZEN = "build frozen"
        BUILD_FAILED = "build failed"
        BLUEPRINT_NOT_FOUND = "blueprint not found"
        SHIPYARD_IS_BUSY = "shipyard is busy"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        @staticmethod
        def from_protobuf(status: public.IResourceContainer.Status) -> "ShipyardI.Status":
            ProtobufStatus = public.IShipyard.Status
            ModuleStatus = ShipyardI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INTERNAL_ERROR: ModuleStatus.INTERNAL_ERROR,
                ProtobufStatus.BUILD_STARTED: ModuleStatus.BUILD_STARTED,
                ProtobufStatus.BUILD_IN_PROGRESS: ModuleStatus.BUILD_IN_PROGRESS,
                ProtobufStatus.BUILD_COMPLETE: ModuleStatus.BUILD_COMPLETE,
                ProtobufStatus.BUILD_CANCELED: ModuleStatus.BUILD_CANCELED,
                ProtobufStatus.BUILD_FROZEN: ModuleStatus.BUILD_FROZEN,
                ProtobufStatus.BUILD_FAILED: ModuleStatus.BUILD_FAILED,
                ProtobufStatus.BLUEPRINT_NOT_FOUND: ModuleStatus.BLUEPRINT_NOT_FOUND,
                ProtobufStatus.SHIPYARD_IS_BUSY: ModuleStatus.SHIPYARD_IS_BUSY,
                # To be extended
            }[status]

    BuildingReportCallback = Callable[[Status, Optional[float]], None]

    def __init__(self, name: Optional[str] = None):
        if not name:
            name = utils.generate_name(ShipyardI)
        super().__init__(name=name)

    async def get_specification(self, timeout: float = 0.5) \
            -> (Status, Optional[Specification]):
        request = public.Message()
        request.shipyard.specification_req = True
        if not self.send(message=request):
            return ShipyardI.Status.FAILED_TO_SEND_REQUEST, None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ShipyardI.Status.RESPONSE_TIMEOUT, None
        spec = protocol.get_message_field(response, ["shipyard", "specification"])
        if not spec:
            return ShipyardI.Status.UNEXPECTED_RESPONSE, None
        spec = Specification(labor_per_sec=spec.labor_per_sec)
        return ShipyardI.Status.SUCCESS, spec

    async def start_build(self, blueprint: str, ship_name: str) -> Status:
        request = public.Message()
        req_body = request.shipyard.start_build
        req_body.blueprint_name = blueprint
        req_body.ship_name = ship_name
        if not self.send(message=request):
            return ShipyardI.Status.FAILED_TO_SEND_REQUEST
        status, _ = await self.wait_building_report()
        return status

    async def wait_building_report(self, timeout: float = 1.0) -> \
            (Status, Optional[float]):
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ShipyardI.Status.RESPONSE_TIMEOUT, None
        report = protocol.get_message_field(response, ["shipyard", "building_report"])
        if not report:
            return ShipyardI.Status.UNEXPECTED_RESPONSE, None
        return ShipyardI.Status.from_protobuf(report.status), report.progress

    async def wait_building_complete(self, timeout: float = 1.0) -> \
            (Status, Optional[str], Optional[int]):
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ShipyardI.Status.RESPONSE_TIMEOUT, None, None
        report = protocol.get_message_field(response, ["shipyard", "building_complete"])
        if not report:
            return ShipyardI.Status.UNEXPECTED_RESPONSE, None, None
        return ShipyardI.Status.SUCCESS, \
               report.ship_name, \
               report.slot_id
