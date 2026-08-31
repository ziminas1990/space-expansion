import * as lowlevel from "../lowlevel/index.js";
import type {
    ObjectType,
    PhysicalObject,
    Position,
} from "../types/index.js";
import { Status } from "../types/status.js";

export type AdministratorClockStatus = lowlevel.AdministratorClockStatus;
export type SpawnComposition = lowlevel.SpawnComposition;
export type SpawnStatus = lowlevel.SpawnStatus;
export type ManipulatorStatus = lowlevel.ManipulatorStatus;

export class Administrator {
    readonly clock: AdministratorClock;
    readonly spawner: Spawner;
    readonly manipulator: BasicManipulator;

    constructor(private readonly rpc: lowlevel.Administrator) {
        this.clock = new AdministratorClock(rpc.clock);
        this.spawner = new Spawner(rpc.spawner);
        this.manipulator = new BasicManipulator(rpc.manipulator);
    }

    down_level(): lowlevel.Administrator {
        return this.rpc;
    }

    async close(): Promise<Status> {
        return await this.rpc.close();
    }
}

export class AdministratorClock {
    constructor(private readonly rpc: lowlevel.AdministratorClock) {}

    async get_time(timeout_ms = 500): Promise<[Status, bigint | undefined]> {
        const send_status = await this.rpc.send_time_request();
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send time request"), undefined];
        }
        const [status, time] = await this.rpc.wait_now(timeout_ms);
        if (!status.is_ok() || time === undefined) {
            return [status.wrap("failed to get clock time"), undefined];
        }
        return [Status.ok(), time];
    }

    async get_mode(timeout_ms = 500)
        : Promise<[Status, AdministratorClockStatus | undefined]>
    {
        const send_status = await this.rpc.send_mode_request();
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send mode request"), undefined];
        }
        return await this.wait_status(timeout_ms);
    }

    async switch_to_real_time(timeout_ms = 500): Promise<Status> {
        const send_status = await this.rpc.send_switch_to_real_time();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to switch to real time");
        }
        return await this.expect_status("MODE_REAL_TIME", timeout_ms);
    }

    async switch_to_debug_mode(timeout_ms = 500): Promise<Status> {
        const send_status = await this.rpc.send_switch_to_debug_mode();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to switch to debug mode");
        }
        return await this.expect_status("MODE_DEBUG", timeout_ms);
    }

    async terminate(timeout_ms = 500): Promise<Status> {
        const send_status = await this.rpc.send_terminate();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to terminate clock");
        }
        return await this.expect_status("MODE_TERMINATED", timeout_ms);
    }

    async set_tick_duration(duration_us: number, timeout_ms = 500): Promise<Status> {
        const send_status = await this.rpc.send_tick_duration(duration_us);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to set tick duration");
        }
        return await this.expect_status("MODE_DEBUG", timeout_ms);
    }

    async proceed_ticks(ticks: number, timeout_ms: number)
        : Promise<[Status, bigint | undefined]>
    {
        const send_status = await this.rpc.send_proceed_ticks(ticks);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to proceed ticks"), undefined];
        }
        const [status, time] = await this.rpc.wait_now(timeout_ms);
        if (!status.is_ok() || time === undefined) {
            return [status.wrap("failed to proceed ticks"), undefined];
        }
        return [Status.ok(), time];
    }

    private async wait_status(timeout_ms: number)
        : Promise<[Status, AdministratorClockStatus | undefined]>
    {
        const [status, mode] = await this.rpc.wait_status(timeout_ms);
        if (!status.is_ok() || mode === undefined) {
            return [status.wrap("failed to get clock status"), undefined];
        }
        return [Status.ok(), mode];
    }

    private async expect_status(
        expected: AdministratorClockStatus,
        timeout_ms: number,
    ): Promise<Status> {
        const [status, mode] = await this.wait_status(timeout_ms);
        if (!status.is_ok() || mode === undefined) {
            return status;
        }
        if (mode !== expected) {
            return Status.fail(`expected clock status ${expected}, got ${mode}`);
        }
        return Status.ok();
    }
}

export class Spawner {
    constructor(private readonly rpc: lowlevel.Spawner) {}

    async spawn_asteroid(
        position: Position,
        composition: SpawnComposition,
        radius: number,
        timeout_ms = 1_000,
    ): Promise<[Status, PhysicalObject | undefined]> {
        const send_status = await this.rpc.send_spawn_asteroid(
            position,
            composition,
            radius,
        );
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to spawn asteroid"), undefined];
        }
        const [status, result, timestamp] = await this.rpc.wait_spawn(timeout_ms);
        if (!status.is_ok() || result === undefined) {
            return [status.wrap("failed to spawn asteroid"), undefined];
        }
        if (result.case === "asteroid_id") {
            return [
                Status.ok(),
                {
                    object_type: "asteroid",
                    object_id: result.id,
                    radius,
                    position: { ...position, timestamp },
                },
            ];
        }
        if (result.case === "problem") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.fail("unexpected spawn response"), undefined];
    }

    async spawn_ship(
        player: string,
        blueprint: string,
        ship_name: string,
        position: Position,
        timeout_ms = 1_000,
    ): Promise<[Status, PhysicalObject | undefined]> {
        const send_status = await this.rpc.send_spawn_ship(
            player,
            blueprint,
            ship_name,
            position,
        );
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to spawn ship"), undefined];
        }
        const [status, result, timestamp] = await this.rpc.wait_spawn(timeout_ms);
        if (!status.is_ok() || result === undefined) {
            return [status.wrap("failed to spawn ship"), undefined];
        }
        if (result.case === "ship_id") {
            return [
                Status.ok(),
                {
                    object_type: "ship",
                    object_id: result.id,
                    radius: 0,
                    position: { ...position, timestamp },
                },
            ];
        }
        if (result.case === "problem") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.fail("unexpected spawn response"), undefined];
    }
}

export class BasicManipulator {
    constructor(private readonly rpc: lowlevel.BasicManipulator) {}

    async get_object(
        object_type: ObjectType,
        object_id: number,
        timeout_ms = 1_000,
    ): Promise<[Status, PhysicalObject | undefined]> {
        const send_status = await this.rpc.send_object_request(
            object_type,
            object_id,
        );
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to request object"), undefined];
        }
        const [status, result] = await this.rpc.wait_manipulator(timeout_ms);
        if (!status.is_ok() || result === undefined) {
            return [status.wrap("failed to get object"), undefined];
        }
        if (result.case === "object") {
            return [Status.ok(), result.object];
        }
        if (result.case === "problem") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.fail("unexpected manipulator response"), undefined];
    }

    async move_object(
        object_type: ObjectType,
        object_id: number,
        position: Position,
        timeout_ms = 1_000,
    ): Promise<[Status, Position | undefined]> {
        const send_status = await this.rpc.send_move(
            object_type,
            object_id,
            position,
        );
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to move object"), undefined];
        }
        const [status, result] = await this.rpc.wait_manipulator(timeout_ms);
        if (!status.is_ok() || result === undefined) {
            return [status.wrap("failed to move object"), undefined];
        }
        if (result.case === "moved_at") {
            return [
                Status.ok(),
                { ...position, timestamp: BigInt(result.time) },
            ];
        }
        if (result.case === "problem") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.fail("unexpected manipulator response"), undefined];
    }
}
