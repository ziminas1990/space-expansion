import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type MessangerStatus = lowlevel.MessangerStatus;
export type MessangerRequest = lowlevel.MessangerRequest;
export type MessangerServicesList = {
    services: string[];
    timestamp: bigint;
};

export class MessangerService {
    private readonly messanger: lowlevel.Messanger;

    constructor(
        public readonly name: string,
        private readonly session: lowlevel.Session,
        private readonly on_closed?: () => void)
    {
        this.messanger = new lowlevel.Messanger(session);
    }

    async wait_request(timeout_ms: number = 0)
        : Promise<[Status, MessangerRequest | undefined]>
    {
        if (timeout_ms <= 0) {
            while (true) {
                const [status, request] = await this.messanger.wait_request(10000);
                if (status.is_timeout()) {
                    continue;
                }
                if (!status.is_ok() || !request) {
                    return [status.wrap("failed to wait request"), undefined];
                }
                return [Status.ok(), request];
            }
        }

        const [status, request] = await this.messanger.wait_request(timeout_ms);
        if (!status.is_ok() || !request) {
            return [status.wrap("failed to wait request"), undefined];
        }
        return [Status.ok(), request];
    }

    async send_response(request: MessangerRequest, body: string): Promise<Status>
    {
        return this.messanger.send_response(request.seq, body);
    }

    // Drop all incoming requests, that has been queued already.
    drop_queued_requests(): void
    {
        this.session.flush();
    }

    async close(): Promise<Status>
    {
        this.on_closed?.();
        return this.session.close();
    }
}

export class Messanger extends BaseModule<lowlevel.Messanger> {
    readonly type = ModuleType.MESSANGER;

    private next_seq = 1;
    private readonly open_session_callback: OpenSessionCallback;
    private readonly hosted_sessions = new Set<lowlevel.Session>();

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Messanger(session)]);
        this.open_session_callback = open_session_callback;
    }

    override async terminate(): Promise<void> {
        const hosted = Array.from(this.hosted_sessions);
        this.hosted_sessions.clear();
        for (const session of hosted) {
            await session.close();
        }
        await super.terminate();
    }

    async services_list(): Promise<[Status, MessangerServicesList | undefined]>
    {
        return await this.run(async (session) => this._services_list(session));
    }

    async send_request(service: string, body: string, timeout_ms: number = 1000)
        : Promise<[Status, string | undefined]>
    {
        const seq = this.next_seq++;
        return await this.run(
            async (session) => this._send_request(session, service, seq, body, timeout_ms));
    }

    async open_service(service_name: string, force: boolean = false)
        : Promise<[Status, MessangerService | undefined]>
    {
        const [status, session] = await this.open_dedicated_session();
        if (!status.is_ok() || !session) {
            return [status.wrap("failed to open service session"), undefined];
        }

        const messanger = new lowlevel.Messanger(session);
        const send_status = await messanger.send_open_service(service_name, force);
        if (!send_status.is_ok()) {
            await session.close();
            return [send_status.wrap("failed to send open service request"), undefined];
        }

        const [open_status, server_status] = await messanger.wait_open_service_status();
        if (!open_status.is_ok() || !server_status) {
            await session.close();
            return [open_status.wrap("failed to open service"), undefined];
        }
        if (server_status !== "SUCCESS") {
            await session.close();
            return [Status.fail(server_status), undefined];
        }

        this.hosted_sessions.add(session);
        return [Status.ok(), new MessangerService(service_name, session, () => {
            this.hosted_sessions.delete(session);
        })];
    }

    private async open_dedicated_session()
        : Promise<[Status, lowlevel.Session | undefined]>
    {
        let last_status = Status.ok();
        for (let attempt = 0; attempt < 3; attempt++) {
            const [status, session] = await this.open_session_callback();
            if (status.is_ok() && session) {
                return [Status.ok(), session];
            }
            last_status = status;
        }
        return [last_status.wrap("failed to open new session"), undefined];
    }

    private async _services_list(session: lowlevel.Messanger)
        : Promise<[Status, MessangerServicesList | undefined]>
    {
        const send_status = await session.send_services_list_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }

        const services: string[] = [];
        let timestamp = 0n;
        while (true) {
            const [status, page] = await session.wait_services_list();
            if (!status.is_ok() || !page) {
                return [status.wrap("failed to get services list page"), undefined];
            }
            services.push(...page.services);
            timestamp = page.timestamp;
            if (page.left === 0) {
                return [Status.ok(), { services, timestamp }];
            }
        }
    }

    private async _send_request(
        session: lowlevel.Messanger,
        service: string,
        seq: number,
        body: string,
        timeout_ms: number)
        : Promise<[Status, string | undefined]>
    {
        const send_status = await session.send_request(service, seq, body, timeout_ms);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send request"), undefined];
        }

        const [status, session_status] = await session.wait_session_status();
        if (!status.is_ok() || !session_status) {
            return [status.wrap("failed to get session status"), undefined];
        }
        if (session_status.seq !== seq) {
            return [
                Status.fail(`got unexpected status sequence ${session_status.seq}`),
                undefined
            ];
        }
        if (session_status.status !== "ROUTED") {
            return [Status.fail(session_status.status), undefined];
        }

        const [event_status, event] = await session.wait_response(timeout_ms + 500);
        if (!event_status.is_ok() || !event) {
            return [event_status.wrap("failed to get response"), undefined];
        }
        if (event.seq !== seq) {
            return [Status.fail(`got unexpected response sequence ${event.seq}`),
                    undefined];
        }
        if (event.case === "session_status") {
            return [Status.fail(event.status), undefined];
        }
        return [Status.ok(), event.body];
    }
}
