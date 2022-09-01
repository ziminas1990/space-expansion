import logging

from typing import Dict, Iterable
from expansion import types


class World:
    def __init__(self):
        self._logger = logging.getLogger("World")
        self.asteroids: Dict[int, types.PhysicalObject] = {}

    def update_object(self, obj: types.PhysicalObject):
        if obj.object_type == types.ObjectType.ASTEROID:
            asteroid = obj
            cached_asteroid = self.asteroids.setdefault(
                asteroid.object_id,
                types.PhysicalObject(object_id=asteroid.object_id,
                                     object_type=asteroid.object_type,
                                     position=asteroid.position,
                                     radius=asteroid.radius))
            cached_asteroid.update(asteroid)

    def update_objects(self, objects: Iterable[types.PhysicalObject]):
        for obj in objects:
            self.update_object(obj)
