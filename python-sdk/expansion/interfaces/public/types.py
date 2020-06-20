from typing import NamedTuple
import math


class Vector(NamedTuple):
    x: float
    y: float

    def abs(self) -> float:
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def __neg__(self) -> 'Vector':
        return Vector(x=-self.x, y=-self.y)

    def __add__(self, other: 'Vector') -> 'Vector':
        return Vector(x=self.x + other.x, y=self.y + other.y)

    def __sub__(self, other: 'Vector') -> 'Vector':
        return Vector(x=self.x - other.x, y=self.y - other.y)

    def __truediv__(self, divider: float) -> 'Vector':
        return Vector(x=self.x / divider, y=self.y / divider)

    def __mul__(self, mult: float) -> 'Vector':
        return Vector(x=self.x * mult, y=self.y * mult)

    def set_length(self, length: float) -> 'Vector':
        k = length / self.abs()
        self.x *= k
        self.y *= k
        return self


class Position(NamedTuple):
    x: float
    y: float
    velocity: Vector

    def distance_to(self, other: 'Position') -> float:
        return math.sqrt((self.x - other.x)**2 + (self.y - other.y)**2)

    def vector_to(self, other: 'Position') -> Vector:
        return Vector(x=other.x - self.x, y=other.y - self.y)
