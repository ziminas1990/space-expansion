import argparse
import asyncio
import json
import logging
import signal
import sys
from pathlib import Path
from typing import Any, Dict, Mapping, Optional

_SDK_ROOT = Path(__file__).resolve().parents[2]
if str(_SDK_ROOT) not in sys.path:
    sys.path.insert(0, str(_SDK_ROOT))

DEFAULTS: Dict[str, Any] = {
    "server": "127.0.0.1",
    "port": 6842,
    "login": "Olenoid",
    "password": "admin",
}


def parse_args(argv: Optional[list] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Headless harvester: mine asteroids and build miner ships."
    )
    parser.add_argument(
        "--config",
        type=Path,
        help="JSON file with server, port, login, and password",
    )
    parser.add_argument("--server", help="Server IP address")
    parser.add_argument("--port", type=int, help="Login port")
    parser.add_argument("--login", help="Player login")
    parser.add_argument("--password", help="Player password")
    parser.add_argument(
        "--log-level",
        default="INFO",
        help="Logging level (DEBUG, INFO, WARNING, ERROR)",
    )
    return parser.parse_args(argv)


def load_credentials(args: argparse.Namespace) -> Dict[str, Any]:
    credentials = dict(DEFAULTS)
    if args.config is not None:
        try:
            raw = json.loads(args.config.read_text())
        except OSError as error:
            raise SystemExit(f"Failed to read config '{args.config}': {error}")
        except json.JSONDecodeError as error:
            raise SystemExit(f"Invalid JSON in '{args.config}': {error}")
        if not isinstance(raw, Mapping):
            raise SystemExit(f"Config '{args.config}' must be a JSON object")
        for key in DEFAULTS:
            if key in raw:
                credentials[key] = raw[key]
    if args.server is not None:
        credentials["server"] = args.server
    if args.port is not None:
        credentials["port"] = args.port
    if args.login is not None:
        credentials["login"] = args.login
    if args.password is not None:
        credentials["password"] = args.password
    credentials["port"] = int(credentials["port"])
    return credentials


def _install_stop_signals(stop: asyncio.Event) -> None:
    loop = asyncio.get_running_loop()

    def request_stop() -> None:
        stop.set()

    try:
        for sig in (signal.SIGINT, signal.SIGTERM):
            loop.add_signal_handler(sig, request_stop)
    except NotImplementedError:
        for sig in (signal.SIGINT, signal.SIGTERM):
            signal.signal(sig, lambda *_: loop.call_soon_threadsafe(request_stop))


async def _cancel(task: Optional[asyncio.Task]) -> None:
    if task is None or task.done():
        return
    task.cancel()
    try:
        await task
    except asyncio.CancelledError:
        pass


async def async_main(credentials: Mapping[str, Any]) -> int:
    from expansion import procedures
    from demo.harvester.tactical_core import TacticalCore

    stop = asyncio.Event()
    _install_stop_signals(stop)

    connection = None
    tactical_core = None
    core_task: Optional[asyncio.Task] = None

    async def operate() -> int:
        nonlocal connection, tactical_core, core_task
        logging.info(
            "Connecting to %s:%s as '%s'",
            credentials["server"],
            credentials["port"],
            credentials["login"],
        )
        connection, error = await procedures.login(
            server_ip=credentials["server"],
            login_port=credentials["port"],
            login=credentials["login"],
            password=credentials["password"],
        )
        if connection is None:
            logging.error("Failed to login: %s", error)
            return 1

        tactical_core = TacticalCore(connection.commutator)
        if not await tactical_core.initialize():
            logging.error("Failed to initialize tactical core!")
            return 1

        core_task = asyncio.create_task(tactical_core.run())
        logging.info("Harvester is running. Press Ctrl+C to stop.")
        await core_task
        if not stop.is_set():
            logging.error("Tactical core stopped unexpectedly")
            return 1
        return 0

    operate_task = asyncio.create_task(operate())
    stop_task = asyncio.create_task(stop.wait())
    try:
        done, _ = await asyncio.wait(
            {operate_task, stop_task},
            return_when=asyncio.FIRST_COMPLETED,
        )
        if operate_task in done:
            return operate_task.result()
        logging.info("Shutdown requested")
        await _cancel(operate_task)
        return 0
    finally:
        if tactical_core is not None:
            await tactical_core.stop()
        await _cancel(core_task)
        if connection is not None:
            connection.close()
        await _cancel(stop_task)


def run(argv: Optional[list] = None) -> None:
    args = parse_args(argv)
    logging.basicConfig(
        level=getattr(logging, args.log_level.upper(), logging.INFO),
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    credentials = load_credentials(args)
    try:
        raise SystemExit(asyncio.run(async_main(credentials)))
    except KeyboardInterrupt:
        raise SystemExit(0)


if __name__ == "__main__":
    run()
