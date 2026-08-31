import { AdministratorClock } from "../midlevel/index.js";
import { expectOk, expectStatus } from "./helpers/status.js";

export class IngameClock {
    private timeManualControl = false;
    private timeWheel: Promise<void> | undefined;
    private timeMultiplier = 1;
    private timeGranularityUs = 1_000;

    constructor(private clock: AdministratorClock){}

    async time(): Promise<bigint> {
        return expectOk(await this.clock.get_time(), "clock time");
    }

    async stop(): Promise<bigint> {
        await this.shutdown();
        expectStatus(
            await this.clock.switch_to_debug_mode(),
            "switch to debug mode",
        );
        return await this.time();
    }

    async play(): Promise<void> {
        await this.stop();
        expectStatus(
            await this.clock.switch_to_real_time(),
            "switch to real time",
        );
    }

    async proceed(
        proceedMs: number,
        timeoutMs: number,
        granularityUs = 1_000,
    ): Promise<bigint> {
        await this.stop();
        this.timeGranularityUs = granularityUs;
        expectStatus(
            await this.clock.set_tick_duration(this.timeGranularityUs),
            "set tick duration",
        );
        return await this.proceedTime(proceedMs, timeoutMs);
    }

    async fastForward(
        speedMultiplier = 1,
        granularityUs = 1_000,
    ): Promise<void> {
        await this.stop();
        this.timeMultiplier = speedMultiplier;
        this.timeGranularityUs = granularityUs;
        this.timeManualControl = true;
        this.timeWheel = this.wheelOfTime();
    }

    async sleep(timeS: number): Promise<void> {
        await this.proceed(
            Math.trunc(timeS * 1_000),
            Math.max(1_000, timeS * 1_000),
            this.timeGranularityUs,
        );
    }

    async shutdown(): Promise<void> {
        if (!this.timeManualControl && this.timeWheel === undefined) {
            return;
        }
        this.timeManualControl = false;
        if (this.timeWheel !== undefined) {
            await this.timeWheel;
            this.timeWheel = undefined;
        }
    }

    private async proceedTime(
        proceedMs: number,
        timeoutMs: number,
    ): Promise<bigint> {
        const proceedUs = proceedMs * 1_000;
        let ticks = Math.trunc(proceedUs / this.timeGranularityUs);
        if (proceedUs % this.timeGranularityUs > 0) {
            ticks += 1;
        }
        return expectOk(
            await this.clock.proceed_ticks(ticks, timeoutMs),
            "proceed ticks",
        );
    }

    private async wheelOfTime(): Promise<void> {
        try {
            const ingameBeginUs = await this.time();
            let ingameNowUs = ingameBeginUs;
            const beginMs = Date.now();
            expectStatus(
                await this.clock.set_tick_duration(this.timeGranularityUs),
                "set tick duration",
            );

            while (this.timeManualControl) {
                const deltaMs = Date.now() - beginMs;
                const ingameDeltaMs = Number(ingameNowUs - ingameBeginUs) / 1_000;
                const proceedIntervalMs = this.timeMultiplier * deltaMs
                    - ingameDeltaMs;
                if (proceedIntervalMs < 1) {
                    await new Promise<void>((resolve) =>
                        setTimeout(resolve, 10)
                    );
                    continue;
                }
                ingameNowUs = await this.proceedTime(
                    proceedIntervalMs,
                    Math.max(2_000, proceedIntervalMs * 3),
                );
            }
        } catch {
            this.timeManualControl = false;
        }
    }
}
