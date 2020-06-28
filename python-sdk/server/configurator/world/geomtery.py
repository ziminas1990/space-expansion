from typing import Optional


class Vector:
    def __init__(self, x: Optional[float] = None, y: Optional[float] = None):
        self.x: Optional[float] = x
        self.y: Optional[float] = y

    def verify(self):
        assert self.x is not None
        assert self.y is not None

    def to_pod(self):
        self.verify()
        return {
            "x": self.x,
            "y": self.y
        }


class Position:
    def __init__(self):
        self.x: Optional[float] = None
        self.y: Optional[float] = None
        self.velocity: Vector = Vector(x=0, y=0)

    def set_position(self, x: float, y: float, velocity: Vector) -> 'Position':
        self.x = x
        self.y = y
        self.velocity = velocity
        return self

    def verify(self):
        assert self.x
        assert self.y
        assert self.velocity

    def to_pod(self):
        self.verify()
        return {
            "position": {
                "x": self.x,
                "y": self.y
            },
            "velocity": self.velocity.to_pod()
        }