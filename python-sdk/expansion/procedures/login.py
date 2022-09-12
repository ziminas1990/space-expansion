import logging.config
from typing import Optional

from expansion.transport import (
    UdpChannel,
    ProtobufChannel,
    Channel,
    SessionsMux
)
from expansion.modules.factory import module_factory

import expansion.api as api
from expansion.interfaces.rpc import AccessPanelI
from expansion.modules import Commutator

logging.basicConfig(level=logging.DEBUG)


async def login(server_ip: str,
                login_port: int,
                login: str,
                password: str)\
        -> (Optional[Commutator], Optional[str]):

    available_sessions: [Channel] = []
    session_mux = SessionsMux()

    async def open_new_connection() -> (Optional[Channel], Optional[str]):
        if len(available_sessions) > 0:
            return available_sessions.pop(-1), None

        # Seems that we should open yet another UDP connection to server
        udp_channel = UdpChannel(
            on_closed_cb=lambda: logging.warning(f"Socket is closed!"))
        if not await udp_channel.open():
            return None, "Failed to open UDP connection!"
        udp_channel.set_remote(server_ip, login_port)
        protobuf_channel = ProtobufChannel(message_type=api.Message)
        access_panel = AccessPanelI()

        # link "socket <-> protobuf <-> access panel" stack
        udp_channel.attach_to_terminal(protobuf_channel)
        protobuf_channel.attach_channel(udp_channel)
        access_panel.attach_to_channel(protobuf_channel)

        remote_port, session_id, additional_sessions, error = \
            await access_panel.login(login, password)
        if error is not None:
            return None, f"Login failed: {error}"
        udp_channel.set_remote(server_ip, remote_port)

        # "access panel" is not needed anymore, so we can replace it
        # with sessions_mux:
        protobuf_channel.attach_to_terminal(session_mux)
        root_session = session_mux.on_session_opened(
            session_id=session_id,
            channel=protobuf_channel)

        # Store additional sessions for further usage
        available_sessions.extend([
            session_mux.on_session_opened(
                session_id=additional_session_id,
                channel=protobuf_channel)
            for additional_session_id in additional_sessions
        ])
        return root_session, None

    return Commutator(
        session_mux=session_mux,
        tunnel_factory=open_new_connection,
        modules_factory=module_factory,
        name="Root"
    ), None
