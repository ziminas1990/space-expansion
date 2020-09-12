import time

from server.configurator.general import General, ApplicationMode
from server.configurator.blueprints.base_blueprint import BlueprintId
from server.configurator.blueprints.blueprints_db import BlueprintsDB
from server.configurator.modules.engine import EngineBlueprint, Engine
from server.configurator.modules.ship import ShipBlueprint, Ship
from server.configurator.configuration import Configuration
from server.configurator.world.player import Player
from server.configurator.world.geomtery import Position, Vector
from server.configurator.world.world import World
from server.configurator.world.asteroids import Asteroid, Asteroids
from server.configurator.expenses import Expenses, ResourceType
from server.server import Server
import yaml

configuration = Configuration()
configuration.set_general(
    General()
        .set_login_port(4737)
        .set_ports_pool(5000, 6000)
        .set_initial_state(ApplicationMode.e_RUN)
        .set_total_threads(2)
        .add_administrator_interface(udp_port=7675, login="admin", password="admin_secret")
)
configuration.set_blueprints(
    BlueprintsDB()
        .add_blueprint(
            EngineBlueprint("Tiny")
            .set_max_thrust(max_thrust=10000)
            .set_expenses(
                Expenses()
                .set(ResourceType.e_METALS, 100)
                .set(ResourceType.e_LABOR, 200)))
        .add_blueprint(
            ShipBlueprint("Jalopy")
            .set_physical_properties(radius=5, weight=1000)
            .set_expenses(
                Expenses()
                .set(ResourceType.e_METALS, 5000)
                .set(ResourceType.e_LABOR, 3000))
            .add_module(name="Main_Engine", blueprint=BlueprintId.engine("Tiny"))
        )
)
configuration.add_player(
    Player()
    .set_credentials(login="James_Bond", password="agent_007")
    .add_ship(
        name="my_jalopy",
        ship=Ship("Jalopy")
            .set_position(position=Position().set_position(x=100, y=200, velocity=Vector(0, 0)))
            .configure_module(name="Main_Engine",
                              cfg=Engine().set_thrust(thrust=Vector(0, 0)))
    )
)
configuration.set_world(
    World()
    .set_steroids(
        Asteroids()
        .add_asteroid(
            Asteroid(position=Position(x=100000, y=200000), radius=50, composition={
                ResourceType.e_METALS: 15,
                ResourceType.e_ICE: 25,
                ResourceType.e_SILICATES: 5,
            }))
        .add_asteroid(
            Asteroid(position=Position(x=150000, y=180000), radius=30, composition={
                ResourceType.e_METALS: 34,
                ResourceType.e_ICE: 12,
                ResourceType.e_SILICATES: 5,
            }))
        .add_asteroid(
            Asteroid(position=Position(x=-40000, y=-170000), radius=40, composition={
                ResourceType.e_METALS: 7,
                ResourceType.e_ICE: 2,
                ResourceType.e_SILICATES: 37,
            })
        )
    )
)
print(yaml.dump(configuration.to_pod()))

server = Server()
server.run(configuration=configuration)
server.stop()