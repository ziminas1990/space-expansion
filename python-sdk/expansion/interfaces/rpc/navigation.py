from typing import Optional

import expansion.api as api
from expansion.transport import IOTerminal, Channel
from expansion.types import Position

import expansion.utils as utils


class INavigation(IOTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(name=name or utils.generate_name(INavigation))

    @Channel.return_on_close(None)
    async def get_position(self, timeout: float = 0.5) -> Optional[Position]:
        """Request current ship's position. Will block until the response
        is received or the specified 'timeout' occurs."""
        request = api.Message()
        request.navigation.position_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=timeout)
        if not response:
            return None
        position = api.get_message_field(response, ["navigation", "position"])
        if not position:
            return None
        assert response.timestamp is not None
        return Position.from_protobuf(position, timestamp=response.timestamp)
