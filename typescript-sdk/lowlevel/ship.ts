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
        const request = new msg.IShip();
        request.choice.case = "stateReq";
        request.choice.value = true;
        return this.send_request(request);
    }

    async send_monitor_request(period_ms: number): Promise<types.Status> {
        const request = new msg.IShip();
        request.choice.case = "monitor";
        request.choice.value = period_ms;
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
            timestamp: timestamp,
            position: {
                timestamp: timestamp,
                point: [state.position?.x ?? 0, state.position?.y ?? 0],
                velocity: [state.position?.vx ?? 0, state.position?.vy ?? 0],
            },
            weight: state.weight ? Number(state.weight) : undefined,
        }];
    }

    async send_request(request: msg.IShip): Promise<types.Status> {
        const message = new msg.Message();
        message.choice.case = "ship";
        message.choice.value = request;
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
        return [types.Status.ok(), response.choice.value, Number(response.timestamp)];
    }

}