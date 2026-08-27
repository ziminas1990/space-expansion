import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";

export type EngineSpecification = lowlevel.EngineSpecification;
export type CurrentThrust = lowlevel.CurrentThrust;

export class Engine extends BaseModule<lowlevel.Engine> {
    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Engine(session)]);
    }

    async get_specification()
        : Promise<[Status, EngineSpecification | undefined]>
    {
        return await this.run(async (session) => this._get_specification(session));
    }

    async get_thrust()
        : Promise<[Status, CurrentThrust | undefined]>
    {
        return await this.run(async (session) => this._get_thrust(session));
    }

    async set_thrust(x: number, y: number, thrust: number,
                     duration_ms: number = 0, at?: bigint): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => session.send_change_thrust(x, y, thrust, duration_ms, at));
    }

    private async _get_specification(session: lowlevel.Engine)
        : Promise<[Status, EngineSpecification | undefined]>
    {
        const send_status = await session.send_specification_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, spec] = await session.wait_specification();
        if (!status.is_ok() || !spec) {
            return [status.wrap("failed to get engine specification"), undefined];
        }
        return [Status.ok(), spec];
    }

    private async _get_thrust(session: lowlevel.Engine)
        : Promise<[Status, CurrentThrust | undefined]>
    {
        const send_status = await session.send_thrust_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        const [status, thrust] = await session.wait_thrust();
        if (!status.is_ok() || !thrust) {
            return [status.wrap("failed to get engine thrust"), undefined];
        }
        return [Status.ok(), thrust];
    }
}
