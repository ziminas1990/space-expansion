from typing import Optional, Dict, TYPE_CHECKING
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType

if TYPE_CHECKING:
    from server.configurator.world.geomtery import Position
    from server.configurator.resources import ResourcesList
    from .base_module import BaseModule


class ShipBlueprint(BaseBlueprint):

    def __init__(self, name: str,
                 radius: float,
                 weight: float,
                 modules: Dict[str, BlueprintId],
                 expenses: "ResourcesList"):
        super().__init__(
            blueprint_id=BlueprintId(ModuleType.e_SHIP, name),
            expenses=expenses
        )
        self.radius: Optional[float] = radius
        self.weight: Optional[float] = weight
        self.modules: Dict[str, BlueprintId] = modules or {}
        # The 'self.modules' stores all modules, that should be installed on the
        # ship. A key is module name (unique for the ship), a value is a module
        # blueprint's id

    def set_physical_properties(self, radius: float, weight: float):
        self.radius = radius
        self.weight = weight
        return self

    def add_module(self, name: str, blueprint: BlueprintId):
        self.modules.update({name: blueprint})
        return self

    def verify(self):
        super(ShipBlueprint, self).verify()
        assert self.radius and self.radius > 0.01
        assert self.weight and self.weight > 0.01

    def to_pod(self):
        self.verify()
        data = super(ShipBlueprint, self).to_pod()
        data.update({
            "radius": self.radius,
            "weight": self.weight,
            "modules": {name: blueprint_id.to_pod() for name, blueprint_id in self.modules.items()}
        })
        return data


class Ship:
    def __init__(self,
                 name: str,
                 ship_type: str,
                 position: Optional["Position"] = None,
                 modules: Dict[str, "BaseModule"] = {}):
        self.ship_name: str = name
        self.ship_type: str = ship_type
        self.position: Optional["Position"] = position
        self.modules: Dict[str, "BaseModule"] = modules

    def set_position(self, position: "Position") -> 'Ship':
        self.position = position
        return self

    def configure_module(self, name: str, cfg: "BaseModule") -> 'Ship':
        assert name not in self.modules
        self.modules.update({name: cfg})
        return self

    def verify(self):
        assert self.position
        assert self.ship_name and len(self.ship_name) > 0
        assert self.ship_type and len(self.ship_type) > 0
        self.position.verify()

    def to_pod(self):
        self.verify()
        data = self.position.to_pod()
        modules_cfg = {}
        for name, module in self.modules.items():
            assert len(name) > 0
            modules_cfg.update({name: module.to_pod()})
        data.update({"modules": modules_cfg})
        return data
