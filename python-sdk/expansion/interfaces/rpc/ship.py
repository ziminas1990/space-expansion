from typing import Any, Optional, NamedTuple
import logging

from .commutator import CommutatorI
from .navigation import INavigation
from expansion.transport import IOTerminal
from expansion.protocol.utils import get_message_field
import expansion.protocol.Protocol_pb2 as public


class State(NamedTuple):
    weight: float


class ShipI(CommutatorI, INavigation, IOTerminal):

    def __init__(self, ship_name: str = "ship"):
        """Create a new ship instance. Note that ship should be attached to
        the channel. The specified 'ship_name' will be used in logs."""
        super().__init__(name=ship_name)
        self.ship_name: str = ship_name
        self.ship_logger = logging.getLogger()

    def navigation(self) -> INavigation:
        # Just for better readability
        return self

    def commutator(self) -> CommutatorI:
        # Just for better readability
        return self

    async def get_state(self) -> Optional[State]:
        """Return current ship's state"""
        request = public.Message()
        request.ship.state_req = True
        if not self.send(message=request):
            return None
        response, _ = await self.wait_message(timeout=0.5)
        if not response:
            return None
        spec = get_message_field(response, "ship.state")
        if not spec:
            return None
        return State(weight=spec.weight)
