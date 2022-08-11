from enum import Enum

import expansion.api as api


class ObjectType(Enum):
    ASTEROID = "asteroid"
    SHIP = "ship"
    UNKNOWN = "unknown"

    @staticmethod
    def from_protobuf(resource_type: api.types.ObjectType) -> 'ObjectType':
        return {
            api.types.OBJECT_ASTEROID: ObjectType.ASTEROID,
            api.types.OBJECT_SHIP: ObjectType.SHIP,
            api.types.OBJECT_UNKNOWN: ObjectType.UNKNOWN,
        }[resource_type]

    def to_protobuf(self) -> api.types.ObjectType:
        return {
            ObjectType.ASTEROID: api.types.OBJECT_ASTEROID,
            ObjectType.SHIP: api.types.OBJECT_SHIP,
            ObjectType.UNKNOWN: api.types.OBJECT_UNKNOWN,
        }[self]
