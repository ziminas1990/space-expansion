from typing import Optional
import expansion.transport as transport
from expansion.protocol import PrivilegedMessage
from expansion.utils import generate_name

from .access import Access
from .system_clock import SystemClock


class Administrator:
    """Administrator panel aggregates all components, that implements parts of
    privileged interface"""

    def __init__(self):
        self.name = generate_name(type(self))
        self.udp_channel: transport.UdpChannel = transport.UdpChannel(
            self.on_channel_closed, channel_name=f"{self.name}.UDP"
        )
        self.protobuf_channel: transport.ProtobufChannel = transport.ProtobufChannel(
            message_type=PrivilegedMessage,
            mode=transport.ChannelMode.PASSIVE,
            channel_name=f"{self.name}.Protobuf")
        self.token: Optional[int] = None
        self.access_panel: Access = Access()
        self.system_clock = SystemClock()

        self.protobuf_channel.attach_channel(self.udp_channel)
        self.access_panel.attach_to_channel(self.protobuf_channel)

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
        self.system_clock.attach_to_channel(self.protobuf_channel, token=self.token)
        return True, None

    def get_clock(self) -> SystemClock:
        return self.system_clock
