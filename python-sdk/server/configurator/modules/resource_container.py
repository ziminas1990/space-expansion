from typing import Optional, Dict
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType
from .base_module import BaseModule
from server.configurator.resources import PhysicalResources
from expansion.types import ResourceType


class ResourceContainerBlueprint(BaseBlueprint):
    def __init__(self, name: str, volume: Optional[int] = None):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_RESOURCE_CONTAINER, name))
        self.volume: Optional[int] = volume

    def verify(self):
        super().verify()
        assert self.volume is not None
        assert self.volume > 0

    def to_pod(self):
        self.verify()
        data = super(ResourceContainerBlueprint, self).to_pod()
        data.update({
            'volume': self.volume
        })
        return data


class ResourceContainer(BaseModule):
    def __init__(self, content: Optional[Dict[ResourceType, float]] = None):
        super(ResourceContainer, self).__init__()
        self.content = PhysicalResources(content)

    def verify(self):
        super(ResourceContainer, self).verify()
        self.content.verify()

    def to_pod(self):
        self.verify()
        return self.content.to_pod()