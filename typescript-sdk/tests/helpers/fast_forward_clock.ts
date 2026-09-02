import type { SystemClock } from "../../highlevel/index.js";
import type { Status } from "../../types/status.js";
import type { IngameClock } from "../ingame_clock.js";

export class FastForwardClock {
    constructor(
        private systemClock: SystemClock,
        private clock: IngameClock,
        private multiplier: number,
    ) {}

    async wait_until(
        time_us: bigint,
        timeout_ms?: number,
    ): Promise<[Status, bigint | undefined]> {
        await this.clock.fastForward(this.multiplier, 1_000);
        try {
            return await this.systemClock.wait_until(
                time_us,
                timeout_ms ?? this.waitTimeoutMs(time_us),
            );
        } finally {
            await this.clock.play();
        }
    }

    private waitTimeoutMs(time_us: bigint): number {
        const remaining_us = Number(time_us - this.systemClock.now_us());
        const real_ms = remaining_us / this.multiplier / 1_000;
        return Math.max(1_000, real_ms * 1.5);
    }
}
