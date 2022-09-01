from PyQt5.QtWidgets import QWidget, QListView, QGridLayout, QLabel
from PyQt5.QtCore import QModelIndex
import logging

from tactical_core import TacticalCore
from .tactical_map import TacticalMap
from .ships_list_model import ShipsListModel
from .controller import Controller
from world import World


class MainWindow(QWidget):
    def __init__(self,
                 tactical_core: TacticalCore,
                 controller: Controller, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.tactical_core = tactical_core
        self.controller = controller
        self.main_grid = QGridLayout(self)

        self.lbl_time_value = QLabel(self)
        self.lbl_time = QLabel(self)
        self.lbl_time.setText("Time:")

        self.ships_list_model = ShipsListModel(self.tactical_core.player)
        self.ships_list = QListView(self)
        self.ships_list.setModel(self.ships_list_model)

        self.tactical_map: TacticalMap = TacticalMap(
            tactical_core=self.tactical_core)

        self.main_grid.addWidget(self.lbl_time, 0, 0)
        self.main_grid.addWidget(self.lbl_time_value, 0, 1)
        self.main_grid.addWidget(self.ships_list, 1, 0)
        self.main_grid.addWidget(self.tactical_map, 1, 1, 2, 2)
        self.main_grid.setRowStretch(0, 0)
        self.main_grid.setRowStretch(1, 1)

        self.ships_list.clicked.connect(self._ships_selected)
        self.tactical_map.left_click.connect(self._map_left_clicked)

    def update(self):
        self._update_time()
        self.tactical_map.update()
        self.ships_list.update()

    def _ships_selected(self, index: QModelIndex):
        ship_name = self.ships_list_model.get_ship_name(index)
        if ship_name is None:
            return
        self.tactical_map.move_to_ship(ship_name)
        self.controller.select_ships([ship_name])

    def _update_time(self):
        millis: int = int(self.tactical_core.time.predict_usec() / 1000)
        seconds: int = int(millis / 1000)
        minutes: int = int(seconds / 60)
        hours: int = int(minutes / 60)
        millis %= 1000
        seconds %= 60
        minutes %= 60
        self.lbl_time_value.setText(f"{hours:4}h {minutes:02}m {seconds:02}s {millis:03}ms")

    def _map_left_clicked(self, x: float, y: float):
        self.controller.left_click(x, y)
