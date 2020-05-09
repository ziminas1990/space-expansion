from typing import Optional, Union

from transport import channel
from protocol.Protocol_pb2 import Message as PlayerMessage
from protocol.utils import get_message_field

class IAccessPanel:

    def __init__(self):
        self._channel: Optional[channel.Channel] = None

    def attach_to_channel(self, channel: channel.Channel):
        self._channel = channel

    async def login(self, login: str, password: str,
                    local_ip: str, local_port: int) -> (int, Optional[str]):
        """Try to open privileged session as user with the
        specified 'login' and 'password'.
        Return tuple (port, error_string)"""
        message = PlayerMessage()
        login_req = message.accessPanel.login
        login_req.login = login
        login_req.password = password
        login_req.ip = local_ip
        login_req.port = local_port
        self._channel.send(message)

        response = await self._channel.receive()
        port: Optional[int] = get_message_field(response, ['accessPanel', 'access_granted'])
        if port:
            return port, None

        error: Optional[str] = get_message_field(response, ['accessPanel', 'access_rejected'])
        return 0, error if error is not None else "Unexpected response"