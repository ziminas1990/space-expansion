import { create } from "@bufbuild/protobuf";
import * as msg from "#sdk/Protocol_pb.js";
import * as types from "#sdk/types/index.js";
import { Session } from "./session.js";

export type Score = {
    player: string;
    score: number;
};

export type GameOver = {
    leaders: Score[];
};

export class Game {

    constructor(private session: Session) {}

    async send_monitor_request(): Promise<types.Status> {
        const request = create(msg.IGameSchema, {
            choice: { case: "monitor", value: true },
        });
        return this.send(request);
    }

    async wait_game_over(timeout: number = 500)
    : Promise<[types.Status, GameOver | undefined]>
    {
        const [status, response] = await this.wait(timeout);
        if (status.is_timeout()) {
            return [status, undefined];
        }
        if (!status.is_ok() || !response) {
            return [status.wrap("no response"), undefined];
        }
        if (response.choice.case != "gameOverReport") {
            return [types.Status.fail(`got unexpected message ${response.choice.case}`),
                    undefined];
        }
        return [types.Status.ok(), {
            leaders: response.choice.value.leaders.map((leader) => ({
                player: leader.player,
                score: leader.score,
            })),
        }];
    }

    private async send(request: msg.IGame): Promise<types.Status> {
        const message = create(msg.MessageSchema, {
            choice: { case: "game", value: request },
        });
        return this.session.send(message);
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
