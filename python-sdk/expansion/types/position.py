from typing import Optional, NamedTuple
import time
import math
from expansion.types import Vector

class Position(NamedTuple):
    x: float
    y: float
    velocity: Vector
    timestamp: Optional[int] = None

    def distance_to(self, other: 'Position') -> float:
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)

    def vector_to(self, other: 'Position') -> Vector:
        return Vector(x=other.x - self.x, y=other.y - self.y)

    def at(self, now_us: Optional[int] = None) -> 'Position':
        if now_us is None:
            return Position(x=self.x,
                            y=self.y,
                            velocity=Vector(self.velocity.x, self.velocity.y))
        assert self.timestamp is not None
        dt_sec: float = (now_us - self.timestamp) / 1000000
        return Position(x=self.x + self.velocity.x * dt_sec,
                        y=self.y + self.velocity.y * dt_sec,
                        velocity=Vector(self.velocity.x, self.velocity.y),
                        timestamp=self.timestamp + int(dt_sec * 1000000))


class CachedPosition:
    def __init__(self, position: Position):
        self.data: Position = position
        self._update_time = time.time()

    def expired(self, timeout_ms: int = 100) -> bool:
        return (time.time() - self._update_time) * 1000 > timeout_ms

    def update(self, position: Position):
        self.data: Position = position
        self._update_time = time.time()

    def predict(self, now_us: Optional[int]) -> Position:
        if now_us is None:
            dt_us = 1000000 * (time.time() - self._update_time)
            now_us = self.data.timestamp + dt_us
        return self.data.at(now_us)
