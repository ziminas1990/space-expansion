from typing import Any, Optional, List

from PyQt5.QtCore import QAbstractListModel, QModelIndex, QVariant, Qt

from tactical_core import TacticalCore
from expansion import modules


class ShipsListModel(QAbstractListModel):

    def __init__(self, tactical_core: TacticalCore, *args, **kwargs):
        super(ShipsListModel, self).__init__(*args, **kwargs)
        self.tactical_core: TacticalCore = tactical_core
        self.ships_list_cache: List[modules.Ship] = []
        # This list updates every time, when GUI calls the 'rowCount' function

    def get_ship(self, index: QModelIndex) -> Optional[modules.Ship]:
        assert index.column() == 0
        if index.row() < len(self.ships_list_cache):
            return self.ships_list_cache[index.row()]
        else:
            return None

    # Override QAbstractListModel
    def rowCount(self, parent: QModelIndex = ...) -> int:
        if parent == QModelIndex():
            self.ships_list_cache = modules.get_all_ships(self.tactical_core.root_commutator)
        return len(self.ships_list_cache)

    # Override QAbstractListModel
    def data(self, index: QModelIndex, role: int = ...) -> Any:
        if not index.isValid():
            return QVariant()

        if role == Qt.DisplayRole:
            return QVariant(self.ships_list_cache[index.row()].name)

        return QVariant()
