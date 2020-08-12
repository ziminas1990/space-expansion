from typing import Optional

from expansion.transport.channel import Channel
import expansion.protocol.Privileged_pb2 as privileged
from expansion.protocol.utils import get_message_field


class Access:

    def __init__(self):
        self._channel: Optional[Channel] = None

    def attach_to_channel(self, channel: Channel):
        self._channel = channel

    async def login(self, login: str, password: str) -> (bool, Optional[int]):
        """Try to open privileged session as user with the
        specified 'login' and 'password'. Return tuple (status, token)"""
        message = privileged.Message()
        login_req = message.access.login
        login_req.login = login
        login_req.password = password
        self._channel.send(message)

        response = await self._channel.receive()
        token: Optional[int] = get_message_field(response, 'access.success')
        return token is not None, token
