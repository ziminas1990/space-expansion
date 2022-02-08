
from typing import Optional, NamedTuple, Tuple, Union
import math
from expansion.types import Vector, TimePoint

import expansion.api as api


class Position(NamedTuple):
    x: float
    y: float
    velocity: Vector
    timestamp: Optional[TimePoint] = None

    @staticmethod
    def build(position: api.types.Position, timestamp: Optional[int] = None) -> 'Position':
        return Position(x=position.x, y=position.y,
                        velocity=Vector(x=position.vx, y=position.vy),
                        timestamp=TimePoint(timestamp))

    def distance_to(self, other: Union['Position', Tuple[float, float]]) -> float:
        if isinstance(other, Position):
            return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)
        else:
            return math.sqrt((self.x - other[0]) ** 2 + (self.y - other[1]) ** 2)

    def vector_to(self, other: Union['Position', Tuple[float, float]]) -> Vector:
        if isinstance(other, Position):
            return Vector(x=other.x - self.x, y=other.y - self.y)
        else:
            return Vector(x=other[0] - self.x, y=other[1] - self.y)

    def expired(self, timeout_ms: int = 100) -> bool:
        assert self.timestamp is not None
        return self.timestamp.dt_sec() * 1000 > timeout_ms

    def predict(self, at: Optional[int] = None) -> 'Position':
        assert self.timestamp is not None

        if at is None:
            at = self.timestamp.predict_usec()

        dt_sec = (at - self.timestamp.usec()) / 10 ** 6

        return Position(x=self.x + self.velocity.x * dt_sec,
                        y=self.y + self.velocity.y * dt_sec,
                        velocity=Vector(self.velocity.x, self.velocity.y),
                        # Since it is predicted position, it should have static
                        # timestamp
                        timestamp=TimePoint(at, static=True))

    def decompose(self, other: "Position") -> Tuple["Position", "Position"]:
        base_vector = other.velocity if other.velocity.abs() > 0.001 else self.velocity
        longitudinal_offset, lateral_offset = other.vector_to(self).decompose(base_vector)
        longitudinal_velocity, lateral_velocity = self.velocity.decompose(base_vector)
        return Position(x = other.x + longitudinal_offset.x,
                        y = other.y + longitudinal_offset.y,
                        velocity=longitudinal_velocity,
                        timestamp=self.timestamp),\
               Position(x=other.x + lateral_offset.x,
                        y=other.y + lateral_offset.y,
                        velocity=lateral_velocity,
                        timestamp=self.timestamp)

    def more_recent_than(self, other: "Position") -> bool:
        return other.timestamp is None or (
                self.timestamp and self.timestamp.more_recent(other.timestamp))

    def no_timestamp(self) -> "Position":
        return Position(self.x, self.y, self.velocity, None)

    def with_timestamp(self, timestamp: int) -> "Position":
        return Position(self.x, self.y, self.velocity, timestamp=TimePoint(timestamp, static=True))
