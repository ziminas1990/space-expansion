from typing import Any, Optional, NamedTuple
import logging

from .commutator import Commutator
from .navigation import INavigation
from expansion.transport.queued_terminal import Terminal
from expansion.transport.channel import Channel, ChannelMode
from expansion.transport.interfaces_mux import InterfacesMux, Interfaces
from expansion.protocol.utils import get_message_field
import expansion.protocol.Protocol_pb2 as public


class State(NamedTuple):
    weight: float


class Ship(Terminal):

    def __init__(self, ship_name: str = "ship"):
        """Create a new ship instance. Note that ship should be attached to
        the channel. The sprcified 'ship_name' will be used in logs."""
        super().__init__(terminal_name=ship_name)
        self.ship_name: str = ship_name

        self.mux: InterfacesMux = InterfacesMux(f"{self.ship_name}::mux")

        self.commutator: Commutator = Commutator()
        self.commutator.attach_channel(self.mux.get_interface(Interfaces.COMMUTATOR))
        self.mux.get_interface(Interfaces.COMMUTATOR).attach_to_terminal(self.commutator)
        self.mux.get_interface(Interfaces.ENCAPSULATED, ).attach_to_terminal(self.commutator)

        self.navigation: INavigation = INavigation()
        self.navigation.attach_channel(self.mux.get_interface(Interfaces.NAVIGATION))
        self.mux.get_interface(Interfaces.NAVIGATION).attach_to_terminal(self.navigation)

        self.ship_tunnel = self.mux.get_interface(interface=Interfaces.SHIP,
                                                  mode=ChannelMode.PASSIVE)
        self.ship_logger = logging.getLogger()

    def get_navigation(self) -> INavigation:
        return self.navigation

    def get_commutator(self) -> Commutator:
        return self.commutator

    async def get_state(self) -> Optional[State]:
        """Return current ship's state"""
        request = public.Message()
        request.ship.state_req = True
        if not self.ship_tunnel.send(message=request):
            return None
        response = await self.ship_tunnel.receive(timeout=0.5)
        if not response:
            return None
        spec = get_message_field(response, "ship.state")
        if not spec:
            return None
        return State(weight=spec.weight)

    # Overrides Terminal's implementation
    def on_receive(self, message: Any):
        super().on_receive(message=message)  # For logging
        self.mux.on_receive(message)

    # Overrides Terminal's implementation
    def attach_channel(self, channel: 'Channel'):
        super().attach_channel(channel=channel)  # For logging
        self.mux.attach_channel(channel)

    # Overrides Terminal's implementation
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        super().on_channel_mode_changed(mode=mode)  # For logging
        self.mux.on_channel_mode_changed(mode=mode)

    # Overrides Terminal's implementation
    def on_channel_detached(self):
        super().on_channel_detached()  # For logging
        self.mux.on_channel_detached()