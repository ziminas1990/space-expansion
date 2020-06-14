from .channel import Channel, ChannelMode
from typing import Callable, Awaitable, Any, Optional


class ChannelAdapter(Channel):
    SendCall = Callable[[Any], bool]
    CloseCall = Callable[[], Awaitable[bool]]
    ReceiveCall = Optional[Callable[[float], Awaitable[Any]]]

    def __init__(self,
                 send_fn: SendCall,
                 close_fn: CloseCall,
                 receive_fn: ReceiveCall,
                 channel_name: str = "ChannelAdapter",
                 mode: ChannelMode = ChannelMode.PASSIVE,
                 *args, **kwargs):
        super().__init__(mode=mode, channel_name=channel_name, *args, **kwargs)
        self.send_fn: ChannelAdapter.SendCall = send_fn
        self.close_fn: ChannelAdapter.CloseCall = close_fn
        self.receive_fn: ChannelAdapter.ReceiveCall = receive_fn

    # Overrides from Channel
    def send(self, message: Any) -> bool:
        return self.send_fn(message)

    # Overrides from Channel
    async def close(self):
        await self.close_fn()

    # Overrides from Channel
    async def receive(self, timeout: float = 5) -> Any:
        if self.receive_fn:
            return await self.receive_fn(timeout)
        else:
            return await super().receive(timeout)
