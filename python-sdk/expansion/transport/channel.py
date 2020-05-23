from typing import Any
import abc


class Channel(abc.ABC):

    @abc.abstractmethod
    def send(self, message: Any) -> bool:
        """Write the specified 'message' to channel. Return true on success,
        otherwise return false"""
        pass

    @abc.abstractmethod
    async def receive(self, timeout: float = 5) -> Any:
        """Await for the message, but not more than 'timeout' seconds"""
        pass

    @abc.abstractmethod
    async def close(self):
        """Close channel"""
        pass