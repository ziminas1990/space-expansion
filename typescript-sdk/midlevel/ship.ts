import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";

export type ShipState = lowlevel.ShipState
export type MonitoringCallback = (status: Status, update: ShipState | undefined) => Promise<boolean>;

export class Ship extends BaseModule<lowlevel.Ship> {

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Ship(session)]);
    }

    async get_state(): Promise<[Status, ShipState | undefined]> {
        return await this.run(this._get_ship_state);
    }

    async monitoring(update_ms: number, callback: MonitoringCallback)
        : Promise<[Status, unknown]> {
        return await this.run(async (session) => this._monitoring(session, update_ms, callback), true);
    }

    async _get_ship_state(session: lowlevel.Ship)
    : Promise<[Status, ShipState | undefined]>
    {
        const send_status = await session.send_state_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        return await session.wait_state();
    }

    private async _monitoring(
        session: lowlevel.Ship,
        update_ms: number,
        callback: MonitoringCallback)
        : Promise<[Status, unknown]> {
        const send_status = await session.send_monitor_request(update_ms);
        if (!send_status.is_ok()) {
            return [send_status.wrap("failed to start monitoring"), undefined];
        }

        while (true) {
            const [status, update] = await session.wait_state();
            if (!status.is_ok()) {
                const stop_status = status.wrap("monitoring stopped");
                callback(stop_status, undefined);
                return [stop_status, undefined];
            } else if (update) {
                const resume = await callback(Status.ok(), update);
                if (!resume) {
                    return [Status.ok(), undefined];
                }
            }
        }
    }

}