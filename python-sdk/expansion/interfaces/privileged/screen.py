from typing import Optional, List

from expansion.transport import IOTerminal
import expansion.api as api
from .types import ObjectType, PhysicalObject


class Screen(IOTerminal):

    def __init__(self, name: str, *args, **kwargs):
        super(Screen, self).__init__(name=name, *args, **kwargs)

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
        self.send(message)

        response = await self.wait_message()
        status = api.get_message_field(response, ["screen", "status"])
        return status is not None and status == api.admin.Screen.Status.SUCCESS

    async def show(self, object_type: ObjectType) -> List[PhysicalObject]:
        message = api.admin.Message()
        message.screen.show = object_type.to_protobuf_type()
        self.send(message)

        shown_objects: List[PhysicalObject] = []

        while True:
            response = await self.wait_message()
            items = api.get_message_field(response, ["screen", "objects"])
            if not items:
                break
            for item in items.object:
                shown_objects.append(PhysicalObject().from_protobuf(item))
            if items.left == 0:
                break
        return shown_objects
