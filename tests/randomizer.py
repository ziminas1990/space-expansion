import math
import random
from typing import Optional

import expansion.types as types


class Randomizer:
    def __init__(self, seed: int):
        self.seed = seed

    def _apply_seed(self):
        random.seed(self.seed)
        self.seed = int(random.random() * 0xFFFFFFFF)

    def random_value(self, min, max):
        self._apply_seed()
        return random.uniform(min, max)

    def random_position(
            self,
            *,
            rect: Optional[types.Rect] = None,
            center: Optional[types.Position] = None,
            radius: Optional[float] = None,
            min_speed: float = 0,
            max_speed: float = 0) -> types.Position:
        self._apply_seed()
        # Generate random velocity vector
        speed = random.uniform(min_speed, max_speed)
        velocity = types.Vector(x=random.random(), y=random.random())
        velocity.normalize().set_length(speed)
        # Build random position
        if rect:
            assert center is None and radius is None
            return types.Position(
                x=random.uniform(rect.left, rect.right),
                y=random.uniform(rect.bottom, rect.top),
                velocity=velocity
            )
        elif center:
            assert rect is None
            assert radius is not None
            alfa = random.uniform(0, 2 * math.pi)
            r = math.sqrt(random.uniform(0, radius ** 2))
            return types.Position(
                x=center.x + r * math.cos(alfa),
                y=center.y + r * math.sin(alfa),
                velocity=velocity
            )
