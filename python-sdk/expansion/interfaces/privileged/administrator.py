from typing import Optional, Dict
from expansion.transport import UdpChannel, ProtobufChannel, Terminal
import expansion.api as api
from expansion.utils import generate_name
from expansion.interfaces.privileged import (
    Access,
    SystemClock,
    Demux
)


class Administrator:
    """Administrator panel aggregates all components, that implements parts of
    privileged interface"""

    def __init__(self):
        self.name = generate_name(type(self))

        # Instantiating components of the stack
        self.udp_channel: UdpChannel = UdpChannel(
            self.on_channel_closed, channel_name=f"{self.name}.UDP"
        )
        self.protobuf_channel: ProtobufChannel = ProtobufChannel(
            message_type=api.admin.Message,
            channel_name=f"{self.name}.Protobuf")
        self.demux = Demux(f"{self.name}.demux")

        self.token: Optional[int] = None
        self.access_panel: Access = Access(f"{self.name}.access")
        self.system_clock = SystemClock(f"{self.name}.clock")

        # Linking components of the stack
        self.access_panel.attach_channel(self.demux)
        self.system_clock.attach_channel(self.demux)
        self.demux.attach_terminals({
            "access": self.access_panel,
            "system_clock": self.system_clock
        })
        self.demux.attach_channel(self.protobuf_channel)
        self.protobuf_channel.attach_to_terminal(self.demux)
        self.protobuf_channel.attach_channel(self.udp_channel)
        self.udp_channel.attach_to_terminal(self.protobuf_channel)

    def on_channel_closed(self):
        self.token = None
        pass

    async def login(self,
                    ip_address: str,
                    port: int,
                    login: str,
                    password: str) -> (bool, Optional[str]):
        """Login on server. If login is done, other parts of privileged interfaces
        may be used"""
        if self.token is not None:
            return False, "Session is already opened!"

        if not await self.udp_channel.open(ip_address, port):
            return False, f"Failed to open UDP socket (dest: {ip_address}:{port})"

        status, self.token = await self.access_panel.login(login, password)
        if not status:
            return False, "Login has been rejected by server"
        self.demux.set_token(self.token)
        return True, None

    def get_clock(self) -> SystemClock:
        return self.system_clock
