import asyncio
import logging
import socket
from typing import Callable, Any, Optional, Tuple

from .channel import Channel, ChannelMode


class UdpChannel(Channel, asyncio.BaseProtocol):

    def __init__(self,
                 on_closed_cb: Callable[[], Any],
                 channel_name: Optional[str] = None):
        """Create UDP channel. The specified 'on_closed_cb' will be called when
        connection is closed. The specified 'channel_name' will be used in
        logs"""
        super().__init__(mode=ChannelMode.PASSIVE)

        self.remote: Optional[Tuple[str, int]] = None
        # Pair, that holds IP and port of the server
        self.local_addr: Optional[Tuple[str, int]] = None
        # Pair, that holds socket's local IP and port
        self.transport: Optional[asyncio.DatagramTransport] = None
        # Object, that returned by asyncio. Will be used to send data
        self.on_closed_cb: Callable[[], Any] = on_closed_cb
        # callback, that will be called if connection is closed/lost
        self.logger = logging.getLogger(f"{__name__} '{channel_name}'") \
            if channel_name else None

    async def bind(self, local_addr: Tuple[str, int]) -> bool:
        self.local_addr = local_addr

        loop = asyncio.get_running_loop()
        self.transport, _ = await loop.create_datagram_endpoint(
            protocol_factory=lambda: self,
            local_addr=self.local_addr,
            family=socket.AF_INET
        )
        return self.transport is not None

    async def open(self, remote_ip: str, remote_port: int) -> bool:
        self.remote = (remote_ip, remote_port)

        loop = asyncio.get_running_loop()
        self.transport, _ = await loop.create_datagram_endpoint(
            protocol_factory=lambda: self,
            remote_addr=self.remote,
        )
        return self.transport is not None

    def set_remote(self, ip: str, port: int):
        assert self.remote is None, f"Remote is already set to {self.remote}"
        self.remote=(ip, port)

    def get_local_address(self) -> Optional[Tuple[str, int]]:
        """Return IP and port of the local socket or None"""
        if not self.transport:
            return None
        if not self.local_addr:
            self.local_addr = self.transport.get_extra_info("peername")
        return self.local_addr

    # Override from Channel
    def send(self, message: bytes) -> bool:
        """Write the specified 'message' to channel"""
        self.logger.debug(f"Sending {len(message)} bytes to {self.remote}")
        self.transport.sendto(message, addr=self.remote)
        return True

    # Override from Channel
    async def close(self):
        # UDP channel don't need to be closed
        pass

    def connection_made(self, transport):
        self.logger.info(f"Connection established to {self.remote}")

    def datagram_received(self, data: bytes, addr):
        self.logger.debug(f"Received {len(data)} bytes from {addr}")
        if self.is_active_mode():
            if self.terminal:
                self.terminal.on_receive(data)
            else:
                self.logger.warning(f"Ignoring {len(data)} bytes message:"
                                    f" not attached to terminal!")
        else:
            self.on_message(message=data)

    def error_received(self, exc):
        self.logger.error(f"Got error: {exc}")

    def connection_lost(self, exc):
        self.logger.warning(f"Connection to {self.remote} has been LOST: {exc}")