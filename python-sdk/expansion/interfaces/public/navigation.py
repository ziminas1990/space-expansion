from typing import Optional, Any

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.channel import Channel, ChannelMode
from expansion.transport.terminal import Terminal
from .types import Position

import expansion.utils as utils


class INavigation(Terminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(INavigation))
        self.channel: Optional[Channel] = None

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

    # Overrides of Terminal
    def on_receive(self, message: Any):
        super(INavigation, self).on_receive(message=message)  #For logging

    # Overrides of Terminal
    def attach_channel(self, channel: 'Channel'):
        super(INavigation, self).attach_channel(channel=channel)  #For logging
        assert channel and channel.mode == ChannelMode.PASSIVE
        self.channel = channel

    # Overrides of Terminal
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        super(INavigation, self).on_channel_mode_changed(mode=mode)  #For logging

    # Overrides of Terminal
    def on_channel_detached(self):
        super(INavigation, self).on_channel_detached()  #For logging