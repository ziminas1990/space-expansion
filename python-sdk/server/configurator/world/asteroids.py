from typing import Optional, Dict, List
from server.configurator.world.geomtery import Position, Vector
from server.configurator.expenses import ResourceType


class Asteroid:
    def __init__(self,
                 position: Optional[Position] = None,
                 radius: Optional[float] = None,
                 composition: Optional[Dict[ResourceType, float]] = None):
        self.position: Optional[Position] = position
        self.radius: Optional[float] = radius
        self.composition: Dict[ResourceType, float] = composition if composition else {}

    def set_position(self, position: Position) -> 'Asteroid':
        self.position = position
        return self

    def set_radius(self, radius: float) -> 'Asteroid':
        self.radius = radius
        return self

    def set_composition(self, resource: ResourceType, density: float) -> 'Asteroid':
        self.composition.update({resource: density})
        return self

    def verify(self):
        assert self.position
        assert self.radius and self.radius > 10
        total_density: float = 0
        for density in self.composition.values():
            total_density += density
        assert 0 < total_density <= 100

    def to_pod(self):
        self.verify()
        data = {}
        data.update(self.position.to_pod())
        data.update({"radius": self.radius})
        for type, density in self.composition.items():
            data.update({type.value: density})
        return data


class Asteroids:
    def __init__(self, asteroids: List[Asteroid] = []):
        self.asteroids: List[Asteroid] = asteroids

    def add_asteroid(self, asteroid: Asteroid) -> 'Asteroids':
        self.asteroids.append(asteroid)
        return self

    def verify(self):
        for asteroid in self.asteroids:
            asteroid.verify()

    def to_pod(self):
        self.verify()
        return [asteroid.to_pod() for asteroid in self.asteroids]