from enum import Enum
from typing import Dict

import expansion.protocol.Protocol_pb2 as protocol


class ResourceType(Enum):
    e_METALS = "metals"
    e_SILICATES = "silicates"
    e_ICE = "ice"
    e_LABOR = "labor"

    @staticmethod
    def convert(resource_type: protocol.ResourceType) -> 'ResourceType':
        return {
            protocol.RESOURCE_ICE: ResourceType.e_ICE,
            protocol.RESOURCE_SILICATES: ResourceType.e_SILICATES,
            protocol.RESOURCE_METALS: ResourceType.e_METALS,
            protocol.RESOURCE_LABOR: ResourceType.e_LABOR
        }[resource_type]

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
        volume: float = 0.0
        for resource_type, amount in resources.items():
            density = ResourceType.density(resource_type)
            if density > 0:
                volume += amount / density
        return volume
