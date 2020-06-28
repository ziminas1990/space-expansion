from typing import Optional
from .geomtery import Position


class PhysicalObject(Position):
    def __init__(self):
        super().__init__()
        self.m: Optional[float] = None
        self.r: Optional[float] = None

    def set_signature(self, m: float, r: float) -> 'PhysicalObject':
        self.m = m
        self.r = r
        return self

    def verify(self):
        super().verify()
        assert self.m and self.m > 0.01
        assert self.r and self.r > 0.01
