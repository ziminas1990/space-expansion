from enum import Enum

import expansion.protocol.Protocol_pb2 as rpc


class ObjectType(Enum):
    ASTEROID = "asteroid"
    SHIP = "ship"
    UNKNOWN = "unknown"

    @staticmethod
    def from_protobuf(resource_type: rpc.ResourceType) -> 'ObjectType':
        return {
            rpc.OBJECT_ASTEROID: ObjectType.ASTEROID,
            rpc.OBJECT_SHIP: ObjectType.SHIP,
            rpc.OBJECT_UNKNOWN: ObjectType.UNKNOWN,
        }[resource_type]
