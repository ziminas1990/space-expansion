from typing import Any, Optional, List, TYPE_CHECKING

from PyQt5.QtCore import QAbstractListModel, QModelIndex, QVariant, Qt

if TYPE_CHECKING:
    from player import Player


class ShipsListModel(QAbstractListModel):

    def __init__(self, player: "Player", *args, **kwargs):
        super(ShipsListModel, self).__init__(*args, **kwargs)
        self.player: "Player" = player
        self.ships_list_cache: List[str] = []
        # This list updates every time, when GUI calls the 'rowCount' function

    def get_ship_name(self, index: QModelIndex) -> Optional[str]:
        assert index.column() == 0
        if index.row() < len(self.ships_list_cache):
            return self.ships_list_cache[index.row()]
        else:
            return None

    # Override QAbstractListModel
    def rowCount(self, parent: QModelIndex = ...) -> int:
        if parent == QModelIndex():
            self.ships_list_cache = list(self.player.ships.keys())
        return len(self.ships_list_cache)

    # Override QAbstractListModel
    def data(self, index: QModelIndex, role: int = ...) -> Any:
        if not index.isValid():
            return QVariant()

        if role == Qt.DisplayRole:
            return QVariant(self.ships_list_cache[index.row()])

        return QVariant()
