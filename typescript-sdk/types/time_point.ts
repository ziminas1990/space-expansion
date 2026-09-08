// Follows a remote (usually server) clock from a local sample.
// predicted = remote + (now - local). Not monotonic.
export class TimePoint {
    private remote_us: number;
    private local_ms: number | undefined;

    constructor(remote_us: number, is_static: boolean = false) {
        this.remote_us = remote_us;
        this.local_ms = is_static ? undefined : performance.now();
    }

    update(us: number): void {
        this.remote_us = us;
        if (this.local_ms !== undefined) {
            this.local_ms = performance.now();
        }
    }

    us(): number {
        return this.remote_us;
    }

    predict_us(): number {
        if (this.local_ms === undefined) {
            return this.remote_us;
        }
        const dt_us = Math.round((performance.now() - this.local_ms) * 1000);
        return this.remote_us + dt_us;
    }

    expired(ms: number): boolean {
        if (this.local_ms === undefined) {
            return true;
        }
        return performance.now() - this.local_ms > ms;
    }
}
