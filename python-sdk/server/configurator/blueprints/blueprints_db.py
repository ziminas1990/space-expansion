from typing import Dict

from .base_blueprint import BlueprintId, BaseBlueprint, ModuleType
from server.configurator.modules.ship import ShipBlueprint


class BlueprintsDict:
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


class BlueprintsDB:
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
