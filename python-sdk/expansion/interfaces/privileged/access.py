from typing import Optional

from expansion.transport import Channel, IOTerminal
import expansion.protocol.Privileged_pb2 as privileged
from expansion.protocol.utils import get_message_field


class Access:

    def __init__(self):
        self._socket: IOTerminal = IOTerminal()

    def fasten_to_channel(self, channel: Channel):
        self._socket.wrap_channel(channel)

    async def login(self, login: str, password: str) -> (bool, Optional[int]):
        """Try to open privileged session as user with the
        specified 'login' and 'password'. Return tuple (status, token)"""
        message = privileged.Message()
        login_req = message.access.login
        login_req.login = login
        login_req.password = password
        self._socket.send(message)

        response, _ = await self._socket.wait_message()
        token: Optional[int] = get_message_field(response, ["access", "success"])
        return token is not None, token
