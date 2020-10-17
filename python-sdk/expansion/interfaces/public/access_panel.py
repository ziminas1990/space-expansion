from typing import Optional

from expansion.transport import Channel, IOTerminal
from expansion.protocol.Protocol_pb2 import Message as PlayerMessage
from expansion.protocol.utils import get_message_field


class IAccessPanel:

    def __init__(self):
        self.socket: IOTerminal = IOTerminal()

    def attach_to_channel(self, channel: Channel):
        self.socket.wrap_channel(channel)

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
        self.socket.send(message)

        response = await self.socket.wait_message()
        port: Optional[int] = get_message_field(response, 'accessPanel.access_granted')
        if port:
            return port, None

        error: Optional[str] = get_message_field(response, 'accessPanel.access_rejected')
        return 0, error if error is not None else "Unexpected response"
