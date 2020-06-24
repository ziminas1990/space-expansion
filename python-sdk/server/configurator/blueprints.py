from typing import Dict, Optional, Tuple, NamedTuple, Any
from enum import Enum

from .expenses import Expenses


class ModuleType(Enum):
    e_SHIP = "Ship"
    e_ENGINE = "Engine"
    e_CELESTIAL_SCANNER = "CelestialScanner"


class BlueprintId(NamedTuple):
    type: ModuleType
    name: str

    def verify(self):
        assert len(self.name) > 0

    def to_pod(self):
        self.verify()
        return f"{self.type.value}/{self.name}"

    @staticmethod
    def engine(name: str):
        return BlueprintId(type=ModuleType.e_ENGINE, name=name)


class BaseBlueprint:
    def __init__(self, blueprint_id: BlueprintId):
        self.id: BlueprintId = blueprint_id
        self.expenses: Expenses = Expenses()

    def set_expenses(self, expenses: Expenses):
        self.expenses = expenses
        return self

    def verify(self):
        self.id.verify()
        self.expenses.verify()

    def to_pod(self):
        self.verify()
        return {
            "expenses": self.expenses.to_pod()
        }


class BlueprintsStorage:
    def __init__(self):
        self.storage: Dict[BlueprintId, BaseBlueprint] = {}

    def add(self, blueprint: BaseBlueprint):
        self.storage.update({blueprint.id: blueprint})

    def verify(self):
        for id, blueprint in self.storage.items():
            assert id == blueprint.id
            blueprint.verify()

    def to_pod(self):
        self.verify()
        data = {}
        for name, blueprint in self.storage.items():
            data.update({name: blueprint.to_pod()})
        return data


class Blueprints:
    def __init__(self):
        self.blueprints: Dict[BlueprintId, BaseBlueprint] = {}

    def add_blueprint(self, blueprint: BaseBlueprint):
        self.blueprints.update({blueprint.id: blueprint})
        return self

    def verify(self):
        for id, blueprint in self.blueprints.items():
            assert id == blueprint.id
            blueprint.verify()
            # Ship blueprint stores information about modules, that should be installed
            # on a ship. Let's check that blueprints for this modules also exists
            # in the configuration
            if id.type == ModuleType.e_SHIP:
                assert isinstance(blueprint, ShipBlueprint)
                for ships_module_blueprint in blueprint.modules.values():
                    assert ships_module_blueprint.type != ModuleType.e_SHIP
                    assert ships_module_blueprint in self.blueprints

    def to_pod(self):
        self.verify()
        modules = {}
        ships = {}
        for id, blueprint in self.blueprints.items():
            if id.type == ModuleType.e_SHIP:
                ships.update({id.name: blueprint.to_pod()})
            else:
                modules.setdefault(id.type.value, {}).update({id.name: blueprint.to_pod()})
        return {
            "Modules": modules,
            "Ships": ships
        }


class ShipBlueprint(BaseBlueprint):

    def __init__(self, name: str):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_SHIP, name))
        self.radius: Optional[float] = None
        self.weight: Optional[float] = None
        self.modules: Dict[str, BlueprintId] = {}
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


class EngineBlueprint(BaseBlueprint):
    def __init__(self, name: str):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_ENGINE, name))
        self.max_thrust: Optional[int] = None

    def set_max_thrust(self, max_thrust: int) -> 'EngineBlueprint':
        self.max_thrust = max_thrust
        return self

    def to_pod(self):
        self.verify()
        data = super(EngineBlueprint, self).to_pod()
        data.update({
            'max_thrust': self.max_thrust
        })
        return data
