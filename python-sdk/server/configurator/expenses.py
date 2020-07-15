from typing import Dict, Optional, Tuple, NamedTuple, Any
from enum import Enum


class ResourceType(Enum):
    e_METALS = "metals"
    e_SILICATES = "silicates"
    e_ICE = "ice"
    e_LABOR = "labor"


class Expenses:
    def __init__(self):
        self.resources: Dict[ResourceType, float] = {}

    def set(self, type: ResourceType, value: float) -> 'Expenses':
        self.resources.update({type: value})
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
