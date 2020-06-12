from typing import Optional, Any
import asyncio

from expansion.protocol.Protocol_pb2 import Message
from expansion.transport.channel import Channel, ChannelMode
from expansion.transport.terminal import Terminal


class BaseModule(Terminal):
    next_module_instance_id: int = 0

    def __init__(self, module_name: str = "BaseModule", *args, **kwargs):
        """
        Create new BaseModule instance.
        All channels, that will be attached to this module, will be switched into
        the specified 'preferable_channel_mode' mode.
        """
        self.module_instance_id = BaseModule.next_module_instance_id
        BaseModule.next_module_instance_id += 1
        self.module_name = f"{module_name}_{self.module_instance_id}"

        super().__init__(terminal_name=f"{self.module_name}::terminal")
        self.queue: asyncio.Queue = asyncio.Queue()
        self.channel: Optional[Channel] = None

    # Override from Terminal
    def on_receive(self, message: Any):
        super().on_receive(message)  # For logging
        self.queue.put_nowait(message)

    # Override from Terminal
    def attach_channel(self, channel: Channel):
        super().attach_channel(channel)  # For logging
        self.channel = channel

    # Override from Terminal
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        super().on_channel_mode_changed(mode)  # For logging

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # For logging
        self.channel = None

    async def wait_message(self, timeout: float = 1.0) -> Optional[Any]:
        # Should be called only if channel is attached to module and passes
        # all received message. Otherwise this call will always fail by
        # timeout
        try:
            return await asyncio.wait_for(self.queue.get(), timeout=timeout)
        except asyncio.TimeoutError:
            return None

    def send_message(self, message: Message) -> bool:
        return self.channel is not None and self.channel.send(message=message)