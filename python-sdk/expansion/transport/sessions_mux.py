from typing import Dict, Any, Optional, List, TYPE_CHECKING
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
            if message.commutator:
                if message.commutator.open_tunnel_report:
                    # Another session has been opened using this one
                    self.owner.on_session_created(
                        session_id=message.commutator.open_tunnel_report,
                        channel=self.channel)
                elif message.commutator.close_tunnel_ind:
                    self.owner.on_session_closed(self.session_id)
            # Pass a message to a client
            self.terminal.on_receive(message, timestamp)

        # Override from Channel
        async def close(self):
            self.detach_terminal()
            self.owner.on_session_closed(self.session_id)

    def __init__(self, trace_mode=False, *args, **kwargs):
        super().__init__("Router", trace_mode, *args, **kwargs)
        self.sessions: Dict[int, SessionsMux.Session] = {}
        self.channel: Optional[Channel] = None

    def on_session_created(self, session_id: int,
                           channel: Channel = None) -> Channel:
        assert(session_id not in self.sessions)
        session = SessionsMux.Session(
            session_id=session_id,
            owner=self,
            name=self.get_name(),
            trace_mode=self.trace_mode())
        session.attach_channel(channel)
        self.sessions.update({
            session_id: session
        })
        return session

    def get_channel_for_session(self, session_id: int) -> Optional[Channel]:
        try:
            return self.sessions[session_id]
        except KeyError:
            # Session has NOT been created
            return None

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
        assert False, "Operation makes no sense, use create_session()"

    def on_channel_detached(self):
        assert False, "Operation makes no sense"
