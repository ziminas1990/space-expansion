import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type EngineSpecification = {
    max_thrust: number;
}

export type CurrentThrust = {
    x: number;
    y: number;
    thrust: number;
}

export class Engine {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IEngineSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, EngineSpecification | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "specification") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            max_thrust: response.choice.value.maxThrust,
        }];
    }

    async send_thrust_request(): Promise<types.Status> {
        const request = create(msg.IEngineSchema, {
            choice: { case: "thrustReq", value: true },
        });
        return this.send(request);
    }

    async wait_thrust(timeout: number = 500)
    : Promise<[types.Status, CurrentThrust | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "thrust") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const thrust = response.choice.value;
        return [types.Status.ok(), {
            x: thrust.x,
            y: thrust.y,
            thrust: thrust.thrust,
        }];
    }

    async send_change_thrust(
        x: number,
        y: number,
        thrust: number,
        duration_ms: number = 0,
        at?: bigint): Promise<types.Status>
    {
        const request = create(msg.IEngineSchema, {
            choice: {
                case: "changeThrust",
                value: { x, y, thrust, durationMs: duration_ms },
            },
        });
        return this.send(request, at);
    }

    private async send(request: msg.IEngine, timestamp?: bigint): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            timestamp: timestamp ?? BigInt(0),
            choice: { case: "engine", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IEngine | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "engine") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}
