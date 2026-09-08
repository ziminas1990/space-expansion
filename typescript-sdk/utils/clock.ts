// Microseconds since process/page start. All Clock timestamps are
// microseconds: local physical, server physical, and ingame.
export function local_now_us(): number {
    return performance.now() * 1000;
}

// Linear map is used to map local physical time to remote physical time,
// since we know that they both monotonically increase with the same rate.
// It assumes, that at some point we may get a observation, that is lower
// then our prediction. In this case, we update the reference point.
// The 'map()' call is not guaranteed to be monotonic after the reference
// update.
// But 'monotonic_now()' is guaranteed to be monotonic. In case of reference
// point is changed, it will just return the same value for some time.
class LinearTimeMapper {

    private base: {
        local_ts: number;
        remote_ts: number;
    } | undefined;
    private last_now: number | undefined;

    observed(local_ts: number, remote_ts: number) {
        if (this.base == undefined) {
            this.base = { local_ts, remote_ts };
            return;
        }
        if (remote_ts < this.base.remote_ts || local_ts < this.base.local_ts) {
            // Ignore the outdated observation
            return;
        }
        const predicted = this.map(local_ts);
        if (predicted !== undefined && remote_ts < predicted) {
            // this observation is better, because is is closer to the
            // remote time
            this.base = { local_ts, remote_ts };
        }
    }

    // Note: not guaranteed to be monotonic
    map(local_ts: number): number | undefined {
        if (this.base === undefined) {
            return undefined;
        }
        return this.base.remote_ts + (local_ts - this.base.local_ts);
    }

    // Guaranteed to be monotonic, but may "freeze" for some time
    monotonic_now(): number | undefined {
        if (this.base === undefined) {
            return undefined;
        }
        const predicted_now = this.map(local_now_us());
        if (predicted_now === undefined) {
            return undefined;
        } else if (this.last_now === undefined) {
            this.last_now = predicted_now;
        } else if (predicted_now < this.last_now) {
            return this.last_now;
        }
        this.last_now = predicted_now;
        return predicted_now;
    }
}

// Smooth time mapper maps linear monotoinc time with non-lnear monotonic time.
// It is used to map physical server time with ingame time, that may go slower
// or faster than the physical time.
// Smooth mapper tries to smoothly compensate the difference.
class SmoothTimeMapper {

    static readonly ALIGN_INTERVAL_US = 500_000;
    static readonly MINIMAL_OBSERVATION_INTERVAL_US = 10_000;

    private last_reference: {
        physical_ts: number;
        simulation_ts: number;
    } | undefined;
    private rate: number = 1;

    // NOTE: physical_ts and simulation_ts are expected to be monotonic
    observed(physical_ts: number, simulation_ts: number) {
        if (this.last_reference === undefined) {
            this.last_reference = { physical_ts, simulation_ts };
            return;
        }

        const physical_diff = physical_ts - this.last_reference.physical_ts;
        if (physical_diff <= SmoothTimeMapper.MINIMAL_OBSERVATION_INTERVAL_US) {
            return;
        }

        const simulation_diff = simulation_ts - this.last_reference.simulation_ts;
        if (simulation_diff <= 0) {
            return;
        }

        const instant_rate = simulation_diff / physical_diff;
        const weight = Math.min(1, physical_diff / SmoothTimeMapper.ALIGN_INTERVAL_US);
        this.rate = this.rate * (1 - weight) + instant_rate * weight;
        this.last_reference = { physical_ts, simulation_ts };
    }

    // Note: not guaranteed to be monotonic
    map(physical_ts: number): number | undefined {
        if (this.last_reference === undefined) {
            return undefined;
        }

        const diff = physical_ts - this.last_reference.physical_ts;
        return this.last_reference.simulation_ts + diff * this.rate;
    }
}

// local → LinearTimeMapper → server → SmoothTimeMapper → ingame
export class Clock {
    private readonly linear = new LinearTimeMapper();
    private readonly smooth = new SmoothTimeMapper();
    private last_ingame_now: number | undefined;

    // local_ts is local physical time (use local_now_us()). server_ts is
    // remote physical time; ingame_ts is simulation time. All microseconds.
    observe(local_ts: number, server_ts: number, ingame_ts: number): void {

        const predicted_before = Math.round((this.predict(local_ts) || 0) / 1000);

        this.linear.observed(local_ts, server_ts);
        this.smooth.observed(server_ts, ingame_ts);

        const predicted_after = Math.round((this.predict(local_ts) || 0) / 1000);

        const diff = Math.round((ingame_ts - (this.predict(local_ts) || 0)) / 1000);
        console.log(`predicted_before: ${predicted_before}, predicted_after: ${predicted_after} , diff: ${diff}, received: ${ingame_ts}`);
    }

    predict(local_ts: number): number | undefined {
        const server_ts = this.linear.map(local_ts);
        if (server_ts === undefined) {
            return undefined;
        }
        return this.smooth.map(server_ts);
    }

    // Guaranteed to be monotonic, but may "freeze" for some time.
    monotonic_now(): number | undefined {
        const server_ts = this.linear.monotonic_now();
        if (server_ts !== undefined) {
            const ingame_ts = this.smooth.map(server_ts);
            if (ingame_ts !== undefined) {
                if (this.last_ingame_now === undefined) {
                    this.last_ingame_now = ingame_ts;
                } else if (ingame_ts < this.last_ingame_now) {
                    return this.last_ingame_now;
                }
                this.last_ingame_now = ingame_ts;
            }
        }
        return this.last_ingame_now;
    }
}
