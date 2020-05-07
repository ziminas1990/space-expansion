import asyncio
import logging
from typing import Callable, Any, Optional

from .channel import Channel


class UdpChannel(Channel, asyncio.BaseProtocol):

    def __init__(self,
                 on_closed_cb: Callable[[], Any],
                 channel_name: str):
        """Create UDP channel. The specified 'on_closed_cb' will be called when
        connection is closed. The specified 'channel_name' will be used in
        logs"""

        self.ip_addr: Optional[str] = None
        # IP address to connect to
        self.port: int = 0
        # UDO port to connect to
        self.transport: asyncio.DatagramTransport = None
        # Object, that returned by asyncio. Will be used to send data
        self.queue: asyncio.Queue = asyncio.Queue()
        # Queue for all received messages
        self.channel_name: str = channel_name
        # Name, that will be used to print logs
        self.on_closed_cb: Callable[[], Any] = on_closed_cb
        # callback, that will be called if connection is closed/lost
        self.logger = logging.getLogger(f"{__name__} '{channel_name}'")

    async def open(self, ip_addr: str, port: int) -> bool:
        """Open connection to the specified 'ip_addr' and 'port'"""
        self.ip_addr = ip_addr
        self.port = port

        loop = asyncio.get_running_loop()
        self.transport, _ = await loop.create_datagram_endpoint(
            protocol_factory=lambda: self,
            remote_addr=(self.ip_addr, self.port),
        )
        return self.transport is not None

    def send(self, message: bytes):
        """Write the specified 'message' to channel"""
        self.logger.debug(f"Sending:\n{message}")
        self.transport.sendto(message)

    async def receive(self, timeout: float = 5) -> Optional[bytes]:
        """Await for the message, but not more than 'timeout' seconds"""
        try:
            return await asyncio.wait_for(self.queue.get(), timeout=timeout)
        except asyncio.TimeoutError:
            return None

    def connection_made(self, transport):
        self.logger.info(f"Connection established to {self.ip_addr}:{self.port}")

    def datagram_received(self, data: bytes, addr):
        self.logger.debug(f"Received {len(data)} bytes from {addr}")
        self.queue.put_nowait(data)

    def error_received(self, exc):
        self.logger.error(f"Got error: {exc}")

    def connection_lost(self, exc):
        self.logger.warning(f"Connection to {self.ip_addr}:{self.port} has been LOST: {exc}")