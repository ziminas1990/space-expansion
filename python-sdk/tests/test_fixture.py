from typing import Optional
import unittest
import asyncio

from server.configurator.configuration import Configuration
from server.server import Server

import expansion.procedures as procedures
import expansion.interfaces.public as modules


class BaseTestFixture(unittest.TestCase):

    def __init__(self, *args, **kwargs):
        super(BaseTestFixture, self).__init__(*args, **kwargs)
        self.server: Server = Server()
        self.config: Optional[Configuration] = None
        self.login_ports = list(range(5000, 5100))

    def close(self):
        self.server.stop()

    def run_server(self, configuraton: Configuration):
        """Run the server with the specified configuration"""
        self.config = configuraton
        self.server.run(configuraton)

    async def login(self, player: str) -> (Optional[modules.Commutator], Optional[str]):
        if player not in self.config.players:
            return None, f"Player {player} doesn't exist!"
        general_cfg = self.config.general
        port = self.login_ports.pop()
        return await procedures.login(
            server_ip="127.0.0.1",
            login_port=general_cfg.login_udp_port,
            login=self.config.players[player].login,
            password=self.config.players[player].password,
            local_ip="127.0.0.1",
            local_port=port)

    @staticmethod
    def async_test(test_func):
        def sync_runner(*args, **kwargs):
            asyncio.run(test_func(*args, **kwargs))
        return sync_runner
