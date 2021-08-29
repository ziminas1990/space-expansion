from typing import Optional
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType
from .base_module import BaseModule
from server.configurator.world.geomtery import Vector
from server.configurator.resources import ResourcesList


class EngineBlueprint(BaseBlueprint):
    def __init__(self,
                 name: str,
                 max_thrust: int,
                 expenses: ResourcesList):
        super().__init__(
            blueprint_id=BlueprintId(ModuleType.e_ENGINE, name),
            expenses=expenses)
        self.max_thrust: Optional[int] = max_thrust

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


class EngineState(BaseModule):
    """Describes an initial state of the engine module"""

    def __init__(self, thrust: Vector = Vector(x=0, y=0)):
        super(EngineState, self).__init__()
        self.thrust: Vector = thrust

    def set_thrust(self, thrust: Vector) -> 'EngineState':
        self.thrust = thrust
        return self
        
    def verify(self):
        super(EngineState, self).verify()
        self.thrust.verify()

    def to_pod(self):
        self.verify()
        return self.thrust.to_pod()
