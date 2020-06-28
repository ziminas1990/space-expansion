from typing import  NamedTuple
from enum import Enum

from server.configurator.expenses import Expenses


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

    def __str__(self):
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

