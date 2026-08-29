// Follows a remote (usually server) clock from a local sample.
// predicted = remote + (now - local). Not monotonic.
export class TimePoint {
    private remote_us: bigint;
    private local_ms: number | undefined;

    constructor(remote_us: bigint, is_static: boolean = false) {
        this.remote_us = remote_us;
        this.local_ms = is_static ? undefined : performance.now();
    }

    update(us: bigint): void {
        this.remote_us = us;
        if (this.local_ms !== undefined) {
            this.local_ms = performance.now();
        }
    }

    us(): bigint {
        return this.remote_us;
    }

    predict_us(): bigint {
        if (this.local_ms === undefined) {
            return this.remote_us;
        }
        const dt_us = BigInt(Math.round((performance.now() - this.local_ms) * 1000));
        return this.remote_us + dt_us;
    }

    expired(ms: number): boolean {
        if (this.local_ms === undefined) {
            return true;
        }
        return performance.now() - this.local_ms > ms;
    }
}
