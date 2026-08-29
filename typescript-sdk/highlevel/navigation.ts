import * as midlevel from "../midlevel/index.js";
import { Position, Status } from "../types/index.js";
import { Cached } from "../utils/cache.js";

export type { Position };

const DEFAULT_CACHE_MS = 20;

// Linear kinematics: p(t) = p0 + v * dt. Velocity is treated as constant.
export function extrapolate(position: Position, at_us: bigint): Position {
    const dt_sec = Number(at_us - position.timestamp) / 1e6;
    return {
        timestamp: at_us,
        point: [
            position.point[0] + position.velocity[0] * dt_sec,
            position.point[1] + position.velocity[1] * dt_sec,
        ],
        velocity: [position.velocity[0], position.velocity[1]],
    };
}

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
            return [Status.ok(), extrapolate(position, at_us)];
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
        return extrapolate(position, at_us);
    }

    async release(): Promise<Status> {
        this.position.reset();
        return Status.ok();
    }
}
