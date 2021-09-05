import logging
from typing import Dict, Optional, Set, NamedTuple, List, TYPE_CHECKING

from PyQt5.QtWidgets import (
    QGraphicsScene,
    QGraphicsEllipseItem,
    QGraphicsRectItem,
    QGraphicsItem,
    QGraphicsLineItem,
)
from PyQt5.QtGui import QBrush, QPen, QColor

from .advanced_viewer import AdvancedViewer
from expansion.types import Position
from expansion.procedures.navigation import FlightPlan
from tactical_core import TacticalCore

if TYPE_CHECKING:
    from ship import Ship


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

    def release(self):
        if self.body_item:
            self.scene.removeItem(self.body_item)
            self.body_item = None
        if self.mark_item:
            self.scene.removeItem(self.mark_item)
            self.mark_item = None

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


class Trace:

    class Segment(NamedTuple):
        x: float
        y: float
        timestamp: int
        item: Optional[QGraphicsLineItem]

    def __init__(self,
                 scene: QGraphicsScene,
                 color: QColor,
                 delay_sec: float = 5,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.scene: QGraphicsScene = scene
        self.color = color
        self.delay: int = round(delay_sec * 10 ** 6)
        self.segments: List["Trace.Segment"] = []

    def release(self):
        for segment in self.segments:
            if segment.item:
                self.scene.removeItem(segment.item)
        self.segments = []

    def append(self, x: float, y: float, timestamp: int):
        if self.segments:
            last = self.segments[-1]
            assert timestamp > last.timestamp
            segment = Trace.Segment(
                x, y, timestamp,
                item=self.scene.addLine(last.x, last.y, x, y)
            )
            segment.item.setPen(QPen(self.color, 3))
        else:
            segment = Trace.Segment(
                x, y, timestamp, item=None
            )
        #segment.item.setBrush(self.color)
        self.segments.append(segment)

    def update(self, now: int):
        while self.segments and self.segments[0].timestamp + self.delay < now:
            segment = self.segments.pop(0)
            self.scene.removeItem(segment.item)


class FlightPlanItem:
    def __init__(self,
                 plan: FlightPlan,
                 scene: QGraphicsScene,
                 color: QColor):
        self.trace: Optional[Trace] = None
        self.scene = scene
        self.color = color
        self.plan = plan

    def release(self):
        self.trace.release()

    def update(self, ship: "Ship", now: int):
        if self.trace:
            self.trace.update(now)
            return

        if not self.trace and self.plan.starts_at() < now:
            self.trace = Trace(self.scene, self.color, delay_sec=0.1)
            position = ship.predict_position(now)
            if not position:
                return
            step = max(0.10, self.plan.duration_sec() / 250)
            points = self.plan.build_path(position, step_ms=round(step * 1000))
            for point in points:
                self.trace.append(point.x, point.y, point.timestamp.usec())


class ShipItem:
    def __init__(self,
                 scene: QGraphicsScene,
                 ship_size: int,
                 ship_color: QColor,
                 *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.scene: QGraphicsScene = scene
        self.ship_item = MapItem(radius=ship_size, scene=scene, color=ship_color)
        self.path = Trace(scene=scene, color=QColor(0, 0, 255, 100), delay_sec=10)
        self.flight_plan: Optional[FlightPlanItem] = None

    def release(self):
        self.ship_item.release()
        self.path.release()

    def spawn(self):
        self.ship_item.spawn()

    def mark(self, *args, **kwargs):
        self.ship_item.mark(*args, **kwargs)

    def update(self, ship: "Ship", now: int) -> bool:
        position: Optional[Position] = ship.predict_position(now)
        if not position:
            return False
        self.ship_item.move(position.x, position.y)

        if not self.path.segments or now - self.path.segments[-1].timestamp > 125000:
            self.path.append(position.x, position.y, timestamp=position.timestamp.usec())
            self.path.update(now)

        if ship.navigator.task_move_to:
            plan = ship.navigator.task_move_to.plan
            if self.flight_plan and self.flight_plan.plan is not plan:
                self.flight_plan.release()
                self.flight_plan = None

            if plan:
                if not self.flight_plan:
                    self.flight_plan = FlightPlanItem(plan, self.scene, QColor("Green"))
                self.flight_plan.update(ship, now)
        return True


class TacticalMap(AdvancedViewer):
    def __init__(self,
                 tactical_core: TacticalCore,
                 area_size=10000000,
                 *args, **kwargs):
        self.scene = QGraphicsScene(-area_size, -area_size, 2 * area_size, 2 * area_size)
        self.scene.setItemIndexMethod(QGraphicsScene.NoIndex)
        super().__init__(self.scene, *args, **kwargs)

        self.tactical_core = tactical_core
        self.world = self.tactical_core.world
        self.player = self.tactical_core.player
        self.ships: Dict[str, ShipItem] = {}
        self.asteroids: Dict[int, MapItem] = {}
        self.markers: Set[QGraphicsRectItem] = set()

        self.logger = logging.getLogger("TacticalMap")

    def center_on(self, x: float, y: float):
        self.centerOn(x, y)

    def move_to_ship(self, ship_name: str, increase_scale: bool = True):
        """Center tactical map on the ship with the specified 'ship'.
        If the ship is moving too fast and may leave the visible area in less
        than a 5 seconds, the scale map scale may be increased. Set the
        'increase_scale' to 'False' to avoid this behavior."""
        ship: Optional["Ship"] = None
        try:
            ship = self.player.ships[ship_name]
        except KeyError:
            self.logger.warning(f"Can't center on ship '{ship_name}': no such ship!")

        if ship:
            position = ship.predict_position()
            if position:
                self.centerOn(position.x, position.y)
            else:
                self.logger.warning(f"Can't center on ship '{ship_name}': "
                                    f"can't predict position!")

    def update(self):
        self._update_ships(self.tactical_core.time.predict_usec())
        self._update_asteroids(self.tactical_core.time.predict_usec())

    def _update_ships(self, now: int):
        for ship in self.player.ships.values():
            try:
                item = self.ships[ship.name]
            except KeyError:
                item = ShipItem(scene=self.scene,
                                ship_size=100,
                                ship_color=QColor("red"))
                item.spawn()
                item.mark()
                self.ships[ship.name] = item
            item.update(ship, now)

    def _update_asteroids(self, now: int):
        for asteroid in self.world.asteroids.values():
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
            position: Optional[Position] = asteroid.position.predict(at=now)

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
