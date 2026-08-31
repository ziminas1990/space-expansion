import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { Commutator } from "./commutator.js";
import { ModuleType } from "./module_type.js";
import { Navigation } from "./navigation.js";

export type ShipState = lowlevel.ShipState
export type MonitoringCallback =
    (update: ShipState | undefined) => Promise<boolean>;

export class Ship extends BaseModule<lowlevel.Ship> {
    readonly type = ModuleType.SHIP;
    private nested_navigation?: Navigation;
    private nested_commutator?: Commutator;

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Ship(session)]);
    }

    navigator(): Navigation {
        this.nested_navigation ??= new Navigation(this.open_session_cb);
        return this.nested_navigation;
    }

    commutator(): Commutator {
        this.nested_commutator ??= new Commutator(this.open_session_cb);
        return this.nested_commutator;
    }

    override async terminate(): Promise<void> {
        await this.nested_navigation?.terminate();
        await this.nested_commutator?.terminate();
        await super.terminate();
    }

    async get_state(): Promise<[Status, ShipState | undefined]> {
        return await this.run(this._get_ship_state);
    }

    async monitoring(update_ms: number, callback: MonitoringCallback)
        : Promise<Status> {
        return await this.run_no_return(
            async (session) => this._monitoring(session, update_ms, callback), true);
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
        : Promise<Status> {
        const send_status = await session.send_monitor_request(update_ms);
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to start monitoring");
        }

        while (true) {
            const [status, update] = await session.wait_state();
            if (status.is_timeout()) {
                const resume = await callback(undefined);
                if (!resume) {
                    return Status.ok();
                }
                continue;
            }
            if (!status.is_ok() || !update) {
                return status.wrap("monitoring stopped");
            }
            const resume = await callback(update);
            if (!resume) {
                return Status.ok();
            }
        }
    }

}