import { BaseModule, OpenSessionCallback } from "./base_module.js";
import * as lowlevel from "../lowlevel/index.js";
import { Status } from "../types/status.js";


export class Commutator extends BaseModule<lowlevel.Commutator> {
    constructor(open_session_callback: OpenSessionCallback,
                private root_session: lowlevel.RootSession)
    {
        super(open_session_callback,
              async (session) => this.create_interface(session));
    }

    async total_slots(): Promise<[Status, number | undefined]> {
        return await this.run(this._total_slots);
    }

    private async create_interface(session: lowlevel.Session): Promise<[Status, lowlevel.Commutator]> {
        return [Status.ok(), new lowlevel.Commutator(session, this.root_session)];
    }

    private async _total_slots(
        session: lowlevel.Commutator,
        timeout_ms: number = 500): Promise<[Status, number]>
    {
        const send_status = await session.send_total_slots_request();
        if (!send_status.is_ok()) {
            return [send_status, 0];
        }
        return await session.wait_total_slots_response(timeout_ms);
    }
}