from typing import Dict, Optional, Union, Set
import math

from PyQt5.QtWidgets import (
    QWidget,
    QGraphicsScene,
    QGraphicsEllipseItem,
    QGraphicsRectItem,
    QGraphicsItem,
    QStyleOptionGraphicsItem
)
from PyQt5.QtCore import QRectF
from PyQt5.QtGui import QBrush, QPen, QColor, QPainter, QTransform

from .advanced_viewer import AdvancedViewer
from expansion.types import Position
from expansion.modules import Ship
from tactical_core import TacticalCore

import logging


class MapItem:
    def __init__(self,
                 radius: float,
                 scene: QGraphicsScene,
                 color: QColor,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.scene: QGraphicsScene = scene
        self.r = radius
        self.x: float = 0
        self.y: float = 0

        self.body_item: Optional[QGraphicsEllipseItem] = None
        self.body_color = color
        self.mark_item: Optional[QGraphicsRectItem] = None

    def spawn(self):
        assert self.body_item is None
        r = self.r / 2
        self.body_item = self.scene.addEllipse(-r, -r, 2*r, 2*r)
        self.body_item.setBrush(QBrush(self.body_color))

    def mark(self,
             size: int = 10,
             color: QColor = QColor(0, 255, 0, 100)):
        assert self.mark_item is None
        size /= 2
        self.mark_item = self.scene.addRect(-size, -size, 2 * size, 2 * size)
        self.mark_item.setBrush(color)
        self.mark_item.setFlag(QGraphicsItem.ItemIgnoresTransformations, True)

    def unmark(self):
        if self.mark_item:
            self.scene.removeItem(self.mark_item)
        self.mark_item = None

    def move(self, x: float, y: float):
        if self.body_item:
            self.body_item.setPos(x, y)
        if self.mark_item:
            self.mark_item.setPos(x, y)


class TacticalMap(AdvancedViewer):
    def __init__(self,
                 tactical_core: TacticalCore,
                 area_size=10000000,
                 *args, **kwargs):
        self.scene = QGraphicsScene(-area_size, -area_size, 2 * area_size, 2 * area_size)
        self.scene.setItemIndexMethod(QGraphicsScene.NoIndex)
        super().__init__(self.scene, *args, **kwargs)

        self.tactical_core = tactical_core
        self.ships: Dict[str, MapItem] = {}
        self.asteroids: Dict[int, MapItem] = {}
        self.markers: Set[QGraphicsRectItem] = set()

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
        self._update_asteroids(self.tactical_core.time.now())

    def _update_ships(self, now: int):
        for ship in self.tactical_core.ships_assistant.ships.values():
            try:
                item = self.ships[ship.name]
            except KeyError:
                item = None

            position: Optional[Position] = ship.get_cached_position(now)
            if position is None:
                if item is not None:
                    self.logger.warning(f"Ship's '{ship.name}' position is lost!")
                    self.scene.removeItem(item)
                continue

            if item is None:
                # Spawning a shape for this ship
                item = MapItem(100, self.scene, QColor("red"))
                item.spawn()
                item.mark()
                self.ships.update({ship.name: item})

            item.move(position.x, position.y)

    def _update_asteroids(self, now: int):
        for asteroid in self.tactical_core.asteroids_tracker.asteroids.values():
            try:
                item = self.asteroids[asteroid.object_id]
            except KeyError:
                item = None

            if asteroid.position is None:
                if item is not None:
                    self.logger.warning(
                        f"Asteroid's #'{asteroid.object_id}' position is lost!")
                    self.scene.removeItem(item)
                continue
            position: Optional[Position] = asteroid.position.make_prediction(now)

            if item is None:
                # Spawning a shape for this ship
                item = MapItem(asteroid.radius, self.scene, QColor("blue"))
                item.spawn()
                # Let the size of the mark depend on the size of the asteroid
                mark_size = 3 + int((17 * asteroid.radius / 20))
                mark_size = min(mark_size, 20)
                item.mark(size=mark_size, color=QColor(0, 0, 255, 100))
                self.asteroids.update({asteroid.object_id: item})

            item.move(position.x, position.y)
