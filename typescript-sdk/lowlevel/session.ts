import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js"
import * as transport from "#sdk/transport/index.js"
import { Status } from "#sdk/types/status.js";
import type { Router } from "./router.js";

export class Session extends transport.Endpoint<msg.Message> {
    constructor(protected channel: transport.IChannel<msg.Message>,
                private session_id: number,
                private readonly owner: Router)
    {
        super();
    }

    router(): Router {
        return this.owner;
    }

    async send(message: msg.Message): Promise<Status> {
        if (!this.channel) {
            return Status.fail("not attached to binary channel");
        }
        message.tunnelId = this.session_id;
        return await this.channel.send(message);
    }

    async on_message(message: msg.Message): Promise<void> {
        if (message.choice.case == "session"
            && message.choice.value.choice.case == "closedInd") {
            // Close session, do not forward to the upper level.
            this.on_closed();
            return;
        }
        super.on_message(message);
    }

    async close(): Promise<Status> {
        if (!this.is_active()) {
            return Status.closed("session is already closed");
        }
        const close_req = create(msg.ISessionControlSchema, {
            choice: { case: "close", value: true },
        });
        const message = create(msg.MessageSchema, {
            choice: { case: "session", value: close_req },
        });
        const status = await this.send(message);
        // We are not obliged to call on_close() here, because we will receive
        // a closeInd soon. But anyway, closing the session immediately is a
        // good idea here.
        await this.on_closed();
        return status;
    }
}
