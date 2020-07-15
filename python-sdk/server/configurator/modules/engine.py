from typing import Optional
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType
from .base_module import BaseModule
from server.configurator.world.geomtery import Vector


class EngineBlueprint(BaseBlueprint):
    def __init__(self, name: str):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_ENGINE, name))
        self.max_thrust: Optional[int] = None

    def set_max_thrust(self, max_thrust: int) -> 'EngineBlueprint':
        self.max_thrust = max_thrust
        return self

    def verify(self):
        super().verify()
        assert self.max_thrust is not None
        assert self.max_thrust > 0

    def to_pod(self):
        self.verify()
        data = super(EngineBlueprint, self).to_pod()
        data.update({
            'max_thrust': self.max_thrust
        })
        return data


class Engine(BaseModule):
    def __init__(self):
        super(Engine, self).__init__()
        self.thrust: Vector = Vector(x=0, y=0)

    def set_thrust(self, thrust: Vector) -> 'Engine':
        self.thrust = thrust
        return self
        
    def verify(self):
        super(Engine, self).verify()
        self.thrust.verify()

    def to_pod(self):
        self.verify()
        return self.thrust.to_pod()