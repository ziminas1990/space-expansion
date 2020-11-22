from typing import Optional, Callable, Awaitable, Tuple
import time

from expansion.interfaces.public import ResourceContainerI
from expansion import utils
from expansion.types import ResourceItem
from .base_module import BaseModule


class ResourceContainer(BaseModule):

    Content = ResourceContainerI.Content
    Status = ResourceContainerI.Status

    class Cache:
        def __init__(self):
            self.content: Tuple[Optional[ResourceContainer.Content], int] = (None, 0)

    def __init__(self,
                 connection_factory: Callable[[], Awaitable[ResourceContainerI]],
                 name: Optional[str] = None):
        super().__init__(connection_factory=connection_factory,
                         logging_name=name or utils.generate_name(ResourceContainer))
        self.cache = ResourceContainer.Cache()
        self.opened_port: Optional[Tuple[int, int]] = None  # (port, accessKey)

    def get_opened_port(self) -> Optional[Tuple[int, int]]:
        """Return (port, accessKey) pair, if port is opened. Otherwise
        return None"""
        return self.opened_port

    async def get_content(self,
                          timeout: float = 0.5,
                          expiration_ms: int = 250) -> Optional[Content]:
        if not self._is_actual(self.cache.content, expiration_ms):
            async with self._lock_channel() as channel:
                assert isinstance(channel, ResourceContainerI)  # For type hinting
                self.cache.content = await channel.get_content(timeout),\
                                     time.monotonic() * 1000
        return self.cache.content[0]

    async def open_port(self, access_key: int, timeout: int = 0.5) -> (Status, int):
        """Open a new port with the specified 'access_key'. Return operation status
        and port number"""
        async with self._lock_channel() as channel:
            assert isinstance(channel, ResourceContainerI)  # For type hinting
            status, port = await channel.open_port(access_key, timeout)
            if status == ResourceContainer.Status.SUCCESS:
                self.opened_port = port, access_key
            return status, port

    async def close_port(self, timeout: int = 0.5) -> Status:
        """Close an existing opened port"""
        async with self._lock_channel() as channel:
            assert isinstance(channel, ResourceContainerI)  # For type hinting
            status = await channel.close_port(timeout)
            if status == ResourceContainer.Status.SUCCESS:
                self.opened_port = None
            return status

    async def transfer(self, port: int, access_key: int,
                       resource: ResourceItem,
                       progress_cb: Optional[Callable[[ResourceItem], None]] = None,
                       timeout: int = 0.5) -> Status:
        """Transfer the specified 'resource' to the specified 'port' with the
        specified 'access_key'. The optionally specified 'progress_cb' will be
        called to report transferring status (a total amount of transferred
        resources)."""
        def _progress_cb(item: ResourceItem):
            # Content must be changed, so we should reset a
            # cached content data
            self.cache.content = None, 0
            if progress_cb is not None:
                progress_cb(item)

        async with self._lock_channel() as channel:
            assert isinstance(channel, ResourceContainerI)  # For type hinting
            return await channel.transfer(port, access_key, resource, _progress_cb, timeout)
