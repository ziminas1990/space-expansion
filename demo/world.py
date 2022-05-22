import logging

from typing import Dict, Iterable
from expansion import types


class World:
    def __init__(self):
        self._logger = logging.getLogger("World")
        self.asteroids: Dict[int, types.PhysicalObject] = {}

    def update_asteroids(self, asteroids: Iterable[types.PhysicalObject]):
        for asteroid in asteroids:
            if asteroid.object_id not in self.asteroids:
                self._logger.info(f"New asteroid detected: r = {asteroid.radius}m")
            cached_asteroid = self.asteroids.setdefault(
                asteroid.object_id,
                types.PhysicalObject(object_id=asteroid.object_id,
                                     object_type=asteroid.object_type,
                                     position=asteroid.position,
                                     radius=asteroid.radius))
            cached_asteroid.update(asteroid)
