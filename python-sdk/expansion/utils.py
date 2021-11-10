from typing import Type, Dict, Any, Callable
import asyncio
import os

__next_ids: Dict[Type, int] = {}


def generate_name(cls: Type):
    """Generate a name for the instance of the specified 'cls'. The name will
    contain the class name and a unique identifier."""
    id: int = __next_ids[cls] if cls in __next_ids else 1
    __next_ids.update({cls: id + 1})
    return f"{cls.__name__}_{id}"


def set_asyncio_policy():
    if os.name == "nt":
        # Since IOCP proactor doesn't implement 'create_datagram_endpoint()',
        # a Selector event loop should be used instead
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    else:
        asyncio.set_event_loop_policy(asyncio.DefaultEventLoopPolicy())