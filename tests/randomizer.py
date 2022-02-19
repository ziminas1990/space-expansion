import random
from typing import Optional

import expansion.types as types


class Randomizer:
    def __init__(self, seed: int):
        self.seed = seed

    def _apply_seed(self):
        random.seed(self.seed)
        self.seed = int(random.random() * 0xFFFFFFFF)

    def random_position(
            self,
            *,
            rect: Optional[types.Rect] = None,
            min_speed: float = 0,
            max_speed: float = 0) -> types.Position:
        self._apply_seed()
        # Generate random velocity vector
        speed = random.uniform(min_speed, max_speed)
        velocity = types.Vector(x=random.random(), y=random.random())
        velocity.normalize().set_length(speed)
        # Build random position
        return types.Position(
            x=random.uniform(rect.left, rect.right),
            y=random.uniform(rect.bottom, rect.top),
            velocity=velocity
        )


