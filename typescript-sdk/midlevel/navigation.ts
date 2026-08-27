import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";
import { Position } from "../types/index.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";

export class Navigation extends BaseModule<lowlevel.Navigation> {

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Navigation(session)]);
    }

    async get_position(): Promise<[Status, Position | undefined]> {
        return await this.run(this._get_position);
    }

    async _get_position(session: lowlevel.Navigation)
        : Promise<[Status, Position | undefined]>
    {
        const send_status = await session.send_position_request();
        if (!send_status.is_ok()) {
            return [send_status, undefined];
        }
        return await session.wait_position();
    }
}
