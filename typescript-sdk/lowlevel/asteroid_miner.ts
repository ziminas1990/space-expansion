import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";


export type AsteroidMinerStatus =
    | "SUCCESS"
    | "INTERNAL_ERROR"
    | "ASTEROID_DOESNT_EXIST"
    | "MINER_IS_BUSY"
    | "MINER_IS_IDLE"
    | "ASTEROID_TOO_FAR"
    | "NO_SPACE_AVAILABLE"
    | "NOT_BOUND_TO_CARGO"
    | "INTERRUPTED_BY_USER";

export type AsteroidMinerSpecification = {
    max_distance: number;
    cycle_time_ms: number;
    yield_per_cycle: number;
}

export type AsteroidMinerMiningEvent =
    | { case: "mining_report"; resources: types.ResourceItem[] }
    | { case: "mining_is_stopped"; status: AsteroidMinerStatus };

export class AsteroidMiner {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IAsteroidMinerSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, AsteroidMinerSpecification | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "specification") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const spec = response.choice.value;
        return [types.Status.ok(), {
            max_distance: spec.maxDistance,
            cycle_time_ms: spec.cycleTimeMs,
            yield_per_cycle: spec.yieldPerCycle,
        }];
    }

    async send_bind_to_cargo(cargo_name: string): Promise<types.Status> {
        const request = create(msg.IAsteroidMinerSchema, {
            choice: { case: "bindToCargo", value: cargo_name },
        });
        return this.send(request);
    }

    async wait_bind_to_cargo_status(timeout: number = 500)
    : Promise<[types.Status, AsteroidMinerStatus | undefined]>
    {
        return this.wait_status("bindToCargoStatus", timeout);
    }

    async send_start_mining(asteroid_id: number): Promise<types.Status> {
        const request = create(msg.IAsteroidMinerSchema, {
            choice: { case: "startMining", value: asteroid_id },
        });
        return this.send(request);
    }

    async wait_start_mining_status(timeout: number = 500)
    : Promise<[types.Status, AsteroidMinerStatus | undefined]>
    {
        return this.wait_status("startMiningStatus", timeout);
    }

    async send_stop_mining(): Promise<types.Status> {
        const request = create(msg.IAsteroidMinerSchema, {
            choice: { case: "stopMining", value: true },
        });
        return this.send(request);
    }

    async wait_stop_mining_status(timeout: number = 500)
    : Promise<[types.Status, AsteroidMinerStatus | undefined]>
    {
        return this.wait_status("stopMiningStatus", timeout);
    }

    async wait_mining_event(timeout: number = 1000)
    : Promise<[types.Status, AsteroidMinerMiningEvent | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "miningIsStopped") {
            const server_status = asteroidMinerStatusFromProtobuf(response.choice.value);
            if (!server_status) {
                return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "mining_is_stopped",
                status: server_status,
            }];
        }
        if (response.choice.case != "miningReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            case: "mining_report",
            resources: types.resourceItemsFromProtobuf(response.choice.value.items),
        }];
    }

    private async wait_status(
        expected_case: "bindToCargoStatus" | "startMiningStatus" | "stopMiningStatus",
        timeout: number)
    : Promise<[types.Status, AsteroidMinerStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != expected_case) {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const server_status = asteroidMinerStatusFromProtobuf(response.choice.value);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                    undefined];
        }
        return [types.Status.ok(), server_status];
    }

    private async send(request: msg.IAsteroidMiner): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "asteroidMiner", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IAsteroidMiner | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "asteroidMiner") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}

function asteroidMinerStatusFromProtobuf(
    value: msg.IAsteroidMiner_Status): AsteroidMinerStatus | undefined
{
    switch (value) {
        case msg.IAsteroidMiner_Status.SUCCESS: return "SUCCESS";
        case msg.IAsteroidMiner_Status.INTERNAL_ERROR: return "INTERNAL_ERROR";
        case msg.IAsteroidMiner_Status.ASTEROID_DOESNT_EXIST: return "ASTEROID_DOESNT_EXIST";
        case msg.IAsteroidMiner_Status.MINER_IS_BUSY: return "MINER_IS_BUSY";
        case msg.IAsteroidMiner_Status.MINER_IS_IDLE: return "MINER_IS_IDLE";
        case msg.IAsteroidMiner_Status.ASTEROID_TOO_FAR: return "ASTEROID_TOO_FAR";
        case msg.IAsteroidMiner_Status.NO_SPACE_AVAILABLE: return "NO_SPACE_AVAILABLE";
        case msg.IAsteroidMiner_Status.NOT_BOUND_TO_CARGO: return "NOT_BOUND_TO_CARGO";
        case msg.IAsteroidMiner_Status.INTERRUPTED_BY_USER: return "INTERRUPTED_BY_USER";
        default: return undefined;
    }
}
