from enum import Enum
from typing import Dict, List, NamedTuple

import expansion.protocol.Protocol_pb2 as protocol


class ResourceType(Enum):
    e_METALS = "metals"
    e_SILICATES = "silicates"
    e_ICE = "ice"
    e_STONES = "stones"
    e_LABOR = "labor"

    @staticmethod
    def from_protobuf(resource_type: protocol.ResourceType) -> 'ResourceType':
        return {
            protocol.RESOURCE_ICE: ResourceType.e_ICE,
            protocol.RESOURCE_SILICATES: ResourceType.e_SILICATES,
            protocol.RESOURCE_METALS: ResourceType.e_METALS,
            protocol.RESOURCE_LABOR: ResourceType.e_LABOR
        }[resource_type]

    def to_protobuf(self) -> protocol.ResourceType:
        """Convert this type to the protobuf equivalent"""
        return {
            ResourceType.e_ICE: protocol.RESOURCE_ICE,
            ResourceType.e_SILICATES: protocol.RESOURCE_SILICATES,
            ResourceType.e_METALS: protocol.RESOURCE_METALS,
            ResourceType.e_LABOR: protocol.RESOURCE_LABOR
        }[self]

    @staticmethod
    def density(resource: "ResourceType") -> float:
        return {
            ResourceType.e_METALS: 4500.0,
            ResourceType.e_SILICATES: 2330.0,
            ResourceType.e_ICE: 916.0,
            ResourceType.e_LABOR: 0.0,
        }[resource]

    @staticmethod
    def estimate_volume(resources: Dict["ResourceType", float]) -> float:
        """Calculate a volume, that is required to store the specified 'resources'"""
        volume: float = 0.0
        for resource_type, amount in resources.items():
            density = ResourceType.density(resource_type)
            if density > 0:
                volume += amount / density
        return volume

    @staticmethod
    def calculate_amount(resource_type: 'ResourceType', volume: float) -> float:
        """Calculate amount of resource with the specified 'resource_type',
        fitted into the specified 'volume'"""
        return volume * ResourceType.density(resource_type)

class ResourceItem(NamedTuple):
    """Represent a single resource item"""
    resource_type: ResourceType
    amount: float

    @staticmethod
    def from_protobuf(item: protocol.ResourceItem) -> 'ResourceItem':
        return ResourceItem(
            resource_type=ResourceType.from_protobuf(item.type),
            amount=item.amount
        )

    def to_protobuf(self, output):
        """Convert this type to the protobuf equivalent"""
        output.type = self.resource_type.to_protobuf()
        output.amount = self.amount

    def __str__(self):
        return f"{self.resource_type.value}: {self.amount}"

ResourcesDict = Dict[ResourceType, ResourceItem]
