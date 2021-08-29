from typing import Optional
from server.configurator.blueprints import BaseBlueprint, BlueprintId, ModuleType
from server.configurator.resources import ResourcesList


class AsteroidMinerBlueprint(BaseBlueprint):
    def __init__(self, name: str,
                 max_distance: int,
                 cycle_time_ms: int,
                 yield_per_cycle: int,
                 expenses: ResourcesList):
        super().__init__(
            blueprint_id=BlueprintId(ModuleType.e_ASTEROID_MINER, name),
            expenses=expenses)
        self.max_distance: Optional[int] = max_distance
        self.cycle_time_ms: Optional[int] = cycle_time_ms
        self.yield_per_cycle: Optional[int] = yield_per_cycle

    def verify(self):
        super().verify()
        assert self.max_distance is not None and self.max_distance > 1
        assert self.cycle_time_ms is not None and self.cycle_time_ms > 1000
        assert self.yield_per_cycle is not None and self.yield_per_cycle > 1

    def to_pod(self):
        self.verify()
        data = super(AsteroidMinerBlueprint, self).to_pod()
        data.update({
            'max_distance': self.max_distance,
            'cycle_time_ms': self.cycle_time_ms,
            'yield_per_cycle': self.yield_per_cycle
        })
        return data
