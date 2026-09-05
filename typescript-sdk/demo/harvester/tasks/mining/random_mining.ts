import { ModuleType, type SystemClock } from "../../../../highlevel/index.js";
import type { PhysicalObject } from "../../../../types/index.js";
import type { Ship } from "../../ship.js";
import type { TacticalCore } from "../../tactical_core.js";
import { distance_to, sleep } from "../../util.js";
import { BaseTask } from "../base_task.js";
import { SimpleMining } from "./simple_mining.js";

export class RandomMining extends BaseTask {
    private readonly tactical_core: TacticalCore;
    private readonly warehouse: Ship;
    private readonly _tasks = new Map<Ship, SimpleMining | undefined>();

    constructor(
        name: string,
        tactical_core: TacticalCore,
        warehouse: Ship,
        system_clock: SystemClock,
    ) {
        super(name, system_clock);
        this.tactical_core = tactical_core;
        this.warehouse = warehouse;
    }

    static can_use_ship(candidate: Ship): boolean {
        return candidate.has_modules([
            ModuleType.RESOURCE_CONTAINER,
            ModuleType.ASTEROID_MINER,
            ModuleType.ENGINE,
        ]);
    }

    add_ship(miner: Ship): void {
        if (!this._tasks.has(miner)) {
            this._tasks.set(miner, undefined);
        }
    }

    override interrupt(): void {
        for (const task of this._tasks.values()) {
            task?.interrupt();
        }
        super.interrupt();
    }

    protected async _impl(): Promise<boolean> {
        const cycle = 1;
        while (!this.signal?.aborted) {
            for (const [miner, current] of [...this._tasks.entries()]) {
                if (current && !current.finished) {
                    continue;
                }
                const asteroid = this._choose_random_asteroid();
                if (!asteroid) {
                    break;
                }
                this.add_journal_record(
                    `Asteroid ${asteroid.object_id} is chosen for '${miner.name}'`,
                );
                const task = new SimpleMining(
                    `${this.name}.${miner.name}.turn_${cycle}`,
                    this.tactical_core,
                    miner,
                    asteroid.object_id,
                    this.warehouse,
                    this.system_clock,
                );
                this._tasks.set(miner, task);
                task.run_async();
            }
            await sleep(500, this.signal);
        }
        return true;
    }

    private _choose_random_asteroid(): PhysicalObject | undefined {
        const asteroids = [...this.tactical_core.world.asteroids.values()];
        if (asteroids.length === 0) {
            return undefined;
        }
        const attempts = Math.max(1, Math.floor(Math.log2(asteroids.length)) + 1);
        const position = this.warehouse.remote.predict_position(
            this.system_clock.now_us(),
        );

        let best = asteroids[Math.floor(Math.random() * asteroids.length)];
        if (!best) {
            return undefined;
        }
        for (let attempt = 0; attempt < attempts; attempt += 1) {
            const asteroid = asteroids[Math.floor(Math.random() * asteroids.length)];
            if (!asteroid || !position || !best) {
                continue;
            }
            if (distance_to(position, asteroid.position)
                < distance_to(position, best.position))
            {
                best = asteroid;
            }
        }
        return best;
    }
}
