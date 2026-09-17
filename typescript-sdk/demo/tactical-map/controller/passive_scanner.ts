import * as midlevel from "@spx/sdk/midlevel";
import { Status } from "@spx/sdk/types";
import type { PhysicalObject } from "@spx/sdk/types";
import { Asteroid } from "../domain/asteroid.js";
import { Ship as DomainShip } from "../domain/ship.js";
import { EntityRef } from "../domain/world.js";
import { convert_position } from "./helpers.js";
import { IWorld } from "./interfaces.js";
import { Logger } from "../log.js";
import { RetryTimeout } from "../utils/retry_timeout.js";

export class PassiveScanner {

    private stopped: boolean = false;
    private monitoring_task?: Promise<void>;

    constructor(
        private readonly remote: midlevel.PassiveScanner,
        private readonly world: IWorld,
        private readonly logger: Logger,
        private readonly name: string,
    ) {}

    async initialize(): Promise<Status> {
        this.monitoring_task = this.monitoring();
        return Status.ok();
    }

    async stop(): Promise<Status> {
        this.stopped = true;
        await this.remote.terminate();
        if (this.monitoring_task) {
            await this.monitoring_task;
            this.monitoring_task = undefined;
        }
        return Status.ok();
    }

    private async monitoring() {
        const retry_timeout = new RetryTimeout([500, 1000, 2000, 5000]);

        while (!this.stopped) {
            const status = await this.remote.monitoring(
                this.handle_update.bind(this),
            );
            if (!this.stopped && !status.is_ok()) {
                this.logger.error(
                    `PassiveScanner '${this.name}' monitoring failed: ${status.what()}`,
                );
                await retry_timeout.wait_to_retry(() => this.stopped);
            } else {
                retry_timeout.reset();
            }
        }
    }

    private async handle_update(objects: PhysicalObject[] | undefined)
    : Promise<boolean>
    {
        if (objects === undefined) {
            return !this.stopped;
        }
        for (const object of objects) {
            this.apply_object(object);
        }
        return !this.stopped;
    }

    private apply_object(object: PhysicalObject): void {
        switch (object.object_type) {
            case "asteroid":
                this.apply_asteroid(object);
                break;
            case "ship":
                this.apply_ship(object);
                break;
            default:
                break;
        }
    }

    private apply_asteroid(object: PhysicalObject): void {
        const id = String(object.object_id);
        const position = convert_position(object.position);
        const entity: EntityRef = { kind: "asteroid", id };
        if (this.world.has_entity(entity)) {
            this.world.update({
                type: "asteroid_update",
                asteroid_id: id,
                update: { position, radius: object.radius },
            });
        } else {
            this.world.update({
                type: "add_asteroid",
                asteroid: new Asteroid(id, position, object.radius),
            });
        }
    }

    private apply_ship(object: PhysicalObject): void {
        const id = String(object.object_id);
        const position = convert_position(object.position);
        const ship: EntityRef = { kind: "ship", id };
        if (this.world.has_entity(ship)) {
            this.world.update({
                type: "ship_update",
                ship_id: id,
                update: { position },
            });
        } else {
            this.world.update({
                type: "add_ship",
                ship: new DomainShip(id, position),
            });
        }
    }
}
