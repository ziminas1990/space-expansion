import copy
from typing import Optional, NamedTuple, Tuple
import time
import math
from expansion.types import Vector, TimePoint

import expansion.protocol.Protocol_pb2 as api

class Position(NamedTuple):
    x: float
    y: float
    velocity: Vector
    timestamp: Optional[TimePoint] = None

    @staticmethod
    def build(position: api.Position, timestamp: Optional[int] = None) -> 'Position':
        return Position(x=position.x, y=position.y,
                        velocity=Vector(x=position.vx, y=position.vy),
                        timestamp=TimePoint(timestamp))

    def distance_to(self, other: 'Position') -> float:
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)

    def vector_to(self, other: 'Position') -> Vector:
        return Vector(x=other.x - self.x, y=other.y - self.y)

    def expired(self, timeout_ms: int = 100) -> bool:
        assert self.timestamp is not None
        return self.timestamp.dt_sec() * 1000 > timeout_ms

    def predict(self, at: Optional[int] = None) -> 'Position':
        assert self.timestamp is not None

        dt_sec: float = 0
        if at is None:
            dt_sec = self.timestamp.dt_sec()
        else:
            dt_sec = (at - self.timestamp.now(predict=False)) / 1000000

        return Position(x=self.x + self.velocity.x * dt_sec,
                        y=self.y + self.velocity.y * dt_sec,
                        velocity=Vector(self.velocity.x, self.velocity.y),
                        # Predicted position shouldn't have timestamp
                        timestamp=None)

    def decompose(self, other: "Position") -> Tuple["Position", "Position"]:
        longitudinal_offset, lateral_offset = other.vector_to(self).decompose(other.velocity)
        longitudinal_velocity, lateral_velocity = self.velocity.decompose(other.velocity)
        return Position(x = other.x + longitudinal_offset.x,
                        y = other.y + longitudinal_offset.y,
                        velocity=longitudinal_velocity),\
               Position(x=other.x + lateral_offset.x,
                        y=other.y + lateral_offset.y,
                        velocity=lateral_velocity),


    def more_recent_than(self, other: "Position") -> bool:
        return other.timestamp is None or \
               (self.timestamp and
                self.timestamp.now(predict=False) > other.timestamp.now(predict=False))
