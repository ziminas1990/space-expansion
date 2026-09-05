import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js"
import * as transport from "#sdk/transport/index.js"
import { Session } from "./session.js";
import { Status } from "#sdk/types/status.js";
import { Commutator } from "./commutator.js";


export class RootSession extends Session {
    private child_sessions: Map<number, Session> = new Map();

    constructor(channel: transport.IChannel<msg.Message>,
                session_id: number) {

        const invalid_register = () => {
            throw new Error("Root session can't register new session");
        };
        super(channel, session_id, invalid_register);
    }

    async open_session(): Promise<[Status, Session | undefined]> {
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

        return this.register_new_session(session_id);
    }

    async connect_to_root_commutator(): Promise<[Status, Commutator | undefined]> {
        const [register_status, session] = await this.open_session();
        if (!register_status.is_ok() || !session) {
            return [register_status.wrap("failed to register new session"), undefined];
        }
        return [Status.ok(), new Commutator(session)];
    }

    register_new_session(session_id: number): [Status, Session | undefined] {
        if (!this.channel) {
            return [Status.notConnected(), undefined];
        }

        if (session_id == 0) {
            return [Status.fail("invalid session_id"), undefined];
        }

        if (this.child_sessions.has(session_id)) {
            return [Status.fail("session already registered"), undefined];
        }

        const session = new Session(
            this.channel, session_id, (id) => this.register_new_session(id));
        this.child_sessions.set(session_id, session);
        return [Status.ok(), session];
    }

    async on_message(message: msg.Message): Promise<void> {
        if (this.child_sessions.has(message.tunnelId)) {
            await this.child_sessions.get(message.tunnelId)?.on_message(message);
            if (message.choice.case == "session") {
                const session = message.choice.value;
                if (session.choice.case == "closedInd") {
                    // Session has been closed, remove it from the list
                    this.child_sessions.delete(message.tunnelId);
                }
            }
        } else {
            super.on_message(message);
        }
    }

    private async send_new_commutator_session_request() {
        const request = create(msg.IRootSessionSchema, {
            choice: { case: "newCommutatorSession", value: true },
        });
        const message = create(msg.MessageSchema, {
            choice: { case: "rootSession", value: request },
        });
        return await this.send(message);
    }

    private async wait_commutator_session(timeout: number = 500): Promise<[Status, number]> {
        const [status, message] = await this.wait(timeout);
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