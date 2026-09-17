import { Clock } from "@spx/sdk/utils";
import { Logger } from "../log.js";
import { Asteroid, AsteroidUpdate } from "./asteroid.js";
import { Ship, ShipUpdate } from "./ship.js";

const DEFAULT_OUTDATED_AFTER_US = 60_000_000;

export type EntityRef = {
    kind: "asteroid" | "ship";
    id: string;
}

export type WorldUpdate =
    | { type: "add_asteroid", asteroid: Asteroid }
    | { type: "add_ship", ship: Ship }
    | { type: "asteroid_update", asteroid_id: string, update: AsteroidUpdate }
    | { type: "ship_update", ship_id: string, update: ShipUpdate }
    | { type: "remove_entity", entity: EntityRef }

export class World {

    private readonly asteroids: Map<string, Asteroid> = new Map();
    private readonly ships: Map<string, Ship> = new Map();
    private readonly clock = new Clock();

    constructor(
        private journal: Logger,
        private outdated_after_us: number = DEFAULT_OUTDATED_AFTER_US,
    ) {}

    has_entity(entity: EntityRef): boolean {
        switch (entity.kind) {
            case "asteroid":
                return this.asteroids.has(entity.id);
            case "ship":
                return this.ships.has(entity.id);
            default:
                return false;
        }
    }

    update(update: WorldUpdate): void {
        switch (update.type) {
            case "add_asteroid":
                this.journal.info(`Asteroid ${update.asteroid.get_id()} added`);
                this.asteroids.set(update.asteroid.get_id(), update.asteroid);
                break;
            case "add_ship":
                this.journal.info(`Ship ${update.ship.get_id()} added`);
                this.ships.set(update.ship.get_id(), update.ship);
                break;
            case "asteroid_update":
                this.asteroids.get(update.asteroid_id)?.update(update.update);
                break;
            case "ship_update":
                this.ships.get(update.ship_id)?.update(update.update);
                break;
            case "remove_entity":
                this.journal.info(`Entity ${update.entity.id} removed`);
                switch (update.entity.kind) {
                    case "asteroid":
                        this.asteroids.delete(update.entity.id);
                        break;
                    case "ship":
                        this.ships.delete(update.entity.id);
                        break;
                }
                break;
            default:
                throw new Error(`Unknown update type`);
        }
        this.tick();
    }

    observe(local_ts: number, server_ts: number, ingame_ts: number): void {
        this.clock.observe(local_ts, server_ts, ingame_ts);
        this.tick();
    }

    private tick(): void {
        const now = this.clock.monotonic_now();
        if (now === undefined) {
            return;
        }
        for (const asteroid of this.asteroids.values()) {
            this.age_one(asteroid, "Asteroid", now);
        }
        for (const ship of this.ships.values()) {
            this.age_one(ship, "Ship", now);
        }
    }

    private age_one(entity: Asteroid | Ship, label: string, now_us: number): void {
        const age = now_us - entity.get_position().timestamp;
        const stale = age > this.outdated_after_us;
        if (stale === entity.outdated) {
            return;
        }
        entity.outdated = stale;
        if (stale) {
            this.journal.info(`${label} ${entity.get_id()} outdated`);
        } else {
            this.journal.info(`${label} ${entity.get_id()} is current again`);
        }
    }

}
