import { MessagesChannel } from "../transport/channels.js";
import { Status } from "../types/status.js";
import * as msg from "../Protocol_pb.js"

export type AccessGranted = {
    port: number,
    session_id: number
}

export class AccessPanel {
    constructor(private channel: MessagesChannel) {
        this.channel = channel;
    }

    async login(login: string, password: string):
        Promise<[Status, AccessGranted | undefined]>
    {
        const message = new msg.IAccessPanel();
        message.choice.case = "login";
        message.choice.value = new msg.IAccessPanel_LoginRequest();
        message.choice.value.login = login;
        message.choice.value.password = password;

        {
            const status = await this.send(message);
            if (!status.isOk()) {
                return [status.wrap("failed to send login request"), undefined];
            }
        }

        const [status, response] = await this.wait_response(1000);
        if (!status.isOk() || !response) {
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
        const [status, response] = await this.channel.receive(timeout);
        if (!status.isOk() || !response) {
            return [status.wrap("no response"), new msg.IAccessPanel()];
        }
        if (response.choice.case != "accessPanel") {
            return [Status.fail(`unexpected response type ${response.choice.case}`), new msg.IAccessPanel()];
        }
        return [Status.ok(), response.choice.value];
    }

    private async send(request: msg.IAccessPanel): Promise<Status> {
        const message = new msg.Message();
        message.choice.case = "accessPanel";
        message.choice.value = request;
        return await this.channel.send(message);
    }
}