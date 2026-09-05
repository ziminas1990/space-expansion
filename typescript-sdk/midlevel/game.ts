import * as lowlevel from "#sdk/lowlevel/index.js";
import { Status } from "#sdk/types/status.js";
import { BaseModule, OpenSessionCallback } from "./base_module.js";

export type GameScore = lowlevel.GameScore;
export type GameOver = lowlevel.GameOver;

export class Game extends BaseModule<lowlevel.Game> {

    constructor(open_session_callback: OpenSessionCallback)
    {
        super(open_session_callback,
              async (session) => [Status.ok(), new lowlevel.Game(session)]);
    }

    async wait_game_over(timeout_ms: number = 500)
        : Promise<[Status, GameOver | undefined]>
    {
        return await this.run(
            async (session) => session.wait_game_over_report(timeout_ms));
    }
}
