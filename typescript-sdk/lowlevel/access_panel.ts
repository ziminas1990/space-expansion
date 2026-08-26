import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js"
import * as transport from "../transport/index.js"
import { Status } from "../types/status.js";

export type AccessGranted = {
    port: number,
    session_id: number
}

export class AccessPanel extends transport.Endpoint<msg.Message> {
    constructor(private channel: transport.MessagesChannel) {
        super();
    }

    async login(login: string, password: string):
        Promise<[Status, AccessGranted | undefined]>
    {
        const message = create(msg.IAccessPanelSchema, {
            choice: {
                case: "login",
                value: create(msg.IAccessPanel_LoginRequestSchema, {
                    login,
                    password,
                }),
            },
        });

        {
            const status = await this.send_request(message);
            if (!status.is_ok()) {
                return [status.wrap("failed to send login request"), undefined];
            }
        }

        const [status, response] = await this.wait_response(1000);
        if (!status.is_ok() || !response) {
            return [status.wrap("failed to receive login response"), undefined];
        }

        if (response.choice.case == "accessGranted") {
            return [Status.ok(), {
                port: response.choice.value.port,
                session_id: response.choice.value.sessionId
            }];
        } else if (response.choice.case == "accessRejected") {
            return [Status.fail(`access denied: ${response.choice.value}`), undefined];
        } else {
            return [Status.fail(`unexpected response type ${response.choice.case}`), undefined];
        }
    }


    private async wait_response(timeout: number = 1000): Promise<[Status, msg.IAccessPanel]> {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), create(msg.IAccessPanelSchema)];
        }
        if (response.choice.case != "accessPanel") {
            return [Status.fail(`unexpected response type ${response.choice.case}`), create(msg.IAccessPanelSchema)];
        }
        return [Status.ok(), response.choice.value];
    }

    private async send_request(request: msg.IAccessPanel): Promise<Status> {
        const message = create(msg.MessageSchema, {
            choice: {
                case: "accessPanel",
                value: request,
            },
        });
        return await this.channel.send(message);
    }
}
