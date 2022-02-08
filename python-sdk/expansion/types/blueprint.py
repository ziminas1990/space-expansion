from typing import List, NamedTuple, TYPE_CHECKING

from .resources import ResourceItem

if TYPE_CHECKING:
    from expansion.api import Protocol_pb2 as api


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

    def pretty_print(self, offset: str = "") -> str:
        if self.nested:
            next_level_offset = offset + "\t"
            nested = "\n".join([
                prop.pretty_print(next_level_offset) for prop in self.nested
            ])
            return f'{offset}"{self.name}":\n{nested}'
        else:
            return f'{offset}"{self.name}": {self.value}'


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

    def pretty_print(self) -> str:
        properties = "\n".join([
            prop.pretty_print("\t\t") for prop in self.properties
        ])
        expenses = "\n\t\t".join([
            str(item) for item in self.expenses
        ])
        return f'"{self.name}":\n' \
               f'\t"properties":\n' \
               f'{properties}\n' \
               f'\t"expenses":\n' \
               f'{expenses}'
