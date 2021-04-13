from typing import Any, Optional, NamedTuple, Callable
import logging

from .commutator import CommutatorI
from .navigation import INavigation
from expansion.transport import IOTerminal
from expansion.protocol.utils import get_message_field
import expansion.protocol.Protocol_pb2 as api
from expansion import types


class State(NamedTuple):
    timestamp: Optional[types.TimePoint]
    weight: Optional[float]
    position: Optional[types.Position]

    @staticmethod
    def build(state: api.IShip.State, timestamp: Optional[int]):
        position: Optional[types.Position] = None
        if state.position:
            position = types.Position.build(state.position, timestamp)

        return State(
            timestamp=types.TimePoint(timestamp),
            weight=state.weight.value if state.weight else None,
            position=position
        )

    def expired(self, timeout_ms: int = 100) -> bool:
        assert self.timestamp is not None
        return self.timestamp.dt_sec() * 1000 > timeout_ms


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

    async def wait_state(self) -> Optional[State]:
        """Await a State message with actual ship's state"""
        message, timestamp = await self.wait_message(timeout=0.5)
        if not message:
            return None
        state = get_message_field(message, "ship.state")
        return State.build(state, timestamp) if state else None

    async def get_state(self) -> Optional[State]:
        """Return current ship's state"""
        request = api.Message()
        request.ship.state_req = True
        if not self.send(message=request):
            return None
        return await self.wait_state()

    async def monitor(self, duration_ms: int) -> Optional[int]:
        """Start ship's state monitoring. Return actual monitoring duration.
        After this call you may use 'wait_state()' to receive updates
        """
        request = msg.Message()
        request.ship.monitor = True
        if not self.send(message=request):
            return None
        ack, _ = self.wait_exact("ship.monitor_ack")
        return ack
