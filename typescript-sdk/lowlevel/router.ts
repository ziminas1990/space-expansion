import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js"
import * as transport from "#sdk/transport/index.js"
import { Session } from "./session.js";
import { Status } from "#sdk/types/status.js";

// Holds a table of all active sessions and routes incoming messages to the
// appropriate session.
export class Router implements transport.ITerminal<msg.Message> {
    private readonly sessions = new Map<number, Session>();
    private readonly root: Session;

    constructor(
        private readonly channel: transport.IChannel<msg.Message>,
        root_session_id: number,
    ) {
        this.root = new Session(channel, root_session_id, this);
        this.sessions.set(root_session_id, this.root);
    }

    async on_message(message: msg.Message): Promise<void> {
        const session = this.sessions.get(message.tunnelId);
        if (!session) {
            return;
        }
        await session.on_message(message);
        if (message.choice.case == "session"
            && message.choice.value.choice.case == "closedInd")
        {
            this.sessions.delete(message.tunnelId);
        }
    }

    async on_closed(): Promise<void> {
        const sessions = [...this.sessions.values()];
        this.sessions.clear();
        await Promise.all(sessions.map((session) => session.on_closed()));
    }

    async close(): Promise<Status> {
        const status = await this.root.close();
        await this.on_closed();
        return status;
    }

    // Binds an already-issued tunnel id to a local Session and adds it to the
    // routing table. Does not talk to the server.
    register_session(session_id: number): [Status, Session | undefined] {
        if (session_id == 0) {
            return [Status.fail("invalid session_id"), undefined];
        }
        if (this.sessions.has(session_id)) {
            return [Status.fail("session already registered"), undefined];
        }
        const session = new Session(this.channel, session_id, this);
        this.sessions.set(session_id, session);
        return [Status.ok(), session];
    }

    // Opens a new tunnel attached to the player's root commutator via
    // IRootSession.newCommutatorSession.
    async open_commutator_session(): Promise<[Status, Session | undefined]> {
        const send_status = await this.send_new_commutator_session_request();
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send request"), undefined];
        }

        const [wait_status, session_id] = await this.wait_commutator_session();
        if (!wait_status.is_ok()) {
            return [wait_status.wrap("no response"), undefined];
        }
        if (session_id == 0) {
            return [Status.fail("got invalid session_id"), undefined];
        }

        return this.register_session(session_id);
    }

    private async send_new_commutator_session_request() {
        const request = create(msg.IRootSessionSchema, {
            choice: { case: "newCommutatorSession", value: true },
        });
        const message = create(msg.MessageSchema, {
            choice: { case: "rootSession", value: request },
        });
        return await this.root.send(message);
    }

    private async wait_commutator_session(timeout: number = 500): Promise<[Status, number]> {
        const [status, message] = await this.root.wait(timeout);
        if (!status.is_ok() || !message) {
            return [status.wrap("no response"), 0];
        }
        if (message.choice.case != "rootSession") {
            return [Status.fail(`unexpected response type ${message.choice.case}`), 0];
        }
        const response = message.choice.value;
        if (response.choice.case != "commutatorSession") {
            return [Status.fail(`unexpected response type rootSession.${response.choice.case}`), 0];
        }
        const session_id = response.choice.value;
        if (session_id == 0) {
            return [Status.fail("got invalid session_id"), 0];
        }
        return [Status.ok(), session_id];
    }
}
