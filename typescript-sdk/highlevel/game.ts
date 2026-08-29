import * as midlevel from "../midlevel/index.js";
import { Status } from "../types/status.js";
import { EventEmitter } from "./events.js";

export type GameScore = midlevel.GameScore;
export type GameOver = midlevel.GameOver;

export type Events = {
    game_over: (result: GameOver) => Promise<void> | void;
}

export class Game extends EventEmitter<Events> {
    public result: GameOver | undefined = undefined;
    private stopped = false;
    private loop?: Promise<void>;

    constructor(private rpc: midlevel.Game) {
        super();
    }

    down_level(): midlevel.Game {
        return this.rpc;
    }

    async terminate(): Promise<void> {
        await this.release();
        await this.down_level().terminate();
    }

    async init(): Promise<Status> {
        this.stopped = false;
        this.result = undefined;
        this.loop = this.wait_loop();
        return Status.ok();
    }

    async release(): Promise<Status> {
        this.stopped = true;
        if (this.loop) {
            await this.loop;
            this.loop = undefined;
        }
        return Status.ok();
    }

    private async wait_loop(): Promise<void> {
        while (!this.stopped) {
            const [status, result] = await this.rpc.wait_game_over(100);
            if (status.is_ok() && result) {
                this.result = result;
                await this.emit("game_over", result);
                return;
            }
        }
    }
}
