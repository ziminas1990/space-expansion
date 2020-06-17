from typing import Optional, Tuple

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.channel import Channel, ChannelMode
from .types import Position


class INavigation:

    def __init__(self, name: Optional[str] = None):
        self.channel: Optional[Channel] = None

    def attach_to_channel(self, channel: Channel):
        assert channel and channel.mode == ChannelMode.PASSIVE
        self.channel = channel

    async def get_position(self) -> Optional[Position]:
        request = public.Message()
        request.navigation.position_req = True
        if not self.channel.send(message=request):
            return None
        response = await self.channel.receive()
        if not response:
            return None
        position = get_message_field(response, ["navigation", "position"])
        if not position:
            return None
        return Position(x=position.x, y=position.y, vx=position.vx, vy=position.vy)
