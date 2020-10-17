from typing import Optional, Dict
from enum import Enum

from .terminal import Terminal
import expansion.protocol.Protocol_pb2 as public
from .channel import Channel
from .pipe import Pipe

from expansion import utils


class Interfaces(Enum):
    ENCAPSULATED = "encapsulated"
    ACCESS_PANEL = "accessPanel"
    COMMUTATOR = "commutator"
    SHIP = "ship"
    NAVIGATION = "navigation"
    ENGINE = "engine"
    CELESTIAL_SCANNER = "celestial_scanner"
    ASTEROID_SCANNER = "asteroid_scanner"
    RESOURCE_CONTAINER = "resource_container"
    ASTEROID_MINER = "asteroid_miner"
    BLUEPRINTS_LIBRARY = "blueprints_library"
    SHIPYARD = "shipyard"
    GAME = "game"


class InterfacesMux(Terminal):

    def __init__(self, name: Optional[str] = None):
        self.name: str = name or utils.generate_name(InterfacesMux)

        super().__init__(terminal_name=self.name)
        self.downlevel: Optional[Channel] = None
        self.interfaces: Dict[Interfaces, Pipe] = {}

    # Override from Terminal
    def on_receive(self, message: public.Message):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        self.terminal_logger.debug(f"Got a message for the '{message.WhichOneof('choice')}' interface")
        channel: Optional[Pipe] = self.interfaces[message.WhichOneof("choice")]
        if channel:
            channel.on_message(message=message)
        else:
            self.terminal_logger.warning("No channel for the '{message.WhichOneof('choice')}'"
                                         " interface has been created")

    # Override from Terminal
    def attach_channel(self, channel: 'Channel'):
        super().attach_channel(channel=channel)  # for logging
        self.downlevel = channel
        for pipe in self.interfaces.values():
            pipe.attach_channel(self.downlevel)

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # for logging
        self.downlevel = None
        for pipe in self.interfaces.values():
            pipe.on_channel_detached()

    def get_interface(self, interface: Interfaces) -> Channel:
        """
        Return a channel for the specified 'interface'. If the channel is not
        been created yet, it will be created.
        """
        if interface.value in self.interfaces:
            return self.interfaces[interface.value]
        pipe: Pipe = Pipe(
            name=f"{self.name}/{interface.value}"
        )
        if self.downlevel:
            pipe.attach_channel(self.downlevel)
        self.interfaces[interface.value] = pipe
        return pipe
