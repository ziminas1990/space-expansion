from typing import Optional, Tuple, Type
from enum import Enum

import expansion.api as api
from expansion.transport import IOTerminal
import expansion.types as types
from expansion.api.utils import get_message_field


class BasicManipulator(IOTerminal):

    class Status(Enum):
        SUCCESS = 0
        OBJECT_DOESNT_EXIST = 1
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        CHANNEL_CLOSED = "channel closed"
        CANCELED = "operation canceled"
        UNKNOWN = "unknown"

        def is_success(self):
            return self == BasicManipulator.Status.SUCCESS

        @staticmethod
        def from_protobuf(mode: api.admin.BasicManipulator.Status):
            try:
                return {
                    api.admin.BasicManipulator.Status.OBJECT_DOESNT_EXIST:
                        BasicManipulator.Status.OBJECT_DOESNT_EXIST,
                }[mode]
            except KeyError:
                return BasicManipulator.Status.UNKNOWN

    def __init__(self, name: str, *args, **kwargs):
        super(BasicManipulator, self).__init__(name=name, *args, **kwargs)

    async def get_object(self,
                         object_type: types.ObjectType,
                         object_id: int) \
            -> Tuple[Status, Optional[types.PhysicalObject]]:
        Status = BasicManipulator.Status

        request = api.admin.Message()
        body = request.manipulator.object_req
        body.object_type = object_type.to_protobuf()
        body.id = object_id
        if not self.send(request):
            return Status.FAILED_TO_SEND_REQUEST, None

        response, timestamp = await self.wait_exact(["manipulator"])
        if not response:
            return Status.RESPONSE_TIMEOUT, None

        obj = get_message_field(response, ["object"])
        if obj is not None:
            return Status.SUCCESS, types.PhysicalObject(
                object_type=types.ObjectType.from_protobuf(obj.object_type),
                object_id=obj.id,
                position=types.Position(
                    x=obj.x,
                    y=obj.y,
                    velocity=types.Vector(x=obj.vx, y=obj.vy),
                    timestamp=types.TimePoint(timestamp))
            )

        problem = get_message_field(response, ["problem"])
        if problem:
            return Status.from_protobuf(problem), None

        return Status.UNEXPECTED_RESPONSE, None

    # Return the same position, but with timestamp, received from server
    async def move_object(self,
                          object_type: types.ObjectType,
                          object_id: int,
                          position: types.Position) \
            -> Tuple[Status, Optional[types.Position]]:
        Status = BasicManipulator.Status

        request = api.admin.Message()
        body = request.manipulator.move

        body.object_id.object_type = object_type.to_protobuf()
        body.object_id.id = object_id
        position.to_protobuf(body.position)
        if not self.send(request):
            return Status.FAILED_TO_SEND_REQUEST, None

        response, timestamp = await self.wait_exact(["manipulator"])

        timestamp = get_message_field(response, ["moved_at"])
        if timestamp is not None:
            return Status.SUCCESS, position.with_timestamp(timestamp)

        problem = get_message_field(response, ["problem"])
        if problem:
            return Status.from_protobuf(problem), None

        return Status.UNEXPECTED_RESPONSE, None
