from typing import Optional
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType
from server.configurator.resources import ResourcesList


class PassiveScannerBlueprint(BaseBlueprint):
    def __init__(self, name: str,
                 max_scanning_radius_km: int,
                 edge_update_time_ms: int,
                 expenses: ResourcesList):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_PASSIVE_SCANNER, name),
                         expenses=expenses)
        self.max_scanning_radius_km: Optional[int] = max_scanning_radius_km
        self.edge_update_time_ms: Optional[int] = edge_update_time_ms

    def verify(self):
        super().verify()
        assert self.max_scanning_radius_km is not None and self.max_scanning_radius_km > 0
        assert self.edge_update_time_ms is not None and self.edge_update_time_ms > 0

    def to_pod(self):
        self.verify()
        data = super(PassiveScannerBlueprint, self).to_pod()
        data.update({
            'max_scanning_radius_km': self.max_scanning_radius_km,
            'edge_update_time_ms': self.edge_update_time_ms,
        })
        return data
