from typing import Optional, NamedTuple, List

import expansion.protocol as protocol
import expansion.transport as transport
import expansion.utils as utils


class Specification(NamedTuple):
    max_distance: int
    cycle_time_ms: int
    yield_per_cycle: int


class AsteroidMiner(transport.QueuedTerminal):

    def __init__(self, name: Optional[str] = None):
        super().__init__(terminal_name=name or utils.generate_name(AsteroidMiner))
        self.specification: Optional[Specification] = None

    async def get_specification(self, timeout: float = 0.5, reset_cached=False)\
            -> Optional[Specification]:
        if reset_cached:
            self.specification = None
        if self.specification:
            return self.specification
        request = protocol.Message()
        request.asteroid_miner.specification_req = True
        if not self.send_message(message=request):
            return None
        response = await self.wait_message(timeout=timeout)
        if not response:
            return None
        spec = protocol.get_message_field(response, "asteroid_miner.specification")
        if not spec:
            return None
        self.specification = Specification(max_distance=spec.max_distance,
                                           cycle_time_ms=spec.cycle_time_ms,
                                           yield_per_cycle=spec.yield_per_cycle)
        return self.specification
