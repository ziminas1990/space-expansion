import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export class Navigation {

    constructor(private session: Session) {}

    async send_position_request(): Promise<types.Status> {
        const request = create(msg.INavigationSchema, {
            choice: { case: "positionReq", value: true },
        });
        return this.send(request);
    }

    async wait_position(timeout: number = 500)
    : Promise<[types.Status, types.Position | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "position") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(),
                types.positionFromProtobuf(response.choice.value, timestamp)];
    }

    private async send(request: msg.INavigation): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "navigation", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.INavigation | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "navigation") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, types.asUint64(response.timestamp)];
    }

}
