from typing import List, NamedTuple, Optional, Callable
import abc
import asyncio
import logging

from expansion import types
from expansion import modules


class JournalRecord(NamedTuple):
    timestamp: int
    # When this record has been added to the journal (ingame milliseconds time)
    action: str
    # What is happening or what has happened

    def __str__(self):
        return f"{self.timestamp}: {self.action}"


class BaseTask:
    global_id: int = 0

    def __init__(self, name: str,
                 system_clock: modules.SystemClock,
                 write_logs: bool = True):
        BaseTask.global_id += 1
        self.id = BaseTask.global_id
        self.name = name
        self.journal: List[JournalRecord] = []
        self.system_clock: modules.SystemClock = system_clock
        self.write_logs = write_logs
        self.finished: bool = False
        self._asyncio_task: Optional[asyncio.Task] = None
        self._logger = logging.getLogger(f"{name}:{self.id}")
        self._complete_cb: Optional[Callable[[bool], None]] = None

    def add_journal_record(self, action: str):
        record = JournalRecord(self.system_clock.cached_time(), action)
        self.journal.append(record)
        if self.write_logs:
            self._logger.info(record)

    async def run(self, *argc, **argv) -> bool:
        self.finished = False
        self.add_journal_record("Started")
        status: bool = False
        try:
            status = await self._impl(*argc, **argv)
            self.add_journal_record("Finished" if status else "Failed")
        except asyncio.CancelledError:
            self.add_journal_record("Canceled")
        except asyncio.TimeoutError:
            self.add_journal_record("Canceled by timeout")
        self.finished = True
        if self._complete_cb:
            self._complete_cb(status)
        return status

    def run_async(self,
                  complete_cb: Optional[Callable[[bool], None]] = None,
                  *argc, **argv):
        self._complete_cb = complete_cb
        self._asyncio_task = asyncio.create_task(self.run(*argc, **argv))

    async def join(self, timeout: Optional[float] = None):
        if self._asyncio_task:
            await asyncio.wait_for(self._asyncio_task, timeout=timeout)

    @abc.abstractmethod
    async def _impl(self, *argc, **argv) -> bool:
        pass

    def interrupt(self):
        if self._asyncio_task is None:
            return
        if self._asyncio_task.cancel():
            self.add_journal_record("Interrupted")
        else:
            self.add_journal_record("Interrupting failed: task is not running")
