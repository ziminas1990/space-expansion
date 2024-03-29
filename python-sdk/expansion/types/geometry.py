import math
from typing import Tuple


class Vector:
    def __init__(self, x: float, y: float):
        self.x: float = x
        self.y: float = y

    def copy(self) -> "Vector":
        return Vector(self.x, self.y)

    def abs(self) -> float:
        return math.sqrt(self.x ** 2 + self.y ** 2)

    def abs_sqr(self) -> float:
        return self.x ** 2 + self.y ** 2

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

    def normalize(self) -> "Vector":
        length = self.abs()
        self.x /= length
        self.y /= length
        return self

    def normalized(self) -> "Vector":
        return Vector(x=self.x, y=self.y).normalize()

    def scalar_mult(self, other: 'Vector') -> float:
        return self.x * other.x + self.y * other.y

    def cosa(self, other: 'Vector') -> float:
        return (self.x * other.x + self.y * other.y) / (self.abs() * other.abs())

    def collinear(self, other: "Vector") -> bool:
        return abs(self.x * other.y - self.y * other.x) < 0.02

    def codirected(self, other: "Vector") -> bool:
        return self.collinear(other) and self.x * other.x >= 0 and self.y * other.y >= 0

    def set_length(self, length: float, *, inplace: bool = True) -> 'Vector':
        k = length / self.abs()
        if inplace:
            self.x *= k
            self.y *= k
            return self
        else:
            return Vector(x=self.x * k, y=self.y * k)

    def zero(self):
        self.x = 0
        self.y = 0
        return self

    def mult_self(self, k: float) -> 'Vector':
        self.x *= k
        self.y *= k
        return self

    def almost_null(self, delta=0.001) -> bool:
        return self.x ** 2 + self.y ** 2 < delta

    @staticmethod
    def almost_equal(left: "Vector", right: "Vector", delta=0.001):
        return (left.x - right.x) ** 2 + (left.y - right.y) ** 2 < delta ** 2

    def decompose(self, other: "Vector") -> Tuple["Vector", "Vector"]:
        """Decompose vector into two vectors: one is parrallel to 'other' and
        another is perpendicular to it."""
        if self.abs() < 0.001:
            return Vector(0, 0), Vector(0, 0)
        cosa = self.cosa(other)
        longitudinal = Vector(other.x, other.y).set_length(self.abs() * cosa)
        lateral = self - longitudinal
        return longitudinal, lateral

    def __repr__(self):
        return f"{{{self.x}, {self.y}}}"


class Rect:
    def __init__(self, left: float, right: float, bottom: float, top: float):
        self.left = left
        self.right = right
        self.bottom = bottom
        self.top = top
