from typing import Dict, Optional, Tuple, NamedTuple, Any
from enum import Enum


class ApplicationMode(Enum):
    e_RUN = "run"
    # In the run state an in-game time is running fluently
    e_FREEZE = 'freezed',
    # In the 'freezed' state an in-game time is controlled via a privileged
    # interface


class AdministratorCfg(NamedTuple):
    udp_port: Optional[int] = None
    login: Optional[str] = None
    password: Optional[str] = None


class General:
    """
    Value-semantic class for the 'application' section
    """

    def __init__(self,
                 total_threads: Optional[int] = None,
                 login_udp_port: Optional[int] = None,
                 initial_state: Optional[ApplicationMode] = None,
                 ports_pool: Optional[Tuple[int, int]] = None,
                 administrator_cfg: Optional[AdministratorCfg] = None):
        self.total_threads: Optional[int] = total_threads
        self.login_udp_port: Optional[int] = login_udp_port
        self.initial_state: Optional[ApplicationMode] = initial_state
        self.ports_pool: Optional[Tuple[int, int]] = ports_pool
        self.administrator_cfg: Optional[AdministratorCfg] = administrator_cfg

    def set_total_threads(self, total_thread: int) -> 'General':
        """Set total number of threads, that should be used. This value should be
        not more than total number of CPU cores"""
        self.total_threads = total_thread
        return self

    def set_login_port(self, port: int):
        """Set UDP port, that will be used to receive login requests"""
        assert 0 < port < 65535
        self.login_udp_port = port
        return self

    def set_initial_state(self, state: ApplicationMode):
        self.initial_state = state
        return self

    def set_ports_pool(self, being: int, end: int):
        """Set the pool of ports, that will be used to communicate with
        the clients"""
        self.ports_pool = (being, end)
        return self

    def add_administrator_interface(self, udp_port: int, login :str, password: str):
        """The administrator interface will be created on port 'udp_port'. The
        specified 'login' and 'password' should be used to login as administrator."""
        self.administrator_cfg = AdministratorCfg(udp_port=udp_port,
                                                  login=login,
                                                  password=password)
        return self

    def verify(self):
        """Check that the configuration is full and correct. Will throw an assert
        exception if something is wrong"""
        assert self.total_threads and self.total_threads > 0
        assert self.login_udp_port and 0 < self.login_udp_port < 65535
        assert  self.initial_state
        assert self.ports_pool and 0 < self.ports_pool[0] < self.ports_pool[1] < 65535
        assert self.login_udp_port < self.ports_pool[0] or \
               self.login_udp_port > self.ports_pool[1]
        if self.administrator_cfg:
            assert self.administrator_cfg.login and len(self.administrator_cfg.login) > 4
            assert self.administrator_cfg.password and len(self.administrator_cfg.password) > 6
            assert 0 < self.administrator_cfg.udp_port < 65535
            assert self.administrator_cfg.udp_port < self.ports_pool[0] or \
                   self.administrator_cfg.udp_port > self.ports_pool[1]
            assert self.administrator_cfg.udp_port != self.login_udp_port

    def to_pod(self) -> Dict[str, Any]:
        self.verify()
        pod = {
            "total-threads": self.total_threads,
            "login-udp-port": self.login_udp_port,
            "initial-state": self.initial_state.value,
            "ports-pool": {
                "begin": self.ports_pool[0],
                "end": self.ports_pool[1]
            },
        }
        if self.administrator_cfg:
            pod.update({
                "administrator": {
                    "udp-port": self.administrator_cfg.udp_port,
                    "login": self.administrator_cfg.login,
                    "password": self.administrator_cfg.password
                }
            })
        return pod
