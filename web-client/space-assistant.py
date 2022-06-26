import asyncio
from typing import Optional, List, Dict, Any, Union
import json
import random

from quart import Quart, render_template, websocket, url_for

from expansion.procedures import login
from expansion import modules
from expansion import types

app = Quart(__name__)


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


class Client:
    def __init__(self):
        self.commutator: Optional[modules.Commutator] = None
        self.system_clock: Optional[modules.SystemClock] = None
        self.ships: List[modules.Ship] = []
        self.asteroids: Dict[int, types.PhysicalObject] = {}

        self._async_tasks: List[Any] = []
        self.now: int = 0

    def connected(self) -> bool:
        return self.commutator is not None

    async def login(self) -> Optional[str]:
        self.commutator, problem = await login(
            server_ip="127.0.0.1",
            login_port=6842,
            login="Olenoid",
            password="admin")
        if problem:
            return problem
        success = await self.commutator.init()
        return None if success else "Failed to initialize commutator"

    async def initialize(self) -> Optional[str]:
        self.system_clock = modules.SystemClock.find(self.commutator)
        if self.system_clock is None:
            return "Failed to instantiate system clock"
        self._async_tasks.append(
            asyncio.create_task(self._monitor_time()))

        # Export all ships and start monitoring for them
        for ship in modules.Ship.get_all_ships(self.commutator):
            await self.on_new_ship(ship)
        return None

    async def on_new_ship(self, ship: modules.Ship):
        ship.start_monitoring()  # automatically updates it's state
        self.ships.append(ship)
        # if ship has a passive scanner, attach to it
        scanner = await modules.PassiveScanner.get_most_ranged(ship)
        if scanner:
            self._async_tasks.append(
                asyncio.create_task(self._scanning_task(scanner)))

    async def _scanning_task(self, passive_scaner: modules.PassiveScanner):
        while True:
            async for item in passive_scaner.scan():
                # Assert for type hinting
                assert isinstance(item, types.PhysicalObject)
                if item.object_type == types.ObjectType.ASTEROID:
                    self.asteroids.update({item.object_id: item})
            # Something went wrong. Try to start scanning in a second
            await asyncio.sleep(1)

    async def _monitor_time(self):
        while True:
            async for time in self.system_clock.monitor(interval_ms=25):
                self.now = time
            # Something went wrong. Try one more time in 0.25 seconds
            await asyncio.sleep(0.25)


client: Client = Client()


async def stream_updates(socket):

    sent_updates: Dict[Item.Key, Item] = {}
    while True:
        await asyncio.sleep(0.1)
        if not client.connected():
            continue
        now = client.now
        random.seed()
        update = Update()
        update.timestamp = now
        # Collect ship updates
        for ship in client.ships:
            item_key = (types.ObjectType.SHIP, ship.name)
            item = sent_updates.setdefault(item_key, Item(*item_key))
            # Check if update is required
            if item.update(ship=ship, now=now):
                update.items.append(item)

        for asteroid in client.asteroids.values():
            assert isinstance(asteroid, types.PhysicalObject)
            item_key = (types.ObjectType.ASTEROID, asteroid.object_id)
            item = sent_updates.setdefault(item_key, Item(*item_key))
            # Check if update is required
            if item.update(asteroid=asteroid, now=now):
                update.items.append(item)

        await socket.send(json.dumps(update.to_pod()))


@app.route("/")
async def main():
    if not client.connected():
        error = await client.login()
        if error:
            return f"<p>Failed to connect to server: {error}"
        error = await client.initialize()
        if error:
            return f"<p>Failed to initialize a client: {error}"
    return await render_template(
        "index.html",
        items_container_js=url_for('static', filename='ItemsContainer.js'),
        stage_view_js=url_for('static', filename='StageView.js'),
        scene_js=url_for('static', filename='Scene.js'),
        main_js=url_for('static', filename='main.js')
    )


@app.websocket('/')
async def ws():
    while True:
        await websocket.accept()
        await stream_updates(websocket)
