import asyncio
import logging
from typing import Optional, Tuple

from PyQt5.QtWidgets import QApplication
from qtwidgets.login import LoginDialog
from PyQt5.QtWidgets import QDialog
from expansion import procedures

from tactical_core import TacticalCore
from gui import MainWindow, Controller


async def login() -> Tuple[Optional[procedures.Connection], Optional[str]]:
    login_dialog = LoginDialog(
        default_server_ip_port=("127.0.0.1", 6842),
        default_login="Olenoid"
    )

    status = login_dialog.exec()
    if status == QDialog.Rejected:
        return None, "Rejected by user"
    login_data = login_dialog.get_data()

    return await procedures.login(
        server_ip=login_data.server_ip,
        login_port=login_data.server_port,
        login=login_data.login,
        password=login_data.password,
    )


async def main():
    app = QApplication([])

    connection, error = await login()
    if connection is None:
        logging.error(f"Faield to login: {error}")
        return

    tactical_core = TacticalCore(connection.commutator)
    if not await tactical_core.initialize():
        logging.error("Failed to initialize tactical core!")
        return
    # Running tactical core
    core_task = asyncio.create_task(tactical_core.run())

    controller = Controller(tactical_core=tactical_core)

    main_window = MainWindow(tactical_core, controller)
    main_window.show()

    try:
        while not core_task.done():
            await asyncio.sleep(0.02)
            main_window.update()
            app.processEvents()
    except asyncio.CancelledError as ex:
        logging.error(ex)
        tactical_core.stop()
    finally:
        connection.close()

asyncio.run(main())
