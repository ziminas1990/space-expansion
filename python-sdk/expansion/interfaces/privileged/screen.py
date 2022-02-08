from typing import Optional, List

from expansion.transport.channel import Channel
import expansion.api as api
from .types import ObjectType, PhysicalObject


class Screen:

    def __init__(self):
        self._channel: Optional[Channel] = None
        self._token: int = 0

    def attach_to_channel(self, channel: Channel, token: int):
        self._channel = channel
        self._token = token

    def _send_message(self, message: api.admin.Message):
        message.token = self._token
        self._channel.send(message)

    async def set_position(self,
                           center_x: float, center_y: float,
                           width: float, height: float):
        """Move screen with the specified 'width' and 'height' to
        the specified 'x' and 'y' position"""
        message = api.admin.Message()
        position = message.screen.move
        position.x = center_x
        position.y = center_y
        position.width = width
        position.height = height
        self._send_message(message)

        response = await self._channel.receive()
        status = api.get_message_field(response, ["screen", "status"])
        return status is not None and status == api.admin.Screen.Status.SUCCESS

    async def show(self, object_type: ObjectType) -> List[PhysicalObject]:
        message = api.admin.Message()
        message.screen.show = object_type.to_protobuf_type()
        self._send_message(message)

        shown_objects: List[PhysicalObject] = []

        while True:
            response = await self._channel.receive()
            items = api.get_message_field(response, ["screen", "objects"])
            if not items:
                break
            for item in items.object:
                shown_objects.append(PhysicalObject().from_protobuf(item))
            if items.left == 0:
                break
        return shown_objects
