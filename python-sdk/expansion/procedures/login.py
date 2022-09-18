import asyncio
import logging.config
from typing import Optional, Tuple, NamedTuple

from expansion.transport import (
    UdpChannel,
    ProtobufChannel,
    Channel,
    SessionsMux
)
from expansion.modules.factory import module_factory

import expansion.api as api
from expansion.interfaces.rpc import AccessPanelI, RootSession
import expansion.modules as modules

logging.basicConfig(level=logging.DEBUG)


class Connection(NamedTuple):
    root_session: RootSession
    commutator: modules.Commutator

    def close(self):
        self.root_session.close()


async def login(server_ip: str,
                login_port: int,
                login: str,
                password: str) -> (Optional[Connection], Optional[str]):

    session_mux = SessionsMux()

    # Build stack: access_panel <-> protobuf_channel <-> udp socket
    udp_channel = UdpChannel(
        on_closed_cb=lambda: logging.warning(f"Socket is closed!"))
    if not await udp_channel.open():
        return None, "Failed to open UDP channel!"
    udp_channel.set_remote(server_ip, login_port)

    protobuf_channel = ProtobufChannel(message_type=api.Message)
    access_panel = AccessPanelI()

    udp_channel.attach_to_terminal(protobuf_channel)
    protobuf_channel.attach_channel(udp_channel)
    access_panel.attach_to_channel(protobuf_channel)

    # Send login request and get remote port and root session_id
    remote_port, root_session_id, error = \
        await access_panel.login(login, password)
    if error is not None:
        return None, f"Login failed: {error}"
    udp_channel.set_remote(server_ip, remote_port)

    # Access panel is not required anymore and can be replaced with
    # SessionMux, because all incoming messages should be forwarded
    # to it.
    protobuf_channel.attach_to_terminal(session_mux)

    # Create RootSession instance
    root_session_channel = session_mux.on_session_opened(
        session_id=root_session_id,
        channel=protobuf_channel)
    root_session = RootSession()
    root_session_channel.attach_to_terminal(root_session)
    root_session.attach_channel(root_session_channel)

    # Define a function, that opens a new session to root commutator
    mutex = asyncio.Lock()
    async def open_commutator_session() \
            -> Tuple[Optional[Channel], Optional[str]]:
        async with mutex:
            session_id, problem = await root_session.open_commutator_session()
            if session_id is not None:
                session = session_mux.on_session_opened(
                    session_id, protobuf_channel)
                return session, None
            else:
                return None, f"failed to open commutator session: {problem}"

    return Connection(
        root_session=root_session,
        commutator=modules.Commutator(
            session_mux=session_mux,
            tunnel_factory=open_commutator_session,
            modules_factory=module_factory,
            name="Root"
        )
    ), None
