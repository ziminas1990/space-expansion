from .channel import Channel
from typing import Callable, Awaitable, Any, Optional

from expansion import utils


class ChannelAdapter(Channel):
    SendCall = Callable[[Any], bool]
    CloseCall = Callable[[], Awaitable[bool]]

    def __init__(self,
                 send_fn: SendCall,
                 close_fn: CloseCall,
                 channel_name: Optional[str] = None,
                 *args, **kwargs):
        name: str = channel_name or utils.generate_name(ChannelAdapter)
        super().__init__(channel_name=name, *args, **kwargs)
        self.send_fn: ChannelAdapter.SendCall = send_fn
        self.close_fn: ChannelAdapter.CloseCall = close_fn

    # Overrides from Channel
    def send(self, message: Any) -> bool:
        return self.send_fn(message)

    # Overrides from Channel
    async def close(self):
        await self.close_fn()
