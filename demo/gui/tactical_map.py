from typing import Dict, Optional, Union

from PyQt5.QtWidgets import QGraphicsScene, QGraphicsEllipseItem
from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QBrush, QPen, QColor

from .advanced_viewer import AdvancedViewer
from expansion.types import Position
from expansion.modules import Ship
from tactical_core import TacticalCore

import logging


class TacticalMap(AdvancedViewer):
    def __init__(self,
                 tactical_core: TacticalCore,
                 area_size=10000000,
                 *args, **kwargs):
        self.scene = QGraphicsScene(-area_size, -area_size, 2 * area_size, 2 * area_size)
        super().__init__(self.scene, *args, **kwargs)

        self.tactical_core = tactical_core
        self.ships_shapes: Dict[str, QGraphicsEllipseItem] = {}

        self.logger = logging.getLogger("TacticalMap")

    def center_on(self, x: float, y: float):
        self.centerOn(x, y)

    def move_to_ship(self, ship: Union[Ship, str],
                     increase_scale: bool = True):
        """Center tactical map on the ship with the specified 'ship'.
        If the ship is moving too fast and may leave the visible area in less
        than a 5 seconds, the scale map scale may be increased. Set the
        'increase_scale' to 'False' to avoid this behavior."""
        ship_instance: Optional[Ship] = None
        if isinstance(ship, str):
            try:
                ship_instance = self.tactical_core.ships_assistant.ships[ship]
            except KeyError:
                self.logger.warning(f"Can't center on ship '{ship}'! No such ship!")
        else:
            assert isinstance(ship, Ship)
            ship_instance = ship

        if ship is not None:
            position = ship_instance.get_cached_position()
            self.centerOn(position.x, position.y)

    def update(self):
        self._update_ships(self.tactical_core.time.now())

    def _update_ships(self, now: int):
        for ship in self.tactical_core.ships_assistant.ships.values():
            try:
                shape = self.ships_shapes[ship.name]
            except KeyError:
                shape = None

            position: Optional[Position] = ship.get_cached_position(now)
            if position is None:
                if shape is not None:
                    self.logger.warning(f"Ship's '{ship.name}' position is lost!")
                    self.scene.removeItem(shape)
                continue

            if shape is None:
                # Spawning a shape for this ship
                shape = self.scene.addEllipse(0, 0, 100, 100)
                shape.setBrush(QBrush(QColor(255, 0, 0)))
                self.ships_shapes.update({ship.name: shape})

            shape.setRect(
                position.x - 50,
                position.y - 50,
                100,
                100
            )
