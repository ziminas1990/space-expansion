from typing import List, NamedTuple, Optional
import abc
import asyncio

from expansion import types


class JournalRecord(NamedTuple):
    timestamp: int
    # When this record has been added to the journal (ingame milliseconds time)
    action: str
    # What is happening or what has happened


class BaseTask:
    def __init__(self, name: str, time: types.TimePoint):
        self.name = name
        self.journal: List[JournalRecord] = []
        self.time: types.TimePoint = time
        self._asyncio_task: Optional[asyncio.Task] = None

    def add_journal_record(self, action: str, timestamp: Optional[int] = None):
        if timestamp is None:
            timestamp = self.time.now()
        self.journal.append(JournalRecord(timestamp, action))

    def run(self, *argc, **argv):
        self._asyncio_task = asyncio.create_task(self._impl(*argc, **argv))

    @abc.abstractmethod
    async def _impl(self, *argc, **argv):
        pass

    def interrupt(self):
        if self._asyncio_task is None:
            return
        self.add_journal_record("interrupted")
        self._asyncio_task.cancel()
