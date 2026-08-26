import * as msg from "../Protocol_pb.js";
import * as types from "../types/index.js";
import { Session } from "./session.js";


export type GameScore = {
    player: string;
    score: number;
}

export type GameOver = {
    leaders: GameScore[];
}

export class Game {

    constructor(private session: Session) {}

    async wait_game_over_report(timeout: number = 500)
    : Promise<[types.Status, GameOver | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "gameOverReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        const report = response.choice.value;
        return [types.Status.ok(), {
            leaders: report.leaders.map((score) => ({
                player: score.player,
                score: score.score,
            })),
        }];
    }

    private async wait(timeout_ms: number = 500)
    : Promise<[types.Status, msg.IGame | undefined]>
    {
        const [status, response] = await this.session.wait(timeout_ms);
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "game") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), response.choice.value];
    }

}
