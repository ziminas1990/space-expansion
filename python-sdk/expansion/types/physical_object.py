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

    def update(self, other: "PhysicalObject"):
        assert self.object_id == other.object_id
        if self.position is None:
            self.position = other.position
        elif other.position is None or other.position.timestamp is None:
            return
        elif self.position.timestamp is None or  \
            self.position.timestamp < other.position.timestamp:
            self.position = other.position
