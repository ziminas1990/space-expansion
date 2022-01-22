from typing import Optional
import expansion.types as types


class PhysicalObject:
    def __init__(self,
                 object_type: types.ObjectType = types.ObjectType.UNKNOWN,
                 object_id: Optional[int] = None,
                 position: Optional[types.Position] = None,
                 radius: Optional[float] = None):
        self.object_type: types.ObjectType = object_type
        self.object_id = object_id
        self.position: Optional[types.Position] = position
        self.radius: Optional[float] = radius

    def update(self, other: "PhysicalObject"):
        assert self.object_type == other.object_type and \
               self.object_id == other.object_id
        if self.position is None or other.position.more_recent_than(self.position):
            self.position = other.position
