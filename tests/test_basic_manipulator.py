from typing import List

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode
from randomizer import Randomizer
import expansion.types as types


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_RUN,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            world=world.World(),
            players={}
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_objects(self):
        # Spawn a number of asteroids
        asteroids: List[types.PhysicalObject] = []
        randomizer = Randomizer(2132)
        spawner = self.administrator.spawner
        manipulator = self.administrator.manipulator
        for i in range(500):
            status, spawned_asteroid = await spawner.spawn_asteroid(
                position=randomizer.random_position(
                    rect=types.Rect(-1000, 1000, -1000, 1000),
                    min_speed=0,
                    max_speed=1000
                ),
                composition=types.make_resources(ice=100),
                radius=10
            )
            assert spawned_asteroid is not None
            asteroids.append(spawned_asteroid)

        for spawned_asteroid in asteroids:
            status, asteroid = await manipulator.get_object(
                object_type=spawned_asteroid.object_type,
                object_id=spawned_asteroid.object_id)
            self.assertTrue(status.is_success())
            self.assertIsNotNone(asteroid)
            predicated_position = spawned_asteroid.position.predict(
                at=asteroid.position.timestamp.usec())
            self.assertTrue(types.Position.almost_equal(
                predicated_position, asteroid.position
            ))

    @BaseTestFixture.run_as_sync
    async def test_move_objects(self):
        # Spawn a number of asteroids
        asteroids: List[types.PhysicalObject] = []
        randomizer = Randomizer(3254)
        spawner = self.administrator.spawner
        manipulator = self.administrator.manipulator

        for i in range(500):
            status, spawned_asteroid = await spawner.spawn_asteroid(
                position=randomizer.random_position(
                    rect=types.Rect(-1000, 1000, -1000, 1000),
                    min_speed=0,
                    max_speed=1000
                ),
                composition=types.make_resources(ice=100),
                radius=10
            )
            assert spawned_asteroid is not None
            asteroids.append(spawned_asteroid)

        for spawned_asteroid in asteroids:
            status, asteroid = await spawner.spawn_asteroid(
                position=randomizer.random_position(
                    rect=types.Rect(-1000, 1000, -1000, 1000),
                    min_speed=0,
                    max_speed=1000
                ),
                composition=types.make_resources(ice=100),
                radius=10
            )
            self.assertTrue(status.is_success())
            self.assertIsNotNone(asteroid)

            # Get spawned asteroid from server
            status, asteroid = await manipulator.get_object(
                object_type=spawned_asteroid.object_type,
                object_id=spawned_asteroid.object_id)
            self.assertTrue(status.is_success())
            self.assertIsNotNone(asteroid)

            # Move spawned asteroid to it's predicted position after 1 minute
            asteroid.position = asteroid.position.predict(
                asteroid.position.timestamp.usec() + 60 * 10*6
            )
            status, new_position = await manipulator.move_object(
                object_type=asteroid.object_type,
                object_id=asteroid.object_id,
                position=asteroid.position)
            self.assertTrue(status.is_success())
            self.assertIsNotNone(new_position)

            # Get asteroid again and check its position now
            status, asteroid = await manipulator.get_object(
                object_type=spawned_asteroid.object_type,
                object_id=spawned_asteroid.object_id)
            self.assertTrue(status.is_success())
            self.assertIsNotNone(asteroid)

            self.assertTrue(types.Position.almost_equal(
                new_position.predict(at=asteroid.position.timestamp.usec()),
                asteroid.position
            ))
