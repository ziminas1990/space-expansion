import * as midlevel from "#sdk/midlevel/index.js";
import { Position, Status } from "#sdk/types/index.js";
import { Cached } from "#sdk/utils/cache.js";
import { predict_position } from "#sdk/utils/predictor.js";

export type { Position };

const DEFAULT_CACHE_MS = 20;

export class Navigation {
    private position = new Cached<Position>();

    constructor(private rpc: midlevel.Navigation) {}

    down_level(): midlevel.Navigation {
        return this.rpc;
    }

    async get_position(
        at_us?: bigint,
        cache_expiring_ms: number = DEFAULT_CACHE_MS,
    ): Promise<[Status, Position | undefined]> {
        let position = this.position.get(cache_expiring_ms);
        if (!position) {
            const [status, fetched] = await this.rpc.get_position();
            if (!status.is_ok() || !fetched) {
                return [status, undefined];
            }
            this.position.set(fetched);
            position = fetched;
        }
        if (at_us !== undefined) {
            return [Status.ok(), predict_position(position, at_us)];
        }
        return [Status.ok(), position];
    }

    update_from(position: Position): void {
        if (position.timestamp > (this.position.get(Infinity)?.timestamp ?? 0n)) {
            this.position.set(position);
        }
    }

    predict_position(at_us: bigint): Position | undefined {
        const position = this.position.get(Infinity);
        if (!position) {
            return undefined;
        }
        return predict_position(position, at_us);
    }

    async release(): Promise<Status> {
        await this.rpc.terminate();
        this.position.reset();
        return Status.ok();
    }
}
