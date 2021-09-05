from typing import Optional, Callable, Awaitable, Tuple, TYPE_CHECKING
import time

from expansion.interfaces.rpc import ResourceContainerI
from expansion import utils
from expansion.types import ResourceItem
from .base_module import BaseModule, ModuleType

if TYPE_CHECKING:
    from expansion.modules.base_module import TunnelFactory
    from expansion.modules import Commutator


class ResourceContainer(BaseModule):

    Content = ResourceContainerI.Content
    Status = ResourceContainerI.Status

    class Cache:
        def __init__(self):
            self.content: Tuple[Optional[ResourceContainer.Content], int] = (None, 0)

    def __init__(self,
                 tunnel_factory: "TunnelFactory",
                 name: Optional[str] = None):
        super().__init__(tunnel_factory=tunnel_factory,
                         name=name or utils.generate_name(ResourceContainer))
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
            async with self.rent_session(ResourceContainerI) as channel:
                self.cache.content = await channel.get_content(timeout),\
                                     time.monotonic() * 1000
        return self.cache.content[0]

    async def open_port(self, access_key: int, timeout: int = 0.5) -> (Status, int):
        """Open a new port with the specified 'access_key'. Return operation status
        and port number"""
        async with self.rent_session(ResourceContainerI) as channel:
            status, port = await channel.open_port(access_key, timeout)
            if status == ResourceContainer.Status.SUCCESS:
                self.opened_port = port, access_key
            return status, port

    async def close_port(self, timeout: int = 0.5) -> Status:
        """Close an existing opened port"""
        async with self.rent_session(ResourceContainerI) as channel:
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

        async with self.open_managed_session(ResourceContainerI) as session:
            if not session:
                return ResourceContainerI.Status.FAILED_TO_SEND_REQUEST
            assert isinstance(session, ResourceContainerI)
            status = await session.transfer(port, access_key, resource, _progress_cb, timeout)
            await session.close()
            return status

    @staticmethod
    def get_by_name(commutator: "Commutator", name: str) -> Optional["ResourceContainer"]:
        return BaseModule.get_by_name(
            commutator=commutator,
            type=ModuleType.RESOURCE_CONTAINER,
            name=name
        )

    @staticmethod
    async def find_most_free(commutator: "Commutator") -> Optional["ResourceContainer"]:
        async def better_than(candidate: "ResourceContainer",
                              best: "ResourceContainer"):
            best_content = await best.get_content()
            if not best_content:
                return False
            content = await candidate.get_content()
            if not content:
                return False
            best_free = best_content.volume - best_content.used
            free = content.volume - content.used
            return free > best_free

        return await BaseModule.find_best(
            commutator=commutator,
            type=ModuleType.RESOURCE_CONTAINER,
            better_than=better_than)
