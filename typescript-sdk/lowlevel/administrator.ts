import { create, fromBinary, toBinary } from "@bufbuild/protobuf";
import * as common from "#sdk/CommonTypes_pb.js";
import * as admin from "#sdk/Privileged_pb.js";
import * as transport from "#sdk/transport/index.js";
import {
    objectTypeToProtobuf,
    physicalObjectFromProtobuf,
    positionToProtobuf,
    resourceItemToProtobuf,
    type ObjectType,
    type PhysicalObject,
    type Position,
    type ResourceItem,
    type ResourceType,
} from "#sdk/types/index.js";
import { Status } from "#sdk/types/status.js";

export type SpawnComposition =
    | readonly ResourceItem[]
    | Partial<Record<ResourceType, number>>;

export type AdministratorClockStatus =
    | "MODE_REAL_TIME"
    | "MODE_DEBUG"
    | "MODE_TERMINATED"
    | "CLOCK_IS_BUSY"
    | "INTERNAL_ERROR";

export type SpawnStatus =
    | "SUCCESS"
    | "PLAYER_DOESNT_EXIST"
    | "BLUEPRINT_DOESNT_EXIST"
    | "NOT_A_SHIP_BLUEPRINT"
    | "CANT_SPAWN_SHIP";

export type SpawnResult =
    | { case: "asteroid_id"; id: number }
    | { case: "ship_id"; id: number }
    | { case: "problem"; status: SpawnStatus };

export type ManipulatorStatus = "OBJECT_DOESNT_EXIST";

export type ManipulatorResult =
    | { case: "object"; object: PhysicalObject }
    | { case: "moved_at"; time: number }
    | { case: "problem"; status: ManipulatorStatus };

export function privilegedDecoder(): transport.Decoder<admin.Message, Uint8Array> {
    return new transport.Decoder(
        (data) => fromBinary(admin.MessageSchema, data),
        (value) => toBinary(admin.MessageSchema, value),
    );
}

export class Administrator implements transport.ITerminal<admin.Message> {
    readonly clock: AdministratorClock;
    readonly spawner: Spawner;
    readonly manipulator: BasicManipulator;

    private readonly clock_queue = new transport.Endpoint<admin.Message>();
    private readonly spawn_queue = new transport.Endpoint<admin.Message>();
    private readonly manipulator_queue = new transport.Endpoint<admin.Message>();
    private closed = false;

    constructor(
        private readonly channel: transport.IChannel<admin.Message>,
        private readonly token: bigint,
    ) {
        this.clock = new AdministratorClock(this);
        this.spawner = new Spawner(this);
        this.manipulator = new BasicManipulator(this);
    }

    async on_message(message: admin.Message): Promise<void> {
        switch (message.choice.case) {
            case "systemClock":
                await this.clock_queue.on_message(message);
                return;
            case "spawn":
                await this.spawn_queue.on_message(message);
                return;
            case "manipulator":
                await this.manipulator_queue.on_message(message);
                return;
            default:
                return;
        }
    }

    async on_closed(): Promise<void> {
        this.closed = true;
        await Promise.all([
            this.clock_queue.on_closed(),
            this.spawn_queue.on_closed(),
            this.manipulator_queue.on_closed(),
        ]);
    }

    async close(): Promise<Status> {
        if (this.closed) {
            return Status.ok();
        }
        this.closed = true;
        return await this.channel.close();
    }

    async send(choice: admin.Message["choice"]): Promise<Status> {
        if (this.closed) {
            return Status.closed("administrator connection is closed");
        }
        return await this.channel.send(create(admin.MessageSchema, {
            token: this.token,
            choice,
        }));
    }

    async wait_clock(timeout_ms = 500)
        : Promise<[Status, admin.SystemClock | undefined]>
    {
        const [status, message] = await this.clock_queue.wait(timeout_ms);
        if (!status.is_ok() || message === undefined) {
            return [status.wrap("no clock response"), undefined];
        }
        if (message.choice.case !== "systemClock") {
            return [
                Status.fail(`unexpected response type ${message.choice.case}`),
                undefined,
            ];
        }
        return [Status.ok(), message.choice.value];
    }

    async wait_spawn(timeout_ms = 500)
        : Promise<[Status, admin.Spawn | undefined, bigint]>
    {
        const [status, message] = await this.spawn_queue.wait(timeout_ms);
        if (!status.is_ok() || message === undefined) {
            return [status.wrap("no spawn response"), undefined, 0n];
        }
        if (message.choice.case !== "spawn") {
            return [
                Status.fail(`unexpected response type ${message.choice.case}`),
                undefined,
                0n,
            ];
        }
        return [Status.ok(), message.choice.value, message.timestamp];
    }

    async wait_manipulator(timeout_ms = 500)
        : Promise<[Status, admin.BasicManipulator | undefined, bigint]>
    {
        const [status, message] = await this.manipulator_queue.wait(timeout_ms);
        if (!status.is_ok() || message === undefined) {
            return [status.wrap("no manipulator response"), undefined, 0n];
        }
        if (message.choice.case !== "manipulator") {
            return [
                Status.fail(`unexpected response type ${message.choice.case}`),
                undefined,
                0n,
            ];
        }
        return [Status.ok(), message.choice.value, message.timestamp];
    }
}

export class AdministratorClock {
    constructor(private readonly administrator: Administrator) {}

    async send_time_request(): Promise<Status> {
        return this.send({ case: "timeReq", value: true });
    }

    async wait_now(timeout_ms = 500): Promise<[Status, bigint | undefined]> {
        const [status, response] = await this.administrator.wait_clock(timeout_ms);
        if (!status.is_ok() || response === undefined) {
            return [status, undefined];
        }
        if (response.choice.case !== "now") {
            return [
                Status.fail(`expected clock time, got '${response.choice.case}'`),
                undefined,
            ];
        }
        return [Status.ok(), response.choice.value];
    }

    async send_mode_request(): Promise<Status> {
        return this.send({ case: "modeReq", value: true });
    }

    async send_switch_to_real_time(): Promise<Status> {
        return this.send({ case: "switchToRealTime", value: true });
    }

    async send_switch_to_debug_mode(): Promise<Status> {
        return this.send({ case: "switchToDebugMode", value: true });
    }

    async send_terminate(): Promise<Status> {
        return this.send({ case: "terminate", value: true });
    }

    async send_tick_duration(duration_us: number): Promise<Status> {
        return this.send({ case: "tickDurationUs", value: duration_us });
    }

    async send_proceed_ticks(ticks: number): Promise<Status> {
        return this.send({ case: "proceedTicks", value: ticks });
    }

    async wait_status(timeout_ms = 500)
        : Promise<[Status, AdministratorClockStatus | undefined]>
    {
        const [status, response] = await this.administrator.wait_clock(timeout_ms);
        if (!status.is_ok() || response === undefined) {
            return [status, undefined];
        }
        if (response.choice.case !== "status") {
            return [
                Status.fail(`expected clock status, got '${response.choice.case}'`),
                undefined,
            ];
        }
        return [Status.ok(), clockStatusFromProtobuf(response.choice.value)];
    }

    private async send(choice: admin.SystemClock["choice"]): Promise<Status> {
        return this.administrator.send({
            case: "systemClock",
            value: create(admin.SystemClockSchema, { choice }),
        });
    }
}

export class Spawner {
    constructor(private readonly administrator: Administrator) {}

    async send_spawn_asteroid(
        position: Position,
        composition: SpawnComposition,
        radius: number,
    ): Promise<Status> {
        return this.administrator.send({
            case: "spawn",
            value: create(admin.SpawnSchema, {
                choice: {
                    case: "asteroid",
                    value: create(admin.Spawn_AsteroidSchema, {
                        position: positionToProtobuf(position),
                        radius,
                        composition: resourcesToProtobuf(composition),
                    }),
                },
            }),
        });
    }

    async send_spawn_ship(
        player: string,
        blueprint: string,
        ship_name: string,
        position: Position,
    ): Promise<Status> {
        return this.administrator.send({
            case: "spawn",
            value: create(admin.SpawnSchema, {
                choice: {
                    case: "ship",
                    value: create(admin.Spawn_ShipSchema, {
                        player,
                        blueprint,
                        shipName: ship_name,
                        position: positionToProtobuf(position),
                    }),
                },
            }),
        });
    }

    async wait_spawn(timeout_ms = 1_000)
        : Promise<[Status, SpawnResult | undefined, bigint]>
    {
        const [status, response, timestamp] =
            await this.administrator.wait_spawn(timeout_ms);
        if (!status.is_ok() || response === undefined) {
            return [status, undefined, timestamp];
        }
        if (response.choice.case === "asteroidId") {
            return [
                Status.ok(),
                { case: "asteroid_id", id: response.choice.value },
                timestamp,
            ];
        }
        if (response.choice.case === "shipId") {
            return [
                Status.ok(),
                { case: "ship_id", id: response.choice.value },
                timestamp,
            ];
        }
        if (response.choice.case === "problem") {
            const problem = spawnStatusFromProtobuf(response.choice.value);
            if (problem === undefined) {
                return [
                    Status.fail("unknown spawn problem"),
                    undefined,
                    timestamp,
                ];
            }
            return [
                Status.ok(),
                { case: "problem", status: problem },
                timestamp,
            ];
        }
        return [
            Status.fail(`unexpected spawn response '${response.choice.case}'`),
            undefined,
            timestamp,
        ];
    }
}

export class BasicManipulator {
    constructor(private readonly administrator: Administrator) {}

    async send_object_request(
        object_type: ObjectType,
        object_id: number,
    ): Promise<Status> {
        return this.administrator.send({
            case: "manipulator",
            value: create(admin.BasicManipulatorSchema, {
                choice: {
                    case: "objectReq",
                    value: create(admin.BasicManipulator_ObjectIdSchema, {
                        objectType: objectTypeToProtobuf(object_type),
                        id: object_id,
                    }),
                },
            }),
        });
    }

    async send_move(
        object_type: ObjectType,
        object_id: number,
        position: Position,
    ): Promise<Status> {
        return this.administrator.send({
            case: "manipulator",
            value: create(admin.BasicManipulatorSchema, {
                choice: {
                    case: "move",
                    value: create(admin.BasicManipulator_MoveSchema, {
                        objectId: create(admin.BasicManipulator_ObjectIdSchema, {
                            objectType: objectTypeToProtobuf(object_type),
                            id: object_id,
                        }),
                        position: positionToProtobuf(position),
                    }),
                },
            }),
        });
    }

    async wait_manipulator(timeout_ms = 1_000)
        : Promise<[Status, ManipulatorResult | undefined]>
    {
        const [status, response, timestamp] =
            await this.administrator.wait_manipulator(timeout_ms);
        if (!status.is_ok() || response === undefined) {
            return [status, undefined];
        }
        if (response.choice.case === "object") {
            return [
                Status.ok(),
                {
                    case: "object",
                    object: physicalObjectFromProtobuf(
                        response.choice.value,
                        timestamp,
                    ),
                },
            ];
        }
        if (response.choice.case === "movedAt") {
            return [
                Status.ok(),
                { case: "moved_at", time: response.choice.value },
            ];
        }
        if (response.choice.case === "problem") {
            const problem = manipulatorStatusFromProtobuf(response.choice.value);
            if (problem === undefined) {
                return [Status.fail("unknown manipulator problem"), undefined];
            }
            return [Status.ok(), { case: "problem", status: problem }];
        }
        return [
            Status.fail(`unexpected manipulator response '${response.choice.case}'`),
            undefined,
        ];
    }
}

function resourcesToProtobuf(composition: SpawnComposition): common.Resources {
    const items = Array.isArray(composition)
        ? composition
        : Object.entries(composition).flatMap(([resource_type, amount]) =>
            amount === undefined
                ? []
                : [{
                    resource_type: resource_type as ResourceType,
                    amount,
                }],
        );
    return create(common.ResourcesSchema, {
        items: items.map(resourceItemToProtobuf),
    });
}

function clockStatusFromProtobuf(
    status: admin.SystemClock_Status,
): AdministratorClockStatus | undefined {
    switch (status) {
        case admin.SystemClock_Status.MODE_REAL_TIME:
            return "MODE_REAL_TIME";
        case admin.SystemClock_Status.MODE_DEBUG:
            return "MODE_DEBUG";
        case admin.SystemClock_Status.MODE_TERMINATED:
            return "MODE_TERMINATED";
        case admin.SystemClock_Status.CLOCK_IS_BUSY:
            return "CLOCK_IS_BUSY";
        case admin.SystemClock_Status.INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        default:
            return undefined;
    }
}

function spawnStatusFromProtobuf(
    status: admin.Spawn_Status,
): SpawnStatus | undefined {
    switch (status) {
        case admin.Spawn_Status.SUCCESS:
            return "SUCCESS";
        case admin.Spawn_Status.PLAYER_DOESNT_EXIST:
            return "PLAYER_DOESNT_EXIST";
        case admin.Spawn_Status.BLUEPRINT_DOESNT_EXIST:
            return "BLUEPRINT_DOESNT_EXIST";
        case admin.Spawn_Status.NOT_A_SHIP_BLUEPRINT:
            return "NOT_A_SHIP_BLUEPRINT";
        case admin.Spawn_Status.CANT_SPAWN_SHIP:
            return "CANT_SPAWN_SHIP";
        default:
            return undefined;
    }
}

function manipulatorStatusFromProtobuf(
    status: admin.BasicManipulator_Status,
): ManipulatorStatus | undefined {
    switch (status) {
        case admin.BasicManipulator_Status.OBJECT_DOESNT_EXIST:
            return "OBJECT_DOESNT_EXIST";
        default:
            return undefined;
    }
}
