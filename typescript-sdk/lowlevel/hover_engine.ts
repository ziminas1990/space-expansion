import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";

export type HoverEngineSpecification = {
    max_thrust: number;
}

export class HoverEngine {

    constructor(private session: Session) {}

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IHoverEngineSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send(request);
    }

    async wait_specification(timeout: number = 500)
    : Promise<[types.Status, HoverEngineSpecification | undefined]>
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
        const request = create(msg.IHoverEngineSchema, {
            choice: { case: "thrustReq", value: true },
        });
        return this.send(request);
    }

    async send_monitor_request(): Promise<types.Status> {
        const request = create(msg.IHoverEngineSchema, {
            choice: { case: "monitor", value: true },
        });
        return this.send(request);
    }

    async wait_thrust(timeout: number = 500)
    : Promise<[types.Status, number | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (status.is_timeout()) {
            return [status, undefined];
        }
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "thrust") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

    async send_change_thrust(
        thrust: number,
        duration_ms: number = 0,
        at?: number): Promise<types.Status>
    {
        const request = create(msg.IHoverEngineSchema, {
            choice: {
                case: "changeThrust",
                value: { thrust, durationMs: duration_ms },
            },
        });
        return this.send(request, at);
    }

    private async send(request: msg.IHoverEngine, timestamp?: number): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            timestamp: BigInt(timestamp ?? 0),
            choice: { case: "hoverEngine", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IHoverEngine | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "hoverEngine") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}
