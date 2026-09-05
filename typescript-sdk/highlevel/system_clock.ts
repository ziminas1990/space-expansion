import * as midlevel from "#sdk/midlevel/index.js";
import { ServerTimestamp, Status, TimePoint } from "#sdk/types/index.js";
import type { BaseModule } from "./base_module.js";

export type MonitoringCallback =
    (time_us: bigint) => Promise<boolean> | boolean;

const INITIAL_SYNC_SAMPLES = 10;
const MIN_WAIT_TIMEOUT_MS = 100;
const AUTO_TIMEOUT_FACTOR = 1.2;
const SHORT_WAIT_DT_US = 10_000;

export class SystemClock implements BaseModule {
    readonly type = midlevel.ModuleType.SYSTEM_CLOCK;
    // Predicts current real time on the server (set by initial_sync).
    private server_time: TimePoint | undefined = undefined;
    // Predicts current ingame time on the server. Same instance is returned
    // by time_point() and updated in place whenever a timestamp arrives.
    private ingame_time = new TimePoint(0n);
    private loops = new Set<Promise<Status>>();
    private tokens = new Set<{ stop: boolean }>();
    private in_callback = false;

    constructor(
        private rpc: midlevel.SystemClock,
        readonly name: string,
    ) {}

    async reinit(rpc: midlevel.MidlevelModule): Promise<Status> {
        if (!midlevel.is_module(rpc, midlevel.ModuleType.SYSTEM_CLOCK)) {
            return Status.fail("expected SystemClock");
        }
        await this.release();
        this.rpc = rpc;
        return Status.ok();
    }

    down_level(): midlevel.SystemClock {
        return this.rpc;
    }

    time_point(): TimePoint {
        return this.ingame_time;
    }

    now_us(): bigint {
        return this.ingame_time.predict_us();
    }

    async initial_sync(): Promise<Status> {
        const started_at_ms = performance.now();
        const points: ServerTimestamp[] = [];
        for (let i = 0; i < INITIAL_SYNC_SAMPLES; i++) {
            const [status, timestamp] = await this.rpc.get_time();
            if (status.is_ok() && timestamp) {
                points.push(timestamp);
            }
        }
        const last = points.at(-1);
        if (!last) {
            return Status.fail("failed to get server time");
        }
        const elapsed_us = (performance.now() - started_at_ms) * 1000;
        const rtt_us = elapsed_us / points.length;
        this.server_time = new TimePoint(
            last.real_us + BigInt(Math.round(rtt_us / 2)));
        this.ingame_time.update(last.ingame_us);
        return Status.ok();
    }

    async sync(): Promise<Status> {
        const [status, timestamp] = await this.rpc.get_time();
        if (!status.is_ok() || !timestamp) {
            return status.wrap("failed to sync system clock");
        }
        this.apply_timestamp(timestamp);
        return Status.ok();
    }

    async time(
        predict: boolean = true,
    ): Promise<[Status, bigint | undefined]> {
        const status = await this.sync();
        if (!status.is_ok()) {
            return [status, undefined];
        }
        return [Status.ok(), predict ? this.now_us() : this.ingame_time.us()];
    }

    async wait_until(
        time_us: bigint,
        timeout_ms?: number,
    ): Promise<[Status, bigint | undefined]> {
        const timeout = timeout_ms ?? this.auto_timeout_until(time_us);
        const [status, timestamp] = await this.rpc.wait_until(time_us, timeout);
        if (!status.is_ok() || !timestamp) {
            return [status, undefined];
        }
        this.apply_timestamp(timestamp);
        return [Status.ok(), this.now_us()];
    }

    async wait_for(
        period_us: bigint,
        timeout_ms?: number,
    ): Promise<[Status, bigint | undefined]> {
        const timeout = timeout_ms ?? this.auto_timeout_for(period_us);
        const [status, timestamp] = await this.rpc.wait_for(period_us, timeout);
        if (!status.is_ok() || !timestamp) {
            return [status, undefined];
        }
        this.apply_timestamp(timestamp);
        return [Status.ok(), this.now_us()];
    }

    async monitoring(
        interval_ms: number,
        callback?: MonitoringCallback,
    ): Promise<Status> {
        const token = { stop: false };
        this.tokens.add(token);
        const loop = this.monitor_loop(interval_ms, callback, token);
        this.loops.add(loop);
        try {
            return await loop;
        } finally {
            this.tokens.delete(token);
            this.loops.delete(loop);
        }
    }

    async release(): Promise<Status> {
        for (const token of this.tokens) {
            token.stop = true;
        }
        await this.rpc.terminate();
        if (!this.in_callback) {
            await Promise.all(this.loops);
        }
        return Status.ok();
    }

    private apply_timestamp(timestamp: ServerTimestamp): bigint {
        const deviation_us = this.server_time
            ? this.server_time.predict_us() - timestamp.real_us
            : 0n;
        this.ingame_time.update(timestamp.ingame_us + deviation_us);
        return this.ingame_time.us();
    }

    private auto_timeout_until(time_us: bigint): number {
        const dt_us = Number(time_us - this.ingame_time.predict_us());
        if (dt_us > SHORT_WAIT_DT_US) {
            return (dt_us * AUTO_TIMEOUT_FACTOR) / 1000;
        }
        return MIN_WAIT_TIMEOUT_MS;
    }

    private auto_timeout_for(period_us: bigint): number {
        return Math.max(
            (Number(period_us) * AUTO_TIMEOUT_FACTOR) / 1000,
            MIN_WAIT_TIMEOUT_MS);
    }

    private async monitor_loop(
        interval_ms: number,
        callback: MonitoringCallback | undefined,
        token: { stop: boolean },
    ): Promise<Status> {
        while (!token.stop) {
            const status = await this.rpc.monitoring(interval_ms, async (timestamp) => {
                if (!timestamp) {
                    // a heartbeat by lower level
                    return !token.stop;
                }
                const time_us = this.apply_timestamp(timestamp);
                this.in_callback = true;
                try {
                    if (callback) {
                        return (await callback(time_us)) && !token.stop;
                    }
                } finally {
                    this.in_callback = false;
                }
                return !token.stop;
            });
            if (token.stop) {
                return Status.ok();
            }
            if (status.is_ok()) {
                // Callback asked to stop this session.
                return Status.ok();
            }
            await new Promise((resolve) => setTimeout(resolve, 100));
        }
        return Status.ok();
    }
}
