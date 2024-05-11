import * as msg from "../Protocol_pb.js"
import * as transport from "../transport/index.js"
import { Status } from "../types/status.js";


export class Session extends transport.Endpoint<msg.Message> {
    private closed: boolean = false;

    constructor(protected channel: transport.IChannel<msg.Message>,
                private session_id: number) {
        super();
    }

    is_active(): boolean {
        return !this.closed;
    }

    async send(message: msg.Message): Promise<Status> {
        if (!this.channel) {
            return Status.fail("not attached to binary channel");
        }
        message.tunnelId = this.session_id;
        return await this.channel.send(message);
    }

    async on_message(message: msg.Message): Promise<void> {
        if (message.choice.case == "session") {
            const session = message.choice.value;
            if (session.choice.case == "heartbeat") {
                // Send heartbeat back, do not forward to the upper level.
                await this.send_hearbeat();
                return;
            } else if (session.choice.case == "closedInd") {
                // Close session, do not forward to the upper level.
                this.on_closed();
                return;
            }
        } else {
            super.on_message(message);
        }
    }

    async on_closed(): Promise<void> {
        this.closed = true;
        super.on_closed();
    }

    async close(): Promise<Status> {
        const close_req = new msg.ISessionControl();
        close_req.choice.case = "close";
        close_req.choice.value = true;
        const message = new msg.Message();
        message.choice.case = "session";
        message.choice.value = close_req;
        this.closed = true;
        return await this.send(message);
    }

    async send_hearbeat() {
        const heartbeat = new msg.ISessionControl();
        heartbeat.choice.case = "heartbeat";
        heartbeat.choice.value = true;
        const message = new msg.Message();
        message.choice.case = "session";
        message.choice.value = heartbeat;
        return await this.send(message);
    }

    async wait_hearbeat(timeout: number = 200): Promise<Status> {
        const [status, message] = await this.wait(timeout);
        if (!status.is_ok() || !message) {
            return status.wrap("no response");
        }
        if (message.choice.case != "session") {
            return Status.fail(`unexpected response type ${message.choice.case}`);
        }
        return Status.ok();
    }

}