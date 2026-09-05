import * as lowlevel from "#sdk/lowlevel/index.js";
import { Status } from "#sdk/types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type ShipyardStatus = lowlevel.ShipyardStatus;
export type ShipyardSpecification = lowlevel.ShipyardSpecification;
export type ShipyardShipBuilt = lowlevel.ShipyardShipBuilt;
export type BuildingCallback =
    (status: ShipyardStatus, progress: number) => Promise<void>;

export class Shipyard extends BaseModule<lowlevel.Shipyard> {
    readonly type = ModuleType.SHIPYARD;
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Shipyard(session)]);
    }

    async get_specification()
        : Promise<[Status, ShipyardSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async bind_to_cargo(cargo_name: string): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._bind_to_cargo(session, cargo_name));
    }

    async cancel_build(): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._cancel_build(session));
    }

    async build_ship(
        blueprint: string,
        ship_name: string,
        progress_cb?: BuildingCallback)
        : Promise<[Status, ShipyardShipBuilt | undefined]>
    {
        return await this.run(
            async (session) => this._build_ship(
                session, blueprint, ship_name, progress_cb),
            true);
    }

    private async _get_specification(session: lowlevel.Shipyard)
        : Promise<[Status, ShipyardSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get shipyard specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _bind_to_cargo(
        session: lowlevel.Shipyard,
        cargo_name: string): Promise<Status>
    {
        const send_status = await session.send_bind_to_cargo(cargo_name);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send bind to cargo request");
        }
        const [status, server_status] = await session.wait_bind_to_cargo_status();
        if (!status.is_ok() || !server_status) {
            return status.wrap("failed to bind to cargo");
        }
        if (server_status !== "SUCCESS") {
            return Status.fail(server_status);
        }
        return Status.ok();
    }

    private async _cancel_build(session: lowlevel.Shipyard): Promise<Status>
    {
        const send_status = await session.send_cancel_build();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send cancel build request");
        }
        const [status, event] = await session.wait_building_event();
        if (!status.is_ok() || !event) {
            return status.wrap("failed to cancel build");
        }
        if (event.case !== "building_report") {
            return Status.fail("got unexpected building complete");
        }
        if (event.report.status !== "BUILD_CANCELED" &&
            event.report.status !== "SUCCESS") {
            return Status.fail(event.report.status);
        }
        return Status.ok();
    }

    private async _build_ship(
        session: lowlevel.Shipyard,
        blueprint: string,
        ship_name: string,
        progress_cb?: BuildingCallback)
        : Promise<[Status, ShipyardShipBuilt | undefined]>
    {
        const send_status = await session.send_start_build(blueprint, ship_name);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to send start build request"), undefined];
        }

        const [start_status, start_event] = await session.wait_building_event();
        if (!start_status.is_ok() || !start_event) {
            return [start_status.wrap("failed to start build"), undefined];
        }
        if (start_event.case !== "building_report") {
            return [Status.fail("got unexpected building complete"), undefined];
        }
        if (start_event.report.status !== "BUILD_STARTED") {
            return [Status.fail(start_event.report.status), undefined];
        }

        while (true) {
            const [status, event] = await session.wait_building_event();
            if (!status.is_ok() || !event) {
                return [status.wrap("failed to get building report"), undefined];
            }
            if (event.case === "building_complete") {
                return [Status.ok(), event.ship];
            }

            if (progress_cb) {
                await progress_cb(event.report.status, event.report.progress);
            }
            if (event.report.status === "BUILD_IN_PROGRESS" ||
                event.report.status === "BUILD_FROZEN") {
                continue;
            }
            if (event.report.status !== "BUILD_COMPLETE") {
                return [Status.fail(event.report.status), undefined];
            }

            const [complete_status, complete_event] =
                await session.wait_building_event();
            if (!complete_status.is_ok() || !complete_event) {
                return [complete_status.wrap("failed to get building complete"),
                        undefined];
            }
            if (complete_event.case !== "building_complete") {
                return [Status.fail("got unexpected building report"), undefined];
            }
            return [Status.ok(), complete_event.ship];
        }
    }
}
