from typing import Optional, Dict, List

from .asteroids import Asteroids


class World:
    def __init__(self):
        self.asteroids: Optional[Asteroids] = None

    def set_steroids(self, asteroids: Asteroids) -> 'World':
        self.asteroids = asteroids
        return self

    def verify(self):
        if self.asteroids:
            self.asteroids.verify()

    def to_pod(self):
        data = {}
        if self.asteroids:
            data.update({"Asteroids": self.asteroids.to_pod()})
        return data