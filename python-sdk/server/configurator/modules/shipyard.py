from typing import TYPE_CHECKING
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType

if TYPE_CHECKING:
    from server.configurator.resources import ResourcesList


class ShipyardBlueprint(BaseBlueprint):
    def __init__(self, name: str,
                 productivity: float,
                 container_name: str,
                 expenses: "ResourcesList"):
        super().__init__(
            blueprint_id=BlueprintId(ModuleType.e_SHIPYARD, name),
            expenses=expenses)
        self.productivity = productivity
        self.container_name = container_name

    def verify(self):
        super().verify()
        assert self.productivity is not None
        assert self.productivity > 0
        assert self.container_name

    def to_pod(self):
        self.verify()
        data = super(ShipyardBlueprint, self).to_pod()
        data.update({
            'productivity': self.productivity,
            'container_name': self.container_name
        })
        return data
