from typing import Optional, NamedTuple, Dict, Callable, List, Tuple
from enum import Enum

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport import IOTerminal, Channel
from expansion.types import ResourceType, ResourceItem

import expansion.utils as utils


class ResourceContainerI(IOTerminal):

    class Status(Enum):
        SUCCESS = "success"
        INTERNAL_ERROR = "internal server error"
        PORT_ALREADY_OPEN = "port is already opened"
        PORT_DOESNT_EXIST = "port doesn't exist"
        PORT_IS_NOT_OPENED = "port is not opened"
        PORT_HAS_BEEN_CLOSED = "port has been closed"
        INVALID_ACCESS_KEY = "invalid access key"
        INVALID_RESOURCE_TYPE = "invalid resource type"
        PORT_TOO_FAR = "receiver is too far"
        TRANSFER_IN_PROGRESS = "transfer in progress"
        NOT_ENOUGH_RESOURCES = "not enough resources"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        CHANNEL_CLOSED = "channel closed"
        CANCELED = "operation canceled"

        def is_success(self):
            return self == ResourceContainerI.Status.SUCCESS

        def is_timeout(self):
            return self == ResourceContainerI.Status.RESPONSE_TIMEOUT

        @staticmethod
        def convert(status: public.IResourceContainer.Status) -> "ResourceContainerI.Status":
            ProtobufStatus = public.IResourceContainer.Status
            ModuleStatus = ResourceContainerI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INTERNAL_ERROR: ModuleStatus.INTERNAL_ERROR,
                ProtobufStatus.PORT_ALREADY_OPEN: ModuleStatus.PORT_ALREADY_OPEN,
                ProtobufStatus.PORT_DOESNT_EXIST: ModuleStatus.PORT_DOESNT_EXIST,
                ProtobufStatus.PORT_IS_NOT_OPENED: ModuleStatus.PORT_IS_NOT_OPENED,
                ProtobufStatus.PORT_HAS_BEEN_CLOSED: ModuleStatus.PORT_HAS_BEEN_CLOSED,
                ProtobufStatus.INVALID_ACCESS_KEY: ModuleStatus.INVALID_ACCESS_KEY,
                ProtobufStatus.INVALID_RESOURCE_TYPE: ModuleStatus.INVALID_RESOURCE_TYPE,
                ProtobufStatus.PORT_TOO_FAR: ModuleStatus.PORT_TOO_FAR,
                ProtobufStatus.TRANSFER_IN_PROGRESS: ModuleStatus.TRANSFER_IN_PROGRESS,
                ProtobufStatus.NOT_ENOUGH_RESOURCES: ModuleStatus.NOT_ENOUGH_RESOURCES,
            }[status]

    class Content(NamedTuple):
        timestamp: int
        volume: int
        used: float
        resources: Dict[ResourceType, float]

        def as_list(self) -> List[ResourceItem]:
            return [
                ResourceItem(resource_type, amount) for
                resource_type, amount in self.resources.items()
            ]

        def print_content(self) -> str:
            return ", ".join(
                [ str(resource) for resource in self.as_list()]
            )

        def print_status(self) -> str:
            used = 100 * self.used / self.volume
            return f"{used:.2f}% used: {self.print_content()}"

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(ResourceContainerI))

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def get_content(self, timeout: float = 0.5) -> \
            Tuple[Status, Optional[Content]]:
        request = public.Message()
        request.resource_container.content_req = True
        if not self.send(message=request):
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST, None
        return await self.wait_content(timeout)

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def wait_content(self, timeout: float = 0.5) -> \
            Tuple[Status, Optional[Content]]:
        response, timestamp = await self.wait_message(timeout=timeout)
        if not response:
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST, None
        content = get_message_field(
            response, ["resource_container", "content"])
        if not content:
            return ResourceContainerI.Status.RESPONSE_TIMEOUT, None
        content = ResourceContainerI.Content(
            timestamp=timestamp,
            volume=content.volume,
            used=content.used,
            resources={ResourceType.from_protobuf(item.type): item.amount
                       for item in content.resources}
        )
        return ResourceContainerI.Status.SUCCESS, content

    @Channel.return_on_close(Status.CHANNEL_CLOSED, 0)
    async def open_port(self, access_key: int, timeout: int = 0.5) -> (Status, int):
        """Open a new port with the specified 'access_key'. Return operation status
        and port number"""
        request = public.Message()
        request.resource_container.open_port = access_key
        if not self.send(message=request):
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST, 0
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ResourceContainerI.Status.RESPONSE_TIMEOUT, 0
        port_id = get_message_field(
            response, ["resource_container", "port_opened"])
        if port_id is not None:
            # Success case
            return ResourceContainerI.Status.SUCCESS, port_id

        error_status = get_message_field(
            response, ["resource_container", "open_port_failed"])
        if error_status is not None:
            return ResourceContainerI.Status.convert(error_status), 0
        return ResourceContainerI.Status.UNEXPECTED_RESPONSE, 0

    @Channel.return_on_close(Status.CHANNEL_CLOSED)
    async def close_port(self, timeout: int = 0.5) -> Status:
        """Close an existing opened port"""
        request = public.Message()
        request.resource_container.close_port = True
        if not self.send(message=request):
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ResourceContainerI.Status.RESPONSE_TIMEOUT
        status = get_message_field(
            response, ["resource_container", "close_port_status"])
        if status is not None:
            # Success case
            return ResourceContainerI.Status.convert(status)

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def monitor(self, timeout: float = 0.5) -> Tuple[Status, Optional[Content]]:
        """Start monitoring session.
        Note: monitoring session can't be closed. The only way to stop
        monitoring session is close the channel.
        """
        request = public.Message()
        request.resource_container.monitor = True
        if not self.send(message=request):
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST, None
        return await self.wait_content(timeout)

    @Channel.return_on_close(Status.CHANNEL_CLOSED)
    async def transfer(self, port: int, access_key: int,
                       resource: ResourceItem,
                       progress_cb: Optional[Callable[[ResourceItem], None]] = None,
                       timeout: int = 0.5) -> Status:
        """Transfer the specified 'resource' to the specified 'port' with the
        specified 'access_key'. The optionally specified 'progress_cb' will be
        called to report transferring status (a total amount of transferred
        resources)."""
        request = public.Message()
        req_body = request.resource_container.transfer
        req_body.port_id = port
        req_body.access_key = access_key
        resource_item = req_body.resource
        resource.to_protobuf(resource_item)

        if not self.send(message=request):
            return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return ResourceContainerI.Status.RESPONSE_TIMEOUT
        status = get_message_field(
            response, ["resource_container", "transfer_status"])
        if status is None:
            return ResourceContainerI.Status.UNEXPECTED_RESPONSE
        if status != public.IResourceContainer.Status.SUCCESS:
            return ResourceContainerI.Status.convert(status)
        # Status is success. Waiting for reports
        while True:
            response, _ = await self.wait_message(timeout=2)
            report = get_message_field(response, ["resource_container", "transfer_report"])
            if not report:
                # May be complete status is received:
                status = get_message_field(response, ["resource_container", "transfer_finished"])
                return ResourceContainerI.Status.convert(status) \
                    if status is not None else ResourceContainerI.Status.UNEXPECTED_RESPONSE
            # Got transfer report:
            if progress_cb is not None:
                progress_cb(ResourceItem.from_protobuf(report))

