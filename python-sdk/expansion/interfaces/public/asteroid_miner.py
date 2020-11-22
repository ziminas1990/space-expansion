from typing import Optional, NamedTuple, Callable
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
import expansion.protocol as protocol
from expansion.transport import IOTerminal
import expansion.utils as utils
from expansion.types import ResourceType, ResourceItem


class Specification(NamedTuple):
    max_distance: int
    cycle_time_ms: int
    yield_per_cycle: int


class AsteroidMinerI(IOTerminal):

    class Status(Enum):
        SUCCESS = "success"
        INTERNAL_ERROR = "internal server error"
        ASTEROID_DOESNT_EXIST = "asteroid doesn't exist!"
        MINER_IS_BUSY = "port doesn't exist"
        MINER_IS_IDLE = "port is not opened"
        ASTEROID_TOO_FAR = "port has been closed"
        NO_SPACE_AVAILABLE = "no space available"
        NOT_BOUND_TO_CARGO = "not bound to cargo"
        INTERRUPTED_BY_USER = "interrupted by user"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        def is_ok(self):
            return self == AsteroidMinerI.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IResourceContainer.Status) -> "AsteroidMinerI.Status":
            ProtobufStatus = public.IAsteroidMiner.Status
            ModuleStatus = AsteroidMinerI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INTERNAL_ERROR: ModuleStatus.INTERNAL_ERROR,
                ProtobufStatus.ASTEROID_DOESNT_EXIST: ModuleStatus.ASTEROID_DOESNT_EXIST,
                ProtobufStatus.MINER_IS_BUSY: ModuleStatus.MINER_IS_BUSY,
                ProtobufStatus.MINER_IS_IDLE: ModuleStatus.MINER_IS_IDLE,
                ProtobufStatus.ASTEROID_TOO_FAR: ModuleStatus.ASTEROID_TOO_FAR,
                ProtobufStatus.NO_SPACE_AVAILABLE: ModuleStatus.NO_SPACE_AVAILABLE,
                ProtobufStatus.NOT_BOUND_TO_CARGO: ModuleStatus.NOT_BOUND_TO_CARGO,
                ProtobufStatus.INTERRUPTED_BY_USER: ModuleStatus.INTERRUPTED_BY_USER
            }[status]

    MiningReportCallback = Callable[[Status, Optional[ResourceItem]], bool]

    def __init__(self, name: Optional[str] = None):
        if not name:
            name = utils.generate_name(AsteroidMinerI)
        super().__init__(name=name)

    async def get_specification(self, timeout: float = 0.5)\
            -> (Status, Optional[Specification]):
        status = AsteroidMinerI.Status
        request = public.Message()
        request.asteroid_miner.specification_req = True
        if not self.send(message=request):
            return status.FAILED_TO_SEND_REQUEST, None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return status.RESPONSE_TIMEOUT, None
        spec = protocol.get_message_field(response, "asteroid_miner.specification")
        if not spec:
            return status.UNEXPECTED_RESPONSE, None
        spec = Specification(max_distance=spec.max_distance,
                             cycle_time_ms=spec.cycle_time_ms,
                             yield_per_cycle=spec.yield_per_cycle)
        return status.SUCCESS, spec

    async def bind_to_cargo(self, cargo_name: str, timeout: float = 0.5) -> Status:
        """Bind miner to the container with the specified 'cargo_name'"""
        request = public.Message()
        request.asteroid_miner.bind_to_cargo = cargo_name
        if not self.send(message=request):
            return AsteroidMinerI.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMinerI.Status.RESPONSE_TIMEOUT
        protobuf_status = protocol.get_message_field(
            response, "asteroid_miner.bind_to_cargo_status")
        if protobuf_status is None:
            return AsteroidMinerI.Status.UNEXPECTED_RESPONSE
        return AsteroidMinerI.Status.from_protobuf(protobuf_status)

    async def start_mining(self,
                           asteroid_id: int,
                           resource: ResourceType,
                           progress_cb: MiningReportCallback,
                           timeout: float = 0.5) -> Status:
        """Start mining asteroid with the specified 'asteroid_id' for the
        specified 'resource'. Only the specified resource will be retrieved.
        The specified 'progress_cb' will be called in the following cases:
        1. the mining report is received from server - in this case a
           'SUCCESS' status will be passed as the first argument, and a total
           retrieved resources (since last call) will be passed as the second
           argument. If callback returns False, then mining process will be
           interrupted, otherwise it will be resumed.
        3. some error occurred during mining - in this case an error code (status)
           will be passed as the first argument, and 'None' as the second argument;
           in this case a value, returned by callback, will be ignored and the mining
           process will be interrupted.
        """
        Status = AsteroidMinerI.Status
        
        spec_status, spec = await self.get_specification()
        if not spec_status.is_ok():
            return spec_status

        if not self._send_start_mining(asteroid_id=asteroid_id, resource=resource):
            return Status.FAILED_TO_SEND_REQUEST

        mining_status = await self._wait_start_mining_status(timeout=timeout)
        if not mining_status.is_ok():
            return mining_status

        report_timeout: float = 2 * spec.cycle_time_ms / 10000.0
        resume = True
        status = Status.INTERNAL_ERROR
        while resume:
            status, resource_item = await self._wait_mining_report(report_timeout)
            resume = status.is_ok() and progress_cb(Status.SUCCESS, resource_item)

        if status.is_ok():
            # Mining was interrupted by the player, so we should send stop_mining
            # to the module
            await self.stop_mining()
        return status

    async def stop_mining(self, timeout: float = 0.5) -> Status:
        """Stop the mining process"""
        request = public.Message()
        request.asteroid_miner.stop_mining = True
        if not self.send(message=request):
            return AsteroidMinerI.Status.FAILED_TO_SEND_REQUEST

        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMinerI.Status.RESPONSE_TIMEOUT
        status = protocol.get_message_field(response, "asteroid_miner.stop_mining_status")
        if status is None:
            return AsteroidMinerI.Status.UNEXPECTED_RESPONSE
        return AsteroidMinerI.Status.from_protobuf(status)

    def _send_start_mining(self, asteroid_id: int, resource: ResourceType):
        request = public.Message()
        body = request.asteroid_miner.start_mining
        body.asteroid_id = asteroid_id
        body.resource = resource.to_protobuf()
        return self.send(message=request)

    async def _wait_start_mining_status(self, timeout: float = 0.5) -> Status:
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMinerI.Status.RESPONSE_TIMEOUT
        mining_status = protocol.get_message_field(response, "asteroid_miner.start_mining_status")
        if mining_status is None:
            return AsteroidMinerI.Status.UNEXPECTED_RESPONSE
        return AsteroidMinerI.Status.from_protobuf(mining_status)

    async def _wait_mining_report(self, timeout: float) \
            -> (Status, Optional[ResourceItem]):
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMinerI.Status.RESPONSE_TIMEOUT, None
        report = protocol.get_message_field(response, "asteroid_miner.mining_report")
        if report is not None:
            return AsteroidMinerI.Status.SUCCESS, ResourceItem.from_protobuf(report)

        # May be mining was interrupted, that is why we haven't received the report?
        stop_ind = protocol.get_message_field(response, "asteroid_miner.mining_is_stopped")
        if stop_ind is not None:
            return AsteroidMinerI.Status.from_protobuf(stop_ind), None

        return AsteroidMinerI.Status.UNEXPECTED_RESPONSE, None
