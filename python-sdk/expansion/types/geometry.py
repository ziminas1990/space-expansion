import math


class Vector():

    def __init__(self, x: float, y: float):
        self.x: float = x
        self.y: float = y

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

    def scalar_mult(self, other: 'Vector') -> float:
        return self.x * other.x + self.y * other.y

    def set_length(self, length: float) -> 'Vector':
        k = length / self.abs()
        self.x *= k
        self.y *= k
        return self

    def mult_self(self, k: float) -> 'Vector':
        self.x *= k
        self.y *= k
        return self

    def __repr__(self):
        return f"{{{self.x}, {self.y}}}"
