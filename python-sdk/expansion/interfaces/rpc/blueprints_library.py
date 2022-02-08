from typing import Optional, List
from enum import Enum

import expansion.api as api
from expansion.transport import IOTerminal, Channel
from expansion import utils
from expansion.types import Blueprint


class Status(Enum):
    SUCCESS = "success"
    INTERNAL_ERROR = "internal error"
    BLUEPRINT_NOT_FOUND = "blueprint not found"

    # Problems, detected on SDK side
    FAILED_TO_SEND_REQUEST = "failed to send request"
    UNREACHABLE = "remote side is unreachable"
    RESPONSE_TIMEOUT = "response timeout"
    UNEXPECTED_RESPONSE = "unexpected response"
    CHANNEL_CLOSED = "channel closed"
    CANCELED = "operation canceled"

    def is_success(self):
        return self == Status.SUCCESS

    @staticmethod
    def from_protobuf(status: api.IResourceContainer.Status) -> "Status":
        PbStatus = api.IBlueprintsLibrary.Status
        return {
            PbStatus.SUCCESS: Status.SUCCESS,
            PbStatus.INTERNAL_ERROR: Status.INTERNAL_ERROR,
            PbStatus.BLUEPRINT_NOT_FOUND: Status.BLUEPRINT_NOT_FOUND,
        }[status]


class BlueprintsLibraryI(IOTerminal):

    def __init__(self, name: Optional[str] = None):
        if not name:
            name = utils.generate_name(BlueprintsLibraryI)
        super().__init__(name=name)

    @Channel.return_on_close(Status.CHANNEL_CLOSED, [])
    async def get_blueprints_list(self,
                                  start_with: str = "",
                                  timeout: float = 0.5) -> (Status, List[str]):
        request = api.Message()
        request.blueprints_library.blueprints_list_req = start_with
        if not self.send(request):
            return Status.FAILED_TO_SEND_REQUEST, []

        names: List[str] = []
        done = False
        while not done:
            response, _ = await self.wait_message(timeout=timeout)
            if not response:
                return Status.RESPONSE_TIMEOUT, []
            response = api.get_message_field(
                response,
                ["blueprints_library", "blueprints_list"])
            if not response:
                return Status.UNEXPECTED_RESPONSE, []
            names.extend(response.names)
            done = response.left == 0
        return Status.SUCCESS, names

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def get_blueprint(self,
                            blueprint_name: str,
                            timeout: float = 0.5) \
            -> (Status, Optional[Blueprint]):
        request = api.Message()
        request.blueprints_library.blueprint_req = blueprint_name
        if not self.send(request):
            return Status.FAILED_TO_SEND_REQUEST, None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return Status.RESPONSE_TIMEOUT, None

        blueprint = api.get_message_field(
                response, ["blueprints_library", "blueprint"])
        if blueprint:
            return Status.SUCCESS, Blueprint.from_protobuf(blueprint)

        fail = api.get_message_field(
            response, ["blueprints_library", "blueprint_fail"])
        if fail:
            return Status.from_protobuf(fail), None

