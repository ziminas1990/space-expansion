from typing import Optional, Tuple

import expansion.api as api
from expansion.transport import IOTerminal
import expansion.types as types


class Spawner(IOTerminal):

    def __init__(self, name: str, *args, **kwargs):
        super(Spawner, self).__init__(name=name, *args, **kwargs)

    async def spawn_asteroid(
            self,
            position: types.Position,
            composition: types.ResourcesDict,
            radius: float) \
            -> Tuple[Optional[int], Optional[int]]:
        """Send request to spawn a new asteroid at the specified 'position'
        and with the specified 'composition' and 'radius'. On success return
        created asteroid id and creation timestamp"""
        request = api.admin.Message()
        body = request.spawn.asteroid
        position.to_protobuf(body.position)
        body.radius = radius
        for item in composition.values():
            item.to_protobuf(body.composition.items.add())
        if not self.send(request):
            return None, None
        return await self.wait_exact(["spawn", "asteroid_id"])

    @staticmethod
    def _resources_to_list(items: types.ResourcesDict,
                           resources: api.types.Resources) -> api.types.Resources:
        for item in items.values():
            item.to_protobuf(resources.items.add())
