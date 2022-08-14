from typing import Optional, Tuple
from enum import Enum

import expansion.api as api
from expansion.transport import IOTerminal
import expansion.types as types


class Spawner(IOTerminal):

    class Status(Enum):
        SUCCESS = 0
        PLAYER_DOESNT_EXIST = "player doesn't exist"
        BLUEPRINT_DOESNT_EXIST = "blueprint doesn't exist"
        NOT_A_SHIP_BLUEPRINT = "not a ship blueprint"
        CANT_SPAWN_SHIP = "can't spawn ship"
        # Internal SDK statuses:
        FAILED_TO_SEND_REQUEST = "failed to send request"
        RESPONSE_TIMEOUT = "response timeout"
        UNEXPECTED_RESPONSE = "unexpected response"
        CHANNEL_CLOSED = "channel closed"
        CANCELED = "operation canceled"
        UNKNOWN = "unknown"

        def is_success(self):
            return self == Spawner.Status.SUCCESS

        @staticmethod
        def from_protobuf(mode: api.admin.Spawn.Status):
            try:
                return {
                    api.admin.Spawn.Status.SUCCESS:
                        Spawner.Status.SUCCESS,
                    api.admin.Spawn.Status.PLAYER_DOESNT_EXIST:
                        Spawner.Status.PLAYER_DOESNT_EXIST,
                    api.admin.Spawn.Status.BLUEPRINT_DOESNT_EXIST:
                        Spawner.Status.BLUEPRINT_DOESNT_EXIST,
                    api.admin.Spawn.Status.NOT_A_SHIP_BLUEPRINT:
                        Spawner.Status.NOT_A_SHIP_BLUEPRINT,
                    api.admin.Spawn.Status.CANT_SPAWN_SHIP:
                        Spawner.Status.CANT_SPAWN_SHIP,
                }[mode]
            except KeyError:
                return Spawner.Status.UNKNOWN

    def __init__(self, name: str, *args, **kwargs):
        super(Spawner, self).__init__(name=name, *args, **kwargs)

    async def spawn_asteroid(
            self,
            position: types.Position,
            composition: types.ResourcesDict,
            radius: float) \
            -> Tuple[Status, Optional[types.PhysicalObject]]:
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
            return Spawner.Status.FAILED_TO_SEND_REQUEST, None

        asteroid_id, timestamp = await self.wait_exact(["spawn", "asteroid_id"])
        if asteroid_id is None:
            return Spawner.Status.UNEXPECTED_RESPONSE, None

        return Spawner.Status.SUCCESS,\
               types.PhysicalObject(
                   object_type=types.ObjectType.ASTEROID,
                    object_id=asteroid_id,
                    position=position.with_timestamp(timestamp),
                    radius=radius)

    async def spawn_ship(self, player: str,
                         blueprint: str,
                         ship_name: str,
                         position: types.Position) \
            -> Tuple[Status, Optional[types.PhysicalObject]]:

        request = api.admin.Message()
        body = request.spawn.ship
        body.player = player
        body.blueprint = blueprint
        body.ship_name = ship_name
        position.to_protobuf(body.position)
        if not self.send(request):
            return Spawner.Status.FAILED_TO_SEND_REQUEST, None

        response, timestamp = await self.wait_exact(["spawn"])
        if response is None:
            return Spawner.Status.RESPONSE_TIMEOUT, None

        ship_id = api.get_message_field(response, ["ship_id"])
        if ship_id is not None:
            return Spawner.Status.SUCCESS, \
                   types.PhysicalObject(
                       object_type=types.ObjectType.SHIP,
                       object_id=ship_id,
                       position=position.with_timestamp(timestamp))
        problem = api.get_message_field(response, ["problem"])
        if problem:
            return Spawner.Status.from_protobuf(problem), None

        return Spawner.Status.UNEXPECTED_RESPONSE, None

    @staticmethod
    def _resources_to_list(items: types.ResourcesDict,
                           resources: api.types.Resources) -> api.types.Resources:
        for item in items.values():
            item.to_protobuf(resources.items.add())
