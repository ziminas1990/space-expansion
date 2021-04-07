from typing import Optional

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport import IOTerminal
from expansion.types import Position, Vector

import expansion.utils as utils


class INavigation(IOTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(INavigation))

    async def get_position(self, timeout: float = 0.5) -> Optional[Position]:
        """Request current ship's position. Will block until the response
        is received or the specified 'timeout' occurs."""
        request = public.Message()
        request.navigation.position_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        position = get_message_field(response, "navigation.position")
        if not position:
            return None
        assert response.timestamp is not None
        return Position(x=position.x, y=position.y,
                        velocity=Vector(x=position.vx, y=position.vy),
                        timestamp=response.timestamp)
