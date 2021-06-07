import math
from typing import Tuple


class Vector():

    def __init__(self, x: float, y: float):
        self.x: float = x
        self.y: float = y

    def copy(self) -> "Vector":
        return Vector(self.x, self.y)

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

    def __pow__(self, power: int, modulo=None) -> "Vector":
        assert power >= 1
        k = self.abs() ** (power - 1)
        return Vector(x = self.x * k, y=self.y * k)

    def normalized(self) -> "Vector":
        len = self.abs()
        return Vector(x = self.x / len, y = self.y / len)

    def scalar_mult(self, other: 'Vector') -> float:
        return self.x * other.x + self.y * other.y

    def cosa(self, other: 'Vector') -> float:
        return (self.x * other.x + self.y * other.y) / (self.abs() * other.abs())

    def set_length(self, length: float, *, inplace: bool = True) -> 'Vector':
        k = length / self.abs()
        if inplace:
            self.x *= k
            self.y *= k
            return self
        else:
            return Vector(x = self.x * k, y = self.y * k)

    def mult_self(self, k: float) -> 'Vector':
        self.x *= k
        self.y *= k
        return self

    def decompose(self, other: "Vector") -> Tuple["Vector", "Vector"]:
        """Decompose vector into two vectors: one is parralel to 'other' and
        another is perpendicular to it."""
        cosa = self.cosa(other)
        longitudinal = Vector(other.x, other.y).set_length(self.abs() * cosa)
        lateral = self - longitudinal
        return longitudinal, lateral

    def __repr__(self):
        return f"{{{self.x}, {self.y}}}"
