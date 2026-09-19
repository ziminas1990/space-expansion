import * as lowlevel from "#sdk/lowlevel/index.js";
import { Status } from "#sdk/types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";
import { ModuleType } from "./module_type.js";

export type Score = lowlevel.GameScore;
export type GameOver = lowlevel.GameOver;
export type MonitoringCallback =
    (update: GameOver | undefined) => Promise<boolean>;

export class Game extends BaseModule<lowlevel.Game> {
    readonly type = ModuleType.GAME;

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Game(session)]);
    }

    async monitoring(
        callback: MonitoringCallback,
        heartbeat_ms: number = 200): Promise<Status>
    {
        return await this.run_no_return(
            async (session) => this._monitoring(session, callback, heartbeat_ms),
            true);
    }

    private async _monitoring(
        session: lowlevel.Game,
        callback: MonitoringCallback,
        heartbeat_ms: number): Promise<Status>
    {
        const send_status = await session.send_monitor_request();
        if (!send_status.is_ok()) {
            return send_status.wrap("failed to send monitor request");
        }

        while (true) {
            const [status, update] = await session.wait_game_over(heartbeat_ms);
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
