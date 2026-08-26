import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type ShipState = {
    timestamp: number;
    position: types.Position;
    weight?: number;
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

    async wait_state(): Promise<[types.Status, ShipState | undefined]> {
        const [status, response, timestamp] = await this.wait();
        if (!response) {
            return [status, undefined];
        }
        if (response.choice.case != "state") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`), undefined];
        }
        const state = response.choice.value as msg.IShip_State;
        return [types.Status.ok(), {
            timestamp: Number(timestamp),
            position: types.positionFromProtobuf(state.position, timestamp),
            weight: state.weight ? Number(state.weight.value) : undefined,
        }];
    }

    async send_request(request: msg.IShip): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "ship", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500):
        Promise<[types.Status, msg.IShip | undefined, bigint]> {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "ship") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, response.timestamp];
    }

}