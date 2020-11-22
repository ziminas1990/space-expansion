from typing import Optional
import expansion.types as types


class PhysicalObject:
    def __init__(self,
                 object_id: Optional[int] = None,
                 position: Optional[types.Position] = None,
                 radius: Optional[float] = None):
        self.object_id = object_id
        self.position: Optional[types.Position] = position
        self.radius: Optional[float] = radius
