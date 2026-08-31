import { BlueprintId, BlueprintsDB, ModuleType } from "./blueprints.js";
import { General } from "./general.js";
import { Player, World } from "./world.js";

export interface ConfigurationOptions {
    general: General;
    blueprints: BlueprintsDB;
    world: World;
    players?: readonly Player[] | Readonly<Record<string, Player>>;
}

export class Configuration {
    general: General;
    blueprints: BlueprintsDB;
    world: World;
    readonly players = new Map<string, Player>();

    constructor(options: ConfigurationOptions) {
        this.general = options.general;
        this.blueprints = options.blueprints;
        this.world = options.world;

        const players = Array.isArray(options.players)
            ? options.players.map((player) => [player.login, player] as const)
            : Object.entries(options.players ?? {});
        for (const [login, player] of players) {
            this.addPlayer(player, login);
        }
    }

    setGeneral(general: General): this {
        this.general = general;
        return this;
    }

    setBlueprints(blueprints: BlueprintsDB): this {
        this.blueprints = blueprints;
        return this;
    }

    setWorld(world: World): this {
        this.world = world;
        return this;
    }

    addPlayer(player: Player, key = player.login): this {
        if (this.players.has(key)) {
            throw new Error(`Player '${key}' is already configured`);
        }
        this.players.set(key, player);
        return this;
    }

    verify(): void {
        this.general.verify();
        this.blueprints.verify();
        this.world.verify();

        for (const [login, player] of this.players) {
            if (login.length <= 3) {
                throw new Error("Player configuration key is too short");
            }
            player.verify();

            for (const ship of player.ships.values()) {
                const shipBlueprint = new BlueprintId(
                    ModuleType.Ship,
                    ship.shipType,
                );
                if (!this.blueprints.has(shipBlueprint)) {
                    throw new Error(
                        `Blueprint '${shipBlueprint.toPod()}' does not exist`,
                    );
                }
            }
        }
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            application: this.general.toPod(),
            Blueprints: this.blueprints.toPod(),
            Players: Object.fromEntries(
                [...this.players].map(([login, player]) => [
                    login,
                    player.toPod(),
                ]),
            ),
            World: this.world.toPod(),
        };
    }
}
