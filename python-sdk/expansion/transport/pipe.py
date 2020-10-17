from typing import Optional, Any

from .proxy_channel import ProxyChannel
from expansion.utils import generate_name


class Pipe(ProxyChannel):
    """Pipe is a proxy channel, that does nothing"""

    def __init__(self, name: Optional[str] = None, *args, **kwargs):
        if not name:
            name = generate_name(Pipe)
        super().__init__(proxy_name=name, *args, **kwargs)

    def decode(self, data: Any) -> Optional[Any]:
        return data

    def encode(self, message: Any) -> Optional[Any]:
        return message