import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";


export type ShipState = {
    timestamp: number;
    position: types.Position;
    orientation: types.Vector;
    weight?: number;
}

export type ShipSpecification = {
    max_rotation_speed: number;
    radius: number;
}

export class Ship {

    constructor(private session: Session) {}

    async send_state_request(): Promise<types.Status> {
        const request = create(msg.IShipSchema, {
            choice: { case: "stateReq", value: true },
        });
        return this.send_request(request);
    }

    async send_monitor_request(period_ms: number): Promise<types.Status> {
        const request = create(msg.IShipSchema, {
            choice: { case: "monitor", value: period_ms },
        });
        return this.send_request(request);
    }

    async wait_state(timeout_ms: number = 500)
    : Promise<[types.Status, ShipState | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout_ms);
        if (status.is_timeout()) {
            return [status, undefined];
        }
        if (!response) {
            return [status, undefined];
        }
        if (response.choice.case != "state") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`), undefined];
        }
        const state = response.choice.value as msg.IShip_State;
        return [types.Status.ok(), {
            timestamp,
            position: types.positionFromProtobuf(state.position, timestamp),
            orientation: directionFrom(state.orientation),
            weight: state.weight ? Number(state.weight.value) : undefined,
        }];
    }

    async send_specification_request(): Promise<types.Status> {
        const request = create(msg.IShipSchema, {
            choice: { case: "specificationReq", value: true },
        });
        return this.send_request(request);
    }

    async wait_specification(timeout_ms: number = 500)
    : Promise<[types.Status, ShipSpecification | undefined]>
    {
        const [status, response] = await this.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "specification") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            max_rotation_speed: response.choice.value.maxRotationSpeed,
            radius: response.choice.value.radius,
        }];
    }

    async send_rotate(x: number, y: number, speed: number): Promise<types.Status> {
        const request = create(msg.IShipSchema, {
            choice: { case: "rotate", value: { x, y, speed } },
        });
        return this.send_request(request);
    }

    async wait_rotate_ack(timeout_ms: number = 500): Promise<types.Status> {
        const [status, response] = await this.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return status.wrap("no response");
        }
        if (response.choice.case != "rotateAck") {
            return types.Status.fail(`got unexpected message ${response.choice.case}`);
        }
        return types.Status.ok();
    }

    async wait_next(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IShip | undefined]>
    {
        const [status, response] = await this.wait(timeout_ms);
        return [status, response];
    }

    async send_request(request: msg.IShip): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "ship", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500):
        Promise<[types.Status, msg.IShip | undefined, number]> {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, 0];
        }
        if (response.choice.case != "ship") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, 0];
        }
        return [
            types.Status.ok(),
            response.choice.value,
            types.asNumber(response.timestamp)
        ];
    }

}

function directionFrom(
    direction: { x: number; y: number } | undefined,
): types.Vector {
    if (direction === undefined) {
        return [1, 0];
    }
    return [direction.x, direction.y];
}