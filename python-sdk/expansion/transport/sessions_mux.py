from typing import Dict, Any, Optional, TYPE_CHECKING
from expansion.transport import Channel, Terminal

if TYPE_CHECKING:
    from expansion.transport import Channel


class SessionsMux(Terminal):

    class Session(Channel, Terminal):
        def __init__(self,
                     session_id: int,
                     owner: "SessionsMux",
                     name: Optional[str] = None,
                     trace_mode: bool = False,
                     *args, **kwargs):
            super().__init__(name, trace_mode, *args, **kwargs)
            self.session_id = session_id
            self.owner = owner

        # Override from Channel
        def send(self, message: Any) -> bool:
            # Just add a tunnel_id
            message.tunnelId = self.session_id
            return self.channel.send(message)

        # Override from Terminal
        def on_receive(self, message: Any, timestamp: Optional[int]):
            self.terminal.on_receive(message, timestamp)
            if message.commutator and message.commutator.close_tunnel_ind:
                self.terminal.on_channel_detached()
                self.close()

        # Override from Channel
        async def close(self):
            self.detach_terminal()
            self.owner.on_session_closed(self.session_id)

    def __init__(self, trace_mode=False, *args, **kwargs):
        super().__init__("Router", trace_mode, *args, **kwargs)
        self.sessions: Dict[int, SessionsMux.Session] = {}
        self.channel: Optional[Channel] = None

    def create_session(self, session_id: int) -> Channel:
        assert(session_id not in self.sessions)
        assert(self.channel is not None)
        session = SessionsMux.Session(
            session_id=session_id,
            owner=self,
            name=self.get_name(),
            trace_mode=self.trace_mode())
        session.attach_channel(self.channel)
        self.sessions.update({
            session_id: session
        })
        return session

    def on_session_closed(self, session_id):
        self.sessions.pop(session_id)

    def on_receive(self, message: Any, timestamp: Optional[int]):
        if self._trace_mode:
            self.terminal_logger.debug(f"Got\n{message}")
        try:
            self.sessions[message.tunnelId].on_receive(message, timestamp)
        except KeyError:
            self.terminal_logger.error(f"invalid session")

    def attach_channel(self, channel: 'Channel'):
        self.terminal_logger.info(f"Attached to channel {channel.channel_name}")
        self.channel = channel
        for session in self.sessions.values():
            session.attach_channel(channel)

    def on_channel_detached(self):
        self.terminal_logger.info(f"Channel detached")
        self.channel = None
        for session in self.sessions.values():
            session.on_channel_detached()

