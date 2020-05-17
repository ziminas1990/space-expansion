from typing import Any
import abc


class Channel(abc.ABC):

    @abc.abstractmethod
    def send(self, message: Any):
        """Write the specified 'message' to channel"""
        pass

    @abc.abstractmethod
    async def receive(self, timeout: float = 5) -> Any:
        """Await for the message, but not more than 'timeout' seconds"""
        pass