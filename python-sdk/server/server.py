from typing import Optional
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

    def __init__(self):
        self.server_process: Optional[subprocess.Popen] = None
        self.config_file = tempfile.NamedTemporaryFile(mode="w")

    def run(self, configuration: Configuration):
        self.config_file.write(yaml.dump(configuration.to_pod()))
        self.config_file.flush()

        self.server_process = subprocess.Popen(
                ["space-expansion-server", self.config_file.name])
        time.sleep(0.05)
        assert self.is_running()

    def is_running(self) -> bool:
        return self.server_process is not None and self.server_process.poll() is None

    def stop(self):
        if self.is_running():
            self.server_process.kill()
