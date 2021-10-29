from typing import List, NamedTuple, TYPE_CHECKING

if TYPE_CHECKING:
    from resources import ResourceItem
    from expansion.protocol import Protocol_pb2 as api


class Property(NamedTuple):
    name: str
    value: str
    nested: List["Property"]

    @staticmethod
    def from_protobuf(data: "api.Property"):
        return Property(
            name=data.name,
            value=data.value,
            nested=list(map(Property.from_protobuf, data.nested))
        )


class Blueprint(NamedTuple):
    name: str
    properties: List[Property]
    expenses: List["ResourceItem"]

    @staticmethod
    def from_protobuf(data: "api.Blueprint"):
        return Blueprint(
            name=data.name,
            properties=list(map(Property.from_protobuf, data.properties)),
            expenses=list(map(ResourceItem.from_protobuf, data.expenses))
        )
