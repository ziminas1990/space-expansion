from typing import Optional, NamedTuple, Dict

import expansion.protocol.Protocol_pb2 as public
from expansion.protocol.utils import get_message_field
from expansion.transport.queued_terminal import QueuedTerminal
from expansion.types import ResourceType

import expansion.utils as utils


class ResourceContainer(QueuedTerminal):

    class Content(NamedTuple):
        volume: int
        used: float
        resources: Dict[ResourceType, float]

    class Cache:
        """Cache is used to store data and may be used by user to obtain
        data without requesting a server"""
        def __init__(self):
            self.content: Optional["ResourceContainer.Content"] = None

        def update_content(self, content: public.IResourceContainer.Content):
            self.content = ResourceContainer.Content(
                volume=content.volume,
                used=content.used,
                resources={ResourceType.convert(item.type): item.amount
                           for item in content.resources}
            )

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(ResourceContainer))
        self._cache: ResourceContainer.Cache = ResourceContainer.Cache()

    def cache(self) -> 'ResourceContainer.Cache':
        return self._cache

    async def get_content(self, timeout: float = 0.5) -> Optional[Content]:
        request = public.Message()
        request.resource_container.content_req = True
        if not self.send_message(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        content = get_message_field(response, "resource_container.content")
        if not content:
            return None
        self._cache.update_content(content)
        return self._cache.content
