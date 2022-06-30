from typing import List, Union, Dict, Any, Optional
import random
from expansion import types, modules


class Item:
    Key = (types.ObjectType, Union[str, int])

    def __init__(self,
                 object_type: types.ObjectType,
                 object_id: Union[str, int]):
        self.type: types.ObjectType = object_type
        self.id: Union[str, int] = object_id
        self.position: Optional[types.Position] = None
        self.radius: Optional[float] = None
        self.updated_at: int = 0
        self._next_forced_update_at: int = 0

    def to_pod(self) -> Dict[str, Any]:
        data = {
            "type": self.type.value,
            "id": self.id,
            "ts": self.updated_at,
        }
        if self.position:
            data.update({"pos": self.position.to_pod()})
        if self.radius:
            data.update({"radius": self.radius})
        return data

    def key_pair(self) -> "Key":
        return self.type, self.id

    def is_ship(self) -> bool:
        return self.type == types.ObjectType.SHIP

    def is_asteroid(self) -> bool:
        return self.type == types.ObjectType.ASTEROID

    # Return true if object has been updated actually
    def update(self,
               now: int,
               *,
               ship: Optional[modules.Ship] = None,
               asteroid: Optional[types.PhysicalObject] = None) -> bool:
        forced_update = self._needs_force_update(now)
        if ship:
            assert self.is_ship()
            has_updates = self._update_as_ship(ship, forced_update)
        elif asteroid:
            assert self.is_asteroid()
            has_updates = self._update_as_asteroid(asteroid, forced_update)
        else:
            assert False, "Nothing is passed to update"

        self.updated_at = now
        if forced_update:
            self._schedule_next_forced_update(now)
        return has_updates or forced_update

    def _update_as_ship(self, ship: modules.Ship, forced_update: bool) -> bool:
        assert self.id == ship.name
        has_updates = False
        if ship.position:
            well_predicted = ship.position.well_predicated_by(self.position)
            if not well_predicted or forced_update:
                has_updates = True
                self.position = ship.position
        return has_updates

    def _update_as_asteroid(self, asteroid: types.PhysicalObject,
                            forced_update: bool) -> bool:
        assert self.id == asteroid.object_id
        has_updates = False
        if asteroid.position:
            well_predicted = asteroid.position.well_predicated_by(self.position)
            if not well_predicted or forced_update:
                has_updates = True
                self.position = asteroid.position
        self.radius = asteroid.radius
        return has_updates

    def _needs_force_update(self, now: int) -> bool:
        return self._next_forced_update_at <= now

    def _schedule_next_forced_update(self, now: int):
        # Force update needs to be done every 1-2 seconds, but only if
        # this object has been updated not really
        one_sec = 10**6
        two_sec = 2 * one_sec
        self._next_forced_update_at = now + random.randint(one_sec, two_sec)


class Update:
    def __init__(self):
        self.timestamp: int = 0
        self.items: List[Item] = []

    def to_pod(self):
        return {
            "ts": self.timestamp,
            "items": [item.to_pod() for item in self.items]
        }