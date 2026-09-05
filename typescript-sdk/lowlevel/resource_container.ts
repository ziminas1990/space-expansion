import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";


export type ResourceContainerStatus =
    | "SUCCESS"
    | "INTERNAL_ERROR"
    | "PORT_ALREADY_OPEN"
    | "PORT_DOESNT_EXIST"
    | "PORT_IS_NOT_OPENED"
    | "PORT_HAS_BEEN_CLOSED"
    | "INVALID_ACCESS_KEY"
    | "INVALID_RESOURCE_TYPE"
    | "PORT_TOO_FAR"
    | "TRANSFER_IN_PROGRESS"
    | "NOT_ENOUGH_RESOURCES";

export type ResourceContainerContent = {
    timestamp: bigint;
    volume: number;
    used: number;
    resources: types.ResourceItem[];
}

export type ResourceContainerOpenPortResult =
    | { case: "port_opened"; port_id: number }
    | { case: "open_port_failed"; status: ResourceContainerStatus };

export type ResourceContainerTransferEvent =
    | { case: "transfer_report"; resource: types.ResourceItem }
    | { case: "transfer_finished"; status: ResourceContainerStatus };

export class ResourceContainer {

    constructor(private session: Session) {}

    async send_content_request(): Promise<types.Status> {
        const request = create(msg.IResourceContainerSchema, {
            choice: { case: "contentReq", value: true },
        });
        return this.send(request);
    }

    async wait_content(timeout: number = 500)
    : Promise<[types.Status, ResourceContainerContent | undefined]>
    {
        const [status, response, timestamp] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "content") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const content = response.choice.value;
        return [types.Status.ok(), {
            timestamp,
            volume: content.volume,
            used: content.used,
            resources: types.resourceItemsFromProtobuf(content.resources),
        }];
    }

    async send_open_port(access_key: number): Promise<types.Status> {
        const request = create(msg.IResourceContainerSchema, {
            choice: { case: "openPort", value: access_key },
        });
        return this.send(request);
    }

    async wait_open_port(timeout: number = 500)
    : Promise<[types.Status, ResourceContainerOpenPortResult | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "openPortFailed") {
            const server_status = resourceContainerStatusFromProtobuf(response.choice.value);
            if (!server_status) {
                return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "open_port_failed",
                status: server_status,
            }];
        }
        if (response.choice.case != "portOpened") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }

        return [types.Status.ok(), {
            case: "port_opened",
            port_id: response.choice.value,
        }];
    }

    async send_close_port(): Promise<types.Status> {
        const request = create(msg.IResourceContainerSchema, {
            choice: { case: "closePort", value: true },
        });
        return this.send(request);
    }

    async wait_close_port_status(timeout: number = 500)
    : Promise<[types.Status, ResourceContainerStatus | undefined]>
    {
        return this.wait_status("closePortStatus", timeout);
    }

    async send_monitor_request(): Promise<types.Status> {
        const request = create(msg.IResourceContainerSchema, {
            choice: { case: "monitor", value: true },
        });
        return this.send(request);
    }

    async send_transfer(
        port_id: number,
        access_key: number,
        resource: types.ResourceItem): Promise<types.Status>
    {
        const request = create(msg.IResourceContainerSchema, {
            choice: {
                case: "transfer",
                value: {
                    portId: port_id,
                    accessKey: access_key,
                    resource: types.resourceItemToProtobuf(resource),
                },
            },
        });
        return this.send(request);
    }

    async wait_transfer_status(timeout: number = 500)
    : Promise<[types.Status, ResourceContainerStatus | undefined]>
    {
        return this.wait_status("transferStatus", timeout);
    }

    async wait_transfer_event(timeout: number = 1000)
    : Promise<[types.Status, ResourceContainerTransferEvent | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case == "transferFinished") {
            const server_status = resourceContainerStatusFromProtobuf(response.choice.value);
            if (!server_status) {
                return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                        undefined];
            }
            return [types.Status.ok(), {
                case: "transfer_finished",
                status: server_status,
            }];
        }
        if (response.choice.case != "transferReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            case: "transfer_report",
            resource: types.resourceItemFromProtobuf(response.choice.value),
        }];
    }

    private async wait_status(
        expected_case: "closePortStatus" | "transferStatus",
        timeout: number)
    : Promise<[types.Status, ResourceContainerStatus | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != expected_case) {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const server_status = resourceContainerStatusFromProtobuf(response.choice.value);
        if (!server_status) {
            return [types.Status.fail(`got unexpected status ${response.choice.value}`),
                    undefined];
        }
        return [types.Status.ok(), server_status];
    }

    private async send(request: msg.IResourceContainer): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "resourceContainer", value: request },
        });
        return this.session.send(message);
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IResourceContainer | undefined, bigint]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined, BigInt(0)];
        }
        if (response.choice.case != "resourceContainer") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined, BigInt(0)];
        }
        return [types.Status.ok(), response.choice.value, types.asUint64(response.timestamp)];
    }

}

function resourceContainerStatusFromProtobuf(
    value: msg.IResourceContainer_Status): ResourceContainerStatus | undefined
{
    switch (value) {
        case msg.IResourceContainer_Status.SUCCESS: return "SUCCESS";
        case msg.IResourceContainer_Status.INTERNAL_ERROR: return "INTERNAL_ERROR";
        case msg.IResourceContainer_Status.PORT_ALREADY_OPEN: return "PORT_ALREADY_OPEN";
        case msg.IResourceContainer_Status.PORT_DOESNT_EXIST: return "PORT_DOESNT_EXIST";
        case msg.IResourceContainer_Status.PORT_IS_NOT_OPENED: return "PORT_IS_NOT_OPENED";
        case msg.IResourceContainer_Status.PORT_HAS_BEEN_CLOSED: return "PORT_HAS_BEEN_CLOSED";
        case msg.IResourceContainer_Status.INVALID_ACCESS_KEY: return "INVALID_ACCESS_KEY";
        case msg.IResourceContainer_Status.INVALID_RESOURCE_TYPE: return "INVALID_RESOURCE_TYPE";
        case msg.IResourceContainer_Status.PORT_TOO_FAR: return "PORT_TOO_FAR";
        case msg.IResourceContainer_Status.TRANSFER_IN_PROGRESS: return "TRANSFER_IN_PROGRESS";
        case msg.IResourceContainer_Status.NOT_ENOUGH_RESOURCES: return "NOT_ENOUGH_RESOURCES";
        default: return undefined;
    }
}
