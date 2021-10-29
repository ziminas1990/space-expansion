from typing import Optional, List

import expansion.protocol.Protocol_pb2 as api
import expansion.protocol as protocol
from expansion.transport import IOTerminal, Channel
from expansion import utils
from base_status import BaseStatus
from expansion.types import Blueprint


class BlueprintsLibraryI(IOTerminal):
    class Status(BaseStatus):
        INTERNAL_ERROR = "internal error"
        BLUEPRINT_NOT_FOUND = "blueprint not found"

        @staticmethod
        def from_protobuf(status: api.IResourceContainer.Status) -> "BlueprintsLibraryI.Status":
            ProtobufStatus = api.IBlueprintsLibrary.Status
            ModuleStatus = BlueprintsLibraryI.Status
            return {
                ProtobufStatus.SUCCESS: ModuleStatus.SUCCESS,
                ProtobufStatus.INTERNAL_ERROR: ModuleStatus.INTERNAL_ERROR,
                ProtobufStatus.BLUEPRINT_NOT_FOUND: ModuleStatus.BLUEPRINT_NOT_FOUND,
            }[status]

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
            return BlueprintsLibraryI.Status.FAILED_TO_SEND_REQUEST, []

        names: List[str] = []
        done = False
        while not done:
            response, _ = await self.wait_message(timeout=timeout)
            if not response:
                return BlueprintsLibraryI.Status.RESPONSE_TIMEOUT, []
            response = protocol.get_message_field(
                response,
                ["blueprints_library", "blueprints_list"])
            if not response:
                return BlueprintsLibraryI.Status.UNEXPECTED_RESPONSE, []
            names.extend(response.names)
            done = response.left == 0

    @Channel.return_on_close(Status.CHANNEL_CLOSED, None)
    async def get_blueprint(self,
                            blueprint_name: str,
                            timeout: float = 0.5) \
            -> (Status, Optional[Blueprint]):
        request = api.Message()
        request.blueprints_library.blueprint_req = blueprint_name
        if not self.send(request):
            return BlueprintsLibraryI.Status.FAILED_TO_SEND_REQUEST, None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return BlueprintsLibraryI.Status.RESPONSE_TIMEOUT, None

        blueprint = protocol.get_message_field(
                response, ["blueprints_library", "blueprint"])
        if blueprint:
            return BlueprintsLibraryI.Status.SUCCESS,\
                   Blueprint.from_protobuf(blueprint)

        fail = protocol.get_message_field(
            response, ["blueprints_library", "blueprint_fail"])
        if fail:
            return BlueprintsLibraryI.Status.from_protobuf(fail)

