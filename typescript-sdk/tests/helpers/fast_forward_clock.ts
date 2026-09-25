import type { SystemClock } from "../../highlevel/index.js";
import type { Status } from "../../types/status.js";
import type { IngameClock } from "../ingame_clock.js";

export class FastForwardClock {
    constructor(
        private systemClock: SystemClock,
        private clock: IngameClock,
        private multiplier: number,
    ) {}

    async get_time(): Promise<[Status, { ingame_us: number } | undefined]> {
        const [status, now] = await this.systemClock.time(false);
        if (!status.is_ok() || now === undefined) {
            return [status, undefined];
        }
        return [status, { ingame_us: now }];
    }

    async wait_until(
        time_us: number,
        timeout_ms?: number,
    ): Promise<[Status, number | undefined]> {
        await this.clock.fastForward(this.multiplier, 1_000);
        try {
            const [status, now] = await this.systemClock.time();
            if (!status.is_ok() || now === undefined) {
                return [status, undefined];
            }
            return await this.systemClock.wait_until(
                time_us,
                timeout_ms ?? this.waitTimeoutMs(time_us - now),
            );
        } finally {
            await this.clock.play();
        }
    }

    async wait_for(
        period_us: number,
    ): Promise<[Status, number | undefined]> {
        await this.clock.fastForward(this.multiplier, 1_000);
        try {
            return await this.systemClock.wait_for(
                period_us,
                this.waitTimeoutMs(period_us),
            );
        } finally {
            await this.clock.play();
        }
    }

    private waitTimeoutMs(remaining_us: number): number {
        const remaining_s = remaining_us / 1_000_000;
        // NOTE: local machine MAY not be able to run simulation with current
        // fast-forward multiplier (say, 50x). In this case, test may fail by
        // timeout in wait_until/wait_for.
        // We assume, that local machine is able to run simulation with at least
        // 10x speed of the server.
        return Math.max(
            5_000,
            remaining_s / Math.min(10, this.multiplier) * 1_000,
        );
    }
}
