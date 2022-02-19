from typing import Optional, Callable, Awaitable, Tuple, AsyncIterable, TYPE_CHECKING
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

    @BaseModule.use_session(
        terminal_type=ResourceContainerI,
        return_on_unreachable=None,
        return_on_cancel=None)
    async def get_content(
            self,
            timeout: float = 0.5,
            expiration_ms: int = 250,
            session: Optional[ResourceContainerI] = None)\
            -> Optional[Content]:
        assert session is not None
        if self._is_actual(self.cache.content, expiration_ms):
            return self.cache.content[0]

        status, content = await session.get_content(timeout)
        self.cache.content = content, time.monotonic() * 1000
        return self.cache.content[0]

    @BaseModule.use_session(
        terminal_type=ResourceContainerI,
        return_on_unreachable=(Status.FAILED_TO_SEND_REQUEST, 0),
        return_on_cancel=(Status.CANCELED, 0))
    async def open_port(
            self,
            access_key: int,
            timeout: int = 0.5,
            session: Optional[ResourceContainerI] = None)\
            -> (Status, int):
        """Open a new port with the specified 'access_key'. Return operation status
        and port number"""
        assert session is not None
        status, port = await session.open_port(access_key, timeout)
        if status == ResourceContainer.Status.SUCCESS:
            self.opened_port = port, access_key
        return status, port

    @BaseModule.use_session(
        terminal_type=ResourceContainerI,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED)
    async def close_port(
            self,
            timeout: int = 0.5,
            session: Optional[ResourceContainerI] = None) -> Status:
        """Close an existing opened port"""
        assert session is not None
        status = await session.close_port(timeout)
        if status == ResourceContainer.Status.SUCCESS:
            self.opened_port = None
        return status

    @BaseModule.use_session(
        terminal_type=ResourceContainerI,
        exclusive=True,
        return_on_unreachable=Status.FAILED_TO_SEND_REQUEST,
        return_on_cancel=Status.CANCELED)
    async def transfer(
            self,
            port: int,
            access_key: int,
            resource: ResourceItem,
            progress_cb: Optional[Callable[[ResourceItem], None]] = None,
            timeout: int = 0.5,
            session: Optional[ResourceContainerI] = None) -> Status:
        """Transfer the specified 'resource' to the specified 'port' with the
        specified 'access_key'. The optionally specified 'progress_cb' will be
        called to report transferring status (a total amount of transferred
        resources)."""
        assert session is not None

        def _progress_cb(item: ResourceItem):
            # Content must be changed, so we should reset a
            # cached content data
            self.cache.content = None, 0
            if progress_cb is not None:
                progress_cb(item)

        status = await session.transfer(port, access_key, resource, _progress_cb, timeout)
        await session.close()
        return status

    @BaseModule.use_session_for_generators(
        terminal_type=ResourceContainerI,
        return_on_unreachable=(Status.FAILED_TO_SEND_REQUEST, None)
    )
    async def monitor(self,
                      session: Optional[ResourceContainerI] = None) \
            -> AsyncIterable[Tuple[Status, Optional[Content]]]:
        assert session is not None
        status, content = await session.monitor()
        yield status, content
        if not status.is_success():
            return
        while status.is_success() or status.is_timeout():
            status, content = await session.wait_content(timeout=60)
            yield status, content

    @staticmethod
    def _get_by_name(commutator: "Commutator", name: str) -> Optional["ResourceContainer"]:
        return BaseModule._get_by_name(
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

        return await BaseModule._find_best(
            commutator=commutator,
            type=ModuleType.RESOURCE_CONTAINER,
            better_than=better_than)
