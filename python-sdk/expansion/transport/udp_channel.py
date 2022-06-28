import asyncio
import socket
from typing import Callable, Any, Optional, Tuple

from expansion import utils
from .channel import Channel


class UdpChannel(Channel, asyncio.BaseProtocol):

    def __init__(self,
                 on_closed_cb: Callable[[], Any],
                 channel_name: Optional[str] = None,
                 trace_mode: bool = False,
                 *args, **kwargs):
        """Create UDP channel. The specified 'on_closed_cb' will be called when
        connection is closed. The specified 'channel_name' will be used in
        logs"""
        super(UdpChannel, self).__init__(
            channel_name=channel_name or utils.generate_name(type(self)),
            trace_mode=trace_mode,
            *args, **kwargs)

        self.remote: Optional[Tuple[str, int]] = None
        # Pair, that holds IP and port of the server
        self.local_addr: Optional[Tuple[str, int]] = None
        # Pair, that holds socket's local IP and port
        self.transport: Optional[asyncio.DatagramTransport] = None
        # Object, that returned by asyncio. Will be used to send data
        self.on_closed_cb: Callable[[], Any] = on_closed_cb
        # callback, that will be called if connection is closed/lost

        self.__trace_mode = trace_mode

    def set_trace_mode(self, on: bool):
        self.__trace_mode = on

    async def open(self) -> bool:
        loop = asyncio.get_running_loop()
        self.transport, _ = await loop.create_datagram_endpoint(
            protocol_factory=lambda: self,
            local_addr=("0.0.0.0", 0),  # Let system to use chose a port
            family=socket.AF_INET
        )
        self.local_addr = self.transport.get_extra_info("sockname")
        local_port = self.local_addr[1]
        if self.channel_name == utils.generate_name(type(self)):
            self.channel_name = f"UDP.{local_port}"
        return self.transport is not None

    def set_remote(self, ip: str, port: int):
        self.remote = (ip, port)

    def get_local_address(self) -> Optional[Tuple[str, int]]:
        """Return IP and port of the local socket or None"""
        if not self.transport:
            return None
        if not self.local_addr:
            self.local_addr = self.transport.get_extra_info("sockname")
        return self.local_addr

    # Override from Channel
    def send(self, message: bytes) -> bool:
        """Write the specified 'message' to channel"""
        assert self.remote is not None
        if self.__trace_mode:
            self.channel_logger.debug(f"Sending:\n{len(message)} bytes to {self.remote}")
        self.transport.sendto(message, addr=self.remote)
        return True

    # Override from Channel
    async def close(self):
        # UDP channel don't need to be closed
        self.channel_logger.info("Closed")

    def connection_made(self, transport):
        self.channel_logger.info(f"UDP socket created")

    def datagram_received(self, data: bytes, addr):
        if self.__trace_mode:
            self.channel_logger.debug(f"Received {len(data)} bytes from {addr}")
        if self.terminal:
            self.terminal.on_receive(data, None)
        else:
            self.channel_logger.warning(
                f"Ignoring {len(data)} bytes message: not attached to the terminal!")

    def error_received(self, exc):
        self.channel_logger.error(f"Got error: {exc}")

    def connection_lost(self, exc):
        self.channel_logger.warning(f"Connection to {self.remote} has been LOST: {exc}")
