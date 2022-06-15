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

    session_mux = SessionsMux()

    async def tunnel_factory() -> (Optional[Channel], Optional[str]):
        udp_channel = UdpChannel(
            on_closed_cb=lambda: logging.warning(f"Socket is closed!"))
        if not await udp_channel.open():
            return None, "Failed to open UDP connection!"
        udp_channel.set_remote(server_ip, login_port)

        protobuf_channel = ProtobufChannel(message_type=api.Message)
        access_panel = AccessPanelI()

        udp_channel.attach_to_terminal(protobuf_channel)
        protobuf_channel.attach_channel(udp_channel)
        access_panel.attach_to_channel(protobuf_channel)

        remote_port, session_id, error = \
            await access_panel.login(login, password)
        if error is not None:
            return None, f"Login failed: {error}"
        udp_channel.set_remote(server_ip, remote_port)

        session_mux.attach_channel(protobuf_channel)
        protobuf_channel.attach_to_terminal(session_mux)

        session = session_mux.create_session(session_id=session_id)
        return session, None

    return Commutator(
        session_mux=session_mux,
        tunnel_factory=tunnel_factory,
        modules_factory=module_factory,
        name="Root"
    ), None
