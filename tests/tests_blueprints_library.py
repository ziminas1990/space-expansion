from typing import Dict, List, Optional

from base_test_fixture import BaseTestFixture
import server.configurator.blueprints as blueprints
import server.configurator.world as world

from server.configurator.modules import default_ships
from server.configurator.configuration import Configuration
from server.configurator.general import General, ApplicationMode

from expansion import modules
from expansion.interfaces.rpc import BlueprintsLibraryStatus
from expansion.types import Blueprint


class TestCase(BaseTestFixture):

    def __init__(self, *args, **kwargs):
        super(TestCase, self).__init__(*args, **kwargs)

        self.configuration = Configuration(
            general=General(total_threads=1,
                            login_udp_port=7456,
                            initial_state=ApplicationMode.e_FREEZE,
                            ports_pool=(12000, 12100)),
            blueprints=blueprints.DefaultBlueprints(),
            players={
                'player': world.Player(
                    login="player",
                    password="secret",
                    ships=[
                        default_ships.make_miner(
                            name="miner",
                            position=world.Position(
                                x=0, y=0, velocity=world.Vector(0, 0))),
                    ]
                )
            },
            world=world.World(),
        )

    def get_configuration(self) -> Configuration:
        return self.configuration

    @BaseTestFixture.run_as_sync
    async def test_get_blueprints_list(self):
        db: blueprints.BlueprintsDB = blueprints.DefaultBlueprints()

        await self.system_clock_fast_forward(speed_multiplier=10)
        commutator, error = await self.login('player', "127.0.0.1", "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        library: Optional[modules.BlueprintsLibrary] = \
            modules.BlueprintsLibrary.find(commutator)
        self.assertIsNotNone(library)

        # Get all blueprints
        status, blueprints_list = await library.get_blueprints_list()
        self.assertEqual(status, BlueprintsLibraryStatus.SUCCESS)
        self.assertIsNotNone(blueprints_list)

        # Check that all blueprints from DB exist in response
        type_to_names: Dict[str, List[str]] = {}
        for blueprint_id in db.blueprints:
            full_name = f"{blueprint_id.type.value}/{blueprint_id.name}"
            self.assertIn(full_name, blueprints_list)
            type_to_names\
                .setdefault(blueprint_id.type.value, [])\
                .append(full_name)

        # Get blueprints by type:
        for blueprint_type, names in type_to_names.items():
            status, blueprints_list = await library.get_blueprints_list(
                start_with=blueprint_type + "/"
            )
            self.assertEqual(status, BlueprintsLibraryStatus.SUCCESS)
            self.assertIsNotNone(blueprints_list)
            self.assertEqual(set(names), set(blueprints_list))

    @BaseTestFixture.run_as_sync
    async def test_get_blueprint(self):
        db: blueprints.BlueprintsDB = blueprints.DefaultBlueprints()

        await self.system_clock_fast_forward(speed_multiplier=10)
        commutator, error = await self.login('player', "127.0.0.1", "127.0.0.1")
        self.assertIsNotNone(commutator)
        self.assertIsNone(error)

        library: Optional[modules.BlueprintsLibrary] = \
            modules.BlueprintsLibrary.find(commutator)
        self.assertIsNotNone(library)

        # Get blueprints list
        status, blueprints_list = await library.get_blueprints_list()
        self.assertEqual(status, BlueprintsLibraryStatus.SUCCESS)
        self.assertIsNotNone(blueprints_list)
        # Get each blueprint
        for blueprint_name in blueprints_list:
            status, blueprint_data = await library.get_blueprint(blueprint_name)
            self.assertIsNotNone(blueprint_data)
