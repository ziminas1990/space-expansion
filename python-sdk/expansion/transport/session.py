import asyncio
from typing import Any, Optional
from enum import Enum
import logging

from expansion.transport import Channel, Terminal
import expansion.api as api


class Session(Channel, Terminal):
    class State(Enum):
        ACTIVE = 0
        CLOSING = 1
        CLOSED = 2

    def __init__(self,
                 session_id: int,
                 name: Optional[str] = None,
                 trace_mode: bool = False,
                 *args, **kwargs):
        super().__init__(name, trace_mode, *args, **kwargs)
        self.session_logger = logging.getLogger(name)
        self.state = Session.State.ACTIVE
        self.session_id = session_id

    # Override from Channel
    def send(self, message: Any) -> bool:
        # Just add a tunnel_id
        #self.session_logger.debug(f"Send: \n{message}")
        message.tunnelId = self.session_id
        return self.channel and self.channel.send(message)

    # Override from Terminal
    def on_receive(self, message: Any, timestamp: Optional[int]):
        if self.state.ACTIVE:
            if self.terminal is None:
                assert False, f"WARNING: Drop message (no terminal attached):" \
                              f"\n{message}"
            assert self.terminal is not None
            #self.session_logger.debug(f"Received: \n{message}")
            self.terminal.on_receive(message, timestamp)

    def on_channel_closed(self):
        """Detach terminal from channel. After it is done, terminal won't
        be able to send messages anymore"""
        super().on_channel_closed()
        self.state = Session.State.CLOSED
        if self.terminal is not None:
            self.terminal.on_channel_closed()
            self.terminal = None

    # Override from Channel
    async def close(self):
        # TODO: verify that channel is closed, otherwise make a number
        #       of attempts to close it (see SES-179)
        request = api.Message()
        request.session.close = True
        if not self.send(request):
            self.state = Session.State.CLOSED
