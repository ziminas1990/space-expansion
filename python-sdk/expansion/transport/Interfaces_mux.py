from typing import Optional, Dict
from enum import Enum
import logging

from .terminal import Terminal
import expansion.protocol.Protocol_pb2 as public
from .channel import Channel, ChannelMode
from .channel_adapter import ChannelAdapter


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
    next_interface_mux_instance_id = 0

    def __init__(self, name: str = "MUX"):
        self.interface_mux_instance_id = InterfacesMux.next_interface_mux_instance_id
        InterfacesMux.next_interface_mux_instance_id += 1
        self.name: str = f"{name}_{self.interface_mux_instance_id}"

        super().__init__(terminal_name=f"{self.name}::Terminal")
        self.downlevel: Optional[Channel] = None
        self.interfaces: Dict[Interfaces, ChannelAdapter] = {}

    # Override from Terminal
    def on_receive(self, message: public.Message):
        """This callback will be called to pass received message, that was
        addressed to this terminal"""
        self.terminal_logger.debug(f"Got a message for the '{message.WhichOneof('choice')}' interface")
        channel: Optional[ChannelAdapter] = self.interfaces[message.WhichOneof("choice")]
        if channel:
            channel.on_message(message=message)
        else:
            self.terminal_logger.warning("No channel for the '{message.WhichOneof('choice')}'"
                                         " interface has been created")

    # Override from Terminal
    def attach_channel(self, channel: 'Channel'):
        super().attach_channel(channel=channel)  # for logging
        assert channel.mode == ChannelMode.ACTIVE
        self.downlevel = channel

    # Override from Terminal
    def on_channel_mode_changed(self, mode: 'ChannelMode'):
        super().on_channel_mode_changed(mode=mode)  # for logging
        assert mode == ChannelMode.ACTIVE

    # Override from Terminal
    def on_channel_detached(self):
        super().on_channel_detached()  # for logging
        self.downlevel = None

    def get_interface(self, interface: Interfaces) -> Channel:
        """
        Return a channel for the specified 'interface'. If the channel is not
        been created yet, it will be created.
        """
        if interface.value in self.interfaces:
            return self.interfaces[interface.value]
        channel: ChannelAdapter = ChannelAdapter(
            send_fn=lambda message: self.downlevel.send(message=message),
            close_fn=lambda: None,
            receive_fn=None,
            name=f"{self.name}::{interface}"
        )
        self.interfaces[interface.value] = channel
        return channel
