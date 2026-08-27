import * as lowlevel from "../lowlevel/index.js";
import { ResourceItem } from "../types/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";

export type ResourceContainerStatus = lowlevel.ResourceContainerStatus;
export type ResourceContainerContent = lowlevel.ResourceContainerContent;
export type TransferCallback =
    (resource: ResourceItem) => Promise<void>;
export type MonitoringCallback =
    (content: ResourceContainerContent) => Promise<boolean>;

export class ResourceContainer extends BaseModule<lowlevel.ResourceContainer> {
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.ResourceContainer(session)]);
    }

    async get_content()
        : Promise<[Status, ResourceContainerContent | undefined]>
    {
        return await this.run(async (session) => this._get_content(session));
    }

    async open_port(access_key: number)
        : Promise<[Status, number | undefined]>
    {
        return await this.run(async (session) => this._open_port(session, access_key));
    }

    async close_port(): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._close_port(session));
    }

    async transfer(
        port_id: number,
        access_key: number,
        resource: ResourceItem,
        progress_cb?: TransferCallback): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._transfer(
                session, port_id, access_key, resource, progress_cb),
            true);
    }

    async monitoring(callback: MonitoringCallback): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._monitoring(session, callback), true);
    }

    private async _get_content(session: lowlevel.ResourceContainer)
        : Promise<[Status, ResourceContainerContent | undefined]>
    {
        const send_status = await session.send_content_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, content] = await session.wait_content();
        if (!status.is_ok() || !content) {
            return [status.wrap("failed to get resource container content"),
                    undefined];
        }
        return [Status.ok(), content];
    }

    private async _open_port(
        session: lowlevel.ResourceContainer,
        access_key: number)
        : Promise<[Status, number | undefined]>
    {
        const send_status = await session.send_open_port(access_key);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send open port request"), undefined];
        }
        const [status, result] = await session.wait_open_port();
        if (!status.is_ok() || !result) {
            return [status.wrap("failed to open port"), undefined];
        }
        if (result.case === "open_port_failed") {
            return [Status.fail(result.status), undefined];
        }
        return [Status.ok(), result.port_id];
    }

    private async _close_port(session: lowlevel.ResourceContainer): Promise<Status>
    {
        const send_status = await session.send_close_port();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send close port request");
        }
        const [status, server_status] = await session.wait_close_port_status();
        if (!status.is_ok() || !server_status) {
            return status.wrap("failed to close port");
        }
        if (server_status !== "SUCCESS") {
            return Status.fail(server_status);
        }
        return Status.ok();
    }

    private async _transfer(
        session: lowlevel.ResourceContainer,
        port_id: number,
        access_key: number,
        resource: ResourceItem,
        progress_cb?: TransferCallback): Promise<Status>
    {
        const send_status = await session.send_transfer(port_id, access_key, resource);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send transfer request");
        }

        const [status, server_status] = await session.wait_transfer_status();
        if (!status.is_ok() || !server_status) {
            return status.wrap("failed to start transfer");
        }
        if (server_status !== "SUCCESS") {
            return Status.fail(server_status);
        }

        while (true) {
            const [event_status, event] = await session.wait_transfer_event(2000);
            if (!event_status.is_ok() || !event) {
                return event_status.wrap("failed to get transfer report");
            }
            if (event.case === "transfer_finished") {
                if (event.status !== "SUCCESS") {
                    return Status.fail(event.status);
                }
                return Status.ok();
            }
            if (progress_cb) {
                await progress_cb(event.resource);
            }
        }
    }

    private async _monitoring(
        session: lowlevel.ResourceContainer,
        callback: MonitoringCallback): Promise<Status>
    {
        const send_status = await session.send_monitor_request();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send monitor request");
        }

        const [start_status, start_content] = await session.wait_content();
        if (!start_status.is_ok() || !start_content) {
            return start_status.wrap("failed to start monitoring");
        }
        if (!await callback(start_content)) {
            return Status.ok();
        }

        while (true) {
            const [status, content] = await session.wait_content();
            if (status.is_timeout()) {
                continue;
            }
            if (!status.is_ok() || !content) {
                return status.wrap("monitoring stopped");
            }
            const resume = await callback(content);
            if (!resume) {
                return Status.ok();
            }
        }
    }
}
