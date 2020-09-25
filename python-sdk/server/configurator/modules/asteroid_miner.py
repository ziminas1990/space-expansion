from typing import Optional
from server.configurator.blueprints.base_blueprint import BaseBlueprint, BlueprintId, ModuleType


class AsteroidMinerBlueprint(BaseBlueprint):
    def __init__(self, name: str,
                 max_distance: Optional[int] = None,
                 cycle_time_ms: Optional[int] = None,
                 yield_per_cycle: Optional[int] = None,
                 container: Optional[str] = None):
        super().__init__(blueprint_id=BlueprintId(ModuleType.e_ASTEROID_MINER, name))
        self.max_distance: Optional[int] = max_distance
        self.cycle_time_ms: Optional[int] = cycle_time_ms
        self.yield_per_cycle: Optional[int] = yield_per_cycle
        self.container: Optional[str] = container

    def verify(self):
        super().verify()
        assert self.max_distance is not None and self.max_distance > 0
        assert self.cycle_time_ms is not None and self.cycle_time_ms > 0
        assert self.yield_per_cycle is not None and self.yield_per_cycle > 0
        assert self.container

    def to_pod(self):
        self.verify()
        data = super(AsteroidMinerBlueprint, self).to_pod()
        data.update({
            'max_distance': self.max_distance,
            'cycle_time_ms': self.cycle_time_ms,
            'yield_per_cycle': self.yield_per_cycle,
            'container': self.container,
        })
        return data
