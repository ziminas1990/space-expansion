import asyncio
import os
from typing import Optional
from queue import Queue, Empty
import threading
import subprocess
import tempfile
import time
import yaml

from server.configurator.configuration import Configuration


class Server:
    """Class provides functionality to run and manage a space expansion server.
    Note that a path to the 'space-expansion-server' executable must be listed
    in the 'PATH' environment variable
    """

    @staticmethod
    async def create_async_queue():
        return Queue()

    def __init__(self,):
        self.server_process: Optional[subprocess.Popen] = None
        self.output_queue = None  # Queue must be created inside a coroutine
        self.stdout_reader = None
        # Why delete=False? Because of Windows...
        self.config_file = tempfile.NamedTemporaryFile(
            mode="w", encoding="UTF-8", delete=False)

    async def run(self, configuration: Configuration):
        self.config_file.write(yaml.dump(configuration.to_pod()))
        self.config_file.close()

        server_bin = os.environ["SPEX_SERVER_BINARY"]
        self.server_process = subprocess.Popen(
            args=[server_bin, self.config_file.name],
            stdout=subprocess.PIPE)
        # Wait process to start (but not more than 2 seconds)
        attempts = 40
        while attempts > 0 and not self.is_running():
            time.sleep(0.05)
            attempts -= 1
        assert self.is_running()

        # Run thread to read and store logs
        self.output_queue = await asyncio.wait_for(
            Server.create_async_queue(), timeout=1)

        def read_server_stdout():
            while self.is_running():
                line = self.server_process.stdout.readline().decode("UTF-8")
                self.output_queue.put(line)
        self.stdout_reader = threading.Thread(target=read_server_stdout)
        self.stdout_reader.daemon = True
        self.stdout_reader.start()

        found, _ = self.wait_log(substr="Server has been started", timeout=1)
        assert found

    def wait_log(self, *, substr: str, timeout: float = 1) \
            -> (bool, Optional[str]):
        assert self.output_queue is not None, \
            "Seems that server hst not been run"
        stop_at = time.time() + timeout
        while stop_at > time.time():
            try:
                line = self.output_queue.get(timeout=0.01)
            except Empty:
                continue
            else:
                if line.find(substr) != -1:
                    return True, line
        return False, None

    def is_running(self) -> bool:
        return self.server_process is not None and self.server_process.poll() is None

    def stop(self):
        if self.is_running():
            self.server_process.kill()
            self.server_process.wait(1)
            assert not self.is_running()
        os.remove(self.config_file.name)
