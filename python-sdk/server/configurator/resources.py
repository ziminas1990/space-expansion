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
