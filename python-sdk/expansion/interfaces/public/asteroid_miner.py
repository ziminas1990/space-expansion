from typing import Optional, NamedTuple
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
import expansion.protocol as protocol
import expansion.transport as transport
import expansion.utils as utils
from expansion.types import ResourceType


class Specification(NamedTuple):
    max_distance: int
    cycle_time_ms: int
    yield_per_cycle: int


class AsteroidMiner(transport.QueuedTerminal):

    class Status(Enum):
        SUCCESS = "success"
        INTERNAL_ERROR = "internal server error"
        ASTEROID_DOESNT_EXIST = "port is already opened"
        MINER_IS_BUSY = "port doesn't exist"
        MINER_IS_IDLE = "port is not opened"
        ASTEROID_TOO_FAR = "port has been closed"
        NO_SPACE_AVAILABLE = "no space available"
        NOT_BOUND_TO_CARGO = "not bound to cargo"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"

        def is_ok(self):
            return self == AsteroidMiner.Status.SUCCESS

        @staticmethod
        def from_protobuf(status: public.IResourceContainer.Status) -> "AsteroidMiner.Status":
            ProtobufStatus = public.IAsteroidMiner.Status
            ModuleStatus = AsteroidMiner.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INTERNAL_ERROR: ModuleStatus.INTERNAL_ERROR,
                ProtobufStatus.ASTEROID_DOESNT_EXIST: ModuleStatus.ASTEROID_DOESNT_EXIST,
                ProtobufStatus.MINER_IS_BUSY: ModuleStatus.MINER_IS_BUSY,
                ProtobufStatus.MINER_IS_IDLE: ModuleStatus.MINER_IS_IDLE,
                ProtobufStatus.ASTEROID_TOO_FAR: ModuleStatus.ASTEROID_TOO_FAR,
                ProtobufStatus.NO_SPACE_AVAILABLE: ModuleStatus.NO_SPACE_AVAILABLE,
                ProtobufStatus.NOT_BOUND_TO_CARGO: ModuleStatus.NOT_BOUND_TO_CARGO
            }[status]

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(AsteroidMiner))
        self.specification: Optional[Specification] = None
        self.cargo_name: Optional[str] = None
        # A name of the resource container, to which the miner is attached

    async def get_specification(self, timeout: float = 0.5, reset_cached=False)\
            -> Optional[Specification]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        request = public.Message()
        request.asteroid_miner.specification_req = True
        if not self.send_message(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        spec = protocol.get_message_field(response, "asteroid_miner.specification")
        if not spec:
            return None
        self.specification = Specification(max_distance=spec.max_distance,
                                           cycle_time_ms=spec.cycle_time_ms,
                                           yield_per_cycle=spec.yield_per_cycle)
        return self.specification

    async def bind_to_cargo(self, cargo_name: str, timeout: float = 0.5) -> Status:
        """Bind miner to the container with the specified 'cargo_name'"""
        request = public.Message()
        request.asteroid_miner.bind_to_cargo = cargo_name
        if not self.send_message(message=request):
            return AsteroidMiner.Status.FAILED_TO_SEND_REQUEST
        response = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMiner.Status.RESPONSE_TIMEOUT
        protobuf_status = protocol.get_message_field(
            response, "asteroid_miner.bind_to_cargo_status")
        if protobuf_status is None:
            return AsteroidMiner.Status.UNEXPECTED_RESPONSE
        status = AsteroidMiner.Status.from_protobuf(protobuf_status)
        if status == AsteroidMiner.Status.SUCCESS:
            self.cargo_name = cargo_name
        return status

    async def start_mining(self,
                           asteroid_id: int,
                           resource: ResourceType,
                           timeout: float = 0.5) -> Status:
        """Start mining asteroid with the specified 'asteroid_id' for the
        specified 'resource'. Only the specified resource will be retrieved"""
        request = public.Message()
        body = request.asteroid_miner.start_mining
        body.asteroid_id = asteroid_id
        body.resource = resource.to_protobuf()
        if not self.send_message(message=request):
            return AsteroidMiner.Status.FAILED_TO_SEND_REQUEST

        response = await self.wait_message(timeout=timeout)
        if not response:
            return AsteroidMiner.Status.RESPONSE_TIMEOUT
        status = protocol.get_message_field(response, "asteroid_miner.start_mining_status")
        if status is None:
            return AsteroidMiner.Status.UNEXPECTED_RESPONSE
        return AsteroidMiner.Status.from_protobuf(status)
