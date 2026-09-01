import { create } from "@bufbuild/protobuf";
import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type MessangerStatus =
    | "SUCCESS"
    | "ROUTED"
    | "SERVICE_EXISTS"
    | "NO_SUCH_SERVICE"
    | "TOO_MANY_SERVCES"
    | "SESSION_BUSY"
    | "WRONG_SEQ"
    | "CLOSED"
    | "UNKNOWN_ERROR"
    | "REQUEST_TIMEOUT_TOO_LONG"
    | "SESSIONS_LIMIT_REACHED";

export type MessangerRequest = {
    service: string;
    seq: number;
    timeout_ms: number;
    body: string;
}

export type MessangerResponse = {
    seq: number;
    body: string;
}

export type MessangerSessionStatus = {
    seq: number;
    status: MessangerStatus;
}

export type MessangerServicesPage = {
    services: string[];
    left: number;
    timestamp: bigint;
}

export type MessangerClientEvent =
    | { case: "response"; seq: number; body: string }
    | { case: "session_status"; seq: number; status: MessangerStatus };

export class Messanger {

    constructor(private session: Session) {}

    async send_open_service(service_name: string, force: boolean = false)
    : Promise<types.Status>
    {
        const request = create(msg.IMessangerSchema, {
            choice: {
                case: "openService",
                value: { serviceName: service_name, force },
            },
        });
        return this.send(request);
    }

    async wait_open_service_status(timeout: number = 500)
    : Promise<[types.Status, MessangerStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "openServiceStatus") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const server_status = messangerStatusFromProtobuf(response.choice.value);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                    undefined];
        }
        return [types.Status.ok(), server_status];
    }

    async send_services_list_request(): Promise<types.Status> {
        const request = create(msg.IMessangerSchema, {
            choice: { case: "servicesListReq", value: true },
        });
        return this.send(request);
    }

    async wait_services_list(timeout: number = 500)
    : Promise<[types.Status, MessangerServicesPage | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "servicesList") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const page = response.choice.value;
        return [types.Status.ok(), {
            services: page.services ?? [],
            left: page.left,
            timestamp,
        }];
    }

    async send_request(
        service: string,
        seq: number,
        body: string,
        timeout_ms: number = 1000): Promise<types.Status>
    {
        const request = create(msg.IMessangerSchema, {
            choice: {
                case: "request",
                value: {
                    service,
                    seq,
                    body,
                    timeoutMs: timeout_ms,
                },
            },
        });
        return this.send(request);
    }

    async send_response(seq: number, body: string): Promise<types.Status> {
        const request = create(msg.IMessangerSchema, {
            choice: {
                case: "response",
                value: { seq, body },
            },
        });
        return this.send(request);
    }

    async wait_request(timeout: number = 500)
    : Promise<[types.Status, MessangerRequest | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "request") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const request = response.choice.value;
        return [types.Status.ok(), {
            service: request.service,
            seq: request.seq,
            timeout_ms: request.timeoutMs,
            body: request.body,
        }];
    }

    async wait_session_status(timeout: number = 500)
    : Promise<[types.Status, MessangerSessionStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "sessionStatus") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const session_status = sessionStatusFromProtobuf(response.choice.value);
        if (!session_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value.status}`),
                    undefined];
        }
        return [types.Status.ok(), session_status];
    }

    async wait_response(timeout: number = 1000)
    : Promise<[types.Status, MessangerClientEvent | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "sessionStatus") {
            const session_status = sessionStatusFromProtobuf(response.choice.value);
            if (!session_status) {
                return [types.Status.fail(
                    `got unexpected status ${response.choice.value.status}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "session_status",
                seq: session_status.seq,
                status: session_status.status,
            }];
        }
        if (response.choice.case != "response") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const payload = response.choice.value;
        return [types.Status.ok(), {
            case: "response",
            seq: payload.seq,
            body: payload.body,
        }];
    }

    private async send(request: msg.IMessanger): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "messanger", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IMessanger | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, 0n];
        }
        if (response.choice.case != "messanger") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, 0n];
        }
        return [
            types.Status.ok(),
            response.choice.value,
            types.asUint64(response.timestamp),
        ];
    }

}

function sessionStatusFromProtobuf(
    value: msg.IMessanger_SessionStatus): MessangerSessionStatus | undefined
{
    const status = messangerStatusFromProtobuf(value.status);
    if (!status) {
        return undefined;
    }
    return { seq: value.seq, status };
}

function messangerStatusFromProtobuf(
    value: msg.IMessanger_Status): MessangerStatus | undefined
{
    switch (value) {
        case msg.IMessanger_Status.SUCCESS: return "SUCCESS";
        case msg.IMessanger_Status.ROUTED: return "ROUTED";
        case msg.IMessanger_Status.SERVICE_EXISTS: return "SERVICE_EXISTS";
        case msg.IMessanger_Status.NO_SUCH_SERVICE: return "NO_SUCH_SERVICE";
        case msg.IMessanger_Status.TOO_MANY_SERVCES: return "TOO_MANY_SERVCES";
        case msg.IMessanger_Status.SESSION_BUSY: return "SESSION_BUSY";
        case msg.IMessanger_Status.WRONG_SEQ: return "WRONG_SEQ";
        case msg.IMessanger_Status.CLOSED: return "CLOSED";
        case msg.IMessanger_Status.UNKNOWN_ERROR: return "UNKNOWN_ERROR";
        case msg.IMessanger_Status.REQUEST_TIMEOUT_TOO_LONG: return "REQUEST_TIMEOUT_TOO_LONG";
        case msg.IMessanger_Status.SESSIONS_LIMIT_REACHED: return "SESSIONS_LIMIT_REACHED";
        default: return undefined;
    }
}
