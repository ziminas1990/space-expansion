from typing import Dict, Optional
from expansion.types import ResourceType


class ResourcesList:
    def __init__(self, resources: Optional[Dict[ResourceType, float]] = None):
        self.resources: Dict[ResourceType, float] = resources or {}

    def set(self, resource_type: ResourceType, value: float) -> 'ResourcesList':
        self.resources.update({resource_type: value})
        return self

    def verify(self):
        pass

    def __iadd__(self, other: "ResourcesList") -> "ResourcesList":
        for resource_type, amount in other.resources.items():
            total = self.resources.setdefault(resource_type, 0) + amount
            self.resources.update({resource_type: total})
        return self

    def __mul__(self, k: float) -> "ResourcesList":
        return ResourcesList(
            resources={
                resource_type: amount * k
                for resource_type, amount in self.resources.items()
            }
        )

    def contains(self, other: "ResourcesList") -> bool:
        for resource_type, amount in other.resources.items():
            if resource_type == ResourceType.e_LABOR:
                continue
            try:
                if self.resources[resource_type] < amount:
                    return False
            except KeyError:
                return False
        return True

    def to_pod(self):
        self.verify()
        data = {}
        for resource, value in self.resources.items():
            if value > 0:
                data.update({resource.value: value})
        return data


class PhysicalResources(ResourcesList):
    def __init__(self, resources: Optional[Dict[ResourceType, float]] = None):
        super().__init__(resources=resources)

    def verify(self):
        super().verify()
        assert ResourceType.e_LABOR not in self.resources
