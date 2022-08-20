from typing import Callable
import asyncio


async def wait_for(predicate: Callable[[], bool], timeout: float = 1) -> bool:
    while timeout > 0:
        if predicate():
            return True
        await asyncio.sleep(0.1)
        timeout -= 0.1
    return False
