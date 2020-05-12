from enum import Enum
import protocol.Privileged_pb2 as privileged


class ObjectType(Enum):
    UNKNOWN = 0
    ASTEROID = 1
    SHIP = 2

    def to_protobuf_type(self):
        assert self.value != self.UNKNOWN.value
        d = {
            self.ASTEROID.value: privileged.ObjectType.ASTEROID,
            self.SHIP.value: privileged.ObjectType.SHIP
        }
        return d[self.value]


class PhysicalObject:

    def __init__(self):
        self.id: int = 0
        self.x: float = 0
        self.y: float = 0
        self.r: float = 0
        self.mass: float = 0
        self.vx: float = 0
        self.vy: float = 0

    def from_protobuf(self, object: privileged.Object):
        self.id = object.base_id
        self.x = object.x
        self.y = object.y
        self.r = object.r
        self.m = object.m
        self.vx = object.vx
        self.vy = object.vy
        return self