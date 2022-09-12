from typing import Dict, Any, Optional
from expansion.transport import Channel, Terminal, Session


class SessionsMux(Terminal):

    def __init__(self, trace_mode=False, *args, **kwargs):
        super().__init__("Router", trace_mode, *args, **kwargs)
        self.sessions: Dict[int, Session] = {}
        self.channel: Optional[Channel] = None

    # Register a new session with the specified 'session_id'.
    # Use the specified 'channel' to send messages with 'session_id'
    # session tag. Return a session object, that can be attached to uplevel
    # terminal.
    def on_session_opened(self, session_id: int,
                          channel: Channel = None) -> Channel:
        # Note: session object is not attached to the terminal yet, it should
        # be done by client, who has just opened a session.
        assert(session_id not in self.sessions)
        session = Session(session_id=session_id,
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

    def on_receive(self, message: Any, timestamp: Optional[int]):
        if self._trace_mode:
            self.terminal_logger.debug(f"Got\n{message}")
        try:
            session = self.sessions[message.tunnelId]
            if message.WhichOneof("choice") == "commutator":
                if message.commutator.WhichOneof("choice") == "open_tunnel_report":
                    # Another session has been opened using this one. Spawn a
                    # new 'session' object for the session.
                    self.on_session_opened(
                        session_id=message.commutator.open_tunnel_report,
                        channel=session.channel)
            if message.WhichOneof("choice") == "session":
                if message.session.WhichOneof("choice") == "closed_ind":
                    session.on_channel_closed()
                    self.sessions.pop(session.session_id)
                elif message.session.WhichOneof("choice") == "heartbeat":
                    # A heartbeat message should be just sent back
                    # No need to pass it to uplevel
                    session.send(message)
                return
            # Pass a message to a client
            if timestamp is None:
                timestamp = message.timestamp
            session.on_receive(message, timestamp)
        except KeyError:
            self.terminal_logger.error(f"invalid session {message.tunnelId}")

    def attach_channel(self, channel: 'Channel'):
        assert False, "Operation makes no sense, use create_session()"

    def on_channel_closed(self):
        assert False, "Operation makes no sense"
