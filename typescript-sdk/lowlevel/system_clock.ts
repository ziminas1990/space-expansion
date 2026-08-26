import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export class SystemClock {

    constructor(private session: Session) {}

    async send_time_request(): Promise<types.Status> {
        const request = create(msg.ISystemClockSchema, {
            choice: { case: "timeReq", value: true },
        });
        return this.send(request);
    }

    async wait_time(timeout: number = 500)
    : Promise<[types.Status, types.ServerTimestamp | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "time") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            real_us: types.asUint64(response.choice.value),
            ingame_us: timestamp,
        }];
    }

    async send_wait_until(time_us: bigint): Promise<types.Status> {
        const request = create(msg.ISystemClockSchema, {
            choice: { case: "waitUntil", value: time_us },
        });
        return this.send(request);
    }

    async send_wait_for(period_us: bigint): Promise<types.Status> {
        const request = create(msg.ISystemClockSchema, {
            choice: { case: "waitFor", value: period_us },
        });
        return this.send(request);
    }

    async wait_ring(timeout: number = 500)
    : Promise<[types.Status, types.ServerTimestamp | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "ring") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            real_us: types.asUint64(response.choice.value),
            ingame_us: timestamp,
        }];
    }

    async send_monitor_request(interval_ms: number): Promise<types.Status> {
        const request = create(msg.ISystemClockSchema, {
            choice: { case: "monitor", value: interval_ms },
        });
        return this.send(request);
    }

    private async send(request: msg.ISystemClock): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "systemClock", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.ISystemClock | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "systemClock") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, types.asUint64(response.timestamp)];
    }

}
