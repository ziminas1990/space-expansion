from typing import Optional, Any

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.queued_terminal import QueuedTerminal
from .types import Position

import expansion.utils as utils


class INavigation(QueuedTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(INavigation))

    async def get_position(self) -> Optional[Position]:
        request = public.Message()
        request.navigation.position_req = True
        if not self.channel.send(message=request):
            return None
        response = await self.wait_message(timeout=0.5)
        if not response:
            return None
        position = get_message_field(response, ["navigation", "position"])
        if not position:
            return None
        return Position(x=position.x, y=position.y, vx=position.vx, vy=position.vy)