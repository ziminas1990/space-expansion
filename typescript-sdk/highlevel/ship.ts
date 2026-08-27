import * as midlevel from "../midlevel/index.js";
import { Position, Status } from "../types/index.js";

export type { Position };
export type ShipState = midlevel.ShipState;

export class Ship {
    private ship: midlevel.Ship;
    private navigation: midlevel.Navigation;
    private commutator: midlevel.Commutator;

    private state: ShipState | undefined = undefined;
    private position: Position | undefined = undefined;
    private state_cached_at_ms: number | undefined = undefined;
    private position_cached_at_ms: number | undefined = undefined;

    private monitor: Promise<Status> | undefined = undefined;
    private commutator_monitor: Promise<Status> | undefined = undefined;
    private stop_monitoring: boolean = false;
    private stop_commutator_monitoring: boolean = false;

    constructor(
        open_session_cb: midlevel.OpenSessionCallback,
        public name: string)
    {
        this.ship = new midlevel.Ship(open_session_cb);
        this.navigation = new midlevel.Navigation(open_session_cb);
        this.commutator = new midlevel.Commutator(open_session_cb);
    }

    get slots() {
        return this.commutator.slots;
    }

    get modules() {
        return this.commutator.modules;
    }

    async init(): Promise<Status> {
        this.stop_monitoring = false;
        this.stop_commutator_monitoring = false;
        this.monitor = this.monitor_ship_state();
        this.commutator_monitor = this.monitor_nested_commutator();
        return Status.ok();
    }

    async reinit(open_session_cb: midlevel.OpenSessionCallback): Promise<Status> {
        await this.release();
        this.ship = new midlevel.Ship(open_session_cb);
        this.navigation = new midlevel.Navigation(open_session_cb);
        this.commutator = new midlevel.Commutator(open_session_cb);
        return await this.init();
    }

    async release(): Promise<Status> {
        this.stop_monitoring = true;
        this.stop_commutator_monitoring = true;
        await this.ship.terminate();
        await this.navigation.terminate();
        await this.commutator.terminate();
        if (this.monitor) {
            await this.monitor;
            this.monitor = undefined;
        }
        if (this.commutator_monitor) {
            await this.commutator_monitor;
            this.commutator_monitor = undefined;
        }
        this.state = undefined;
        this.position = undefined;
        this.state_cached_at_ms = undefined;
        this.position_cached_at_ms = undefined;
        return Status.ok();
    }

    async get_state(cache_expiring_ms: number = 50)
        : Promise<[Status, ShipState | undefined]>
    {
        if (this.state && !this.is_expired(this.state_cached_at_ms, cache_expiring_ms)) {
            return [Status.ok(), this.state];
        }
        const [status, state] = await this.ship.get_state();
        if (status.is_ok() && state) {
            this.cache_state(state);
        }
        return [status, state];
    }

    async get_position(at_us?: bigint, cache_expiring_ms: number = 10)
        : Promise<[Status, Position | undefined]>
    {
        if (!this.position
            || this.is_expired(this.position_cached_at_ms, cache_expiring_ms))
        {
            const [status, position] = await this.navigation.get_position();
            if (!status.is_ok() || !position) {
                return [status, undefined];
            }
            this.cache_position(position);
        }
        if (at_us !== undefined && this.position) {
            return [Status.ok(), this.extrapolate(this.position, at_us)];
        }
        return [Status.ok(), this.position];
    }

    predict_position(at_us?: bigint): Position | undefined {
        if (!this.position) {
            return undefined;
        }
        const at = at_us ?? this.predicted_server_us(this.position);
        return this.extrapolate(this.position, at);
    }

    private cache_state(state: ShipState) {
        this.state = state;
        this.state_cached_at_ms = performance.now();
        if (state.position) {
            this.cache_position(state.position, this.state_cached_at_ms);
        }
    }

    private cache_position(position: Position, cached_at_ms?: number) {
        this.position = position;
        this.position_cached_at_ms = cached_at_ms ?? performance.now();
    }

    private is_expired(cached_at_ms: number | undefined, expiration_ms: number): boolean {
        if (cached_at_ms == undefined) {
            return true;
        }
        return performance.now() - cached_at_ms > expiration_ms;
    }

    private predicted_server_us(position: Position): bigint {
        if (this.position_cached_at_ms == undefined) {
            return position.timestamp;
        }
        const elapsed_us = BigInt(Math.round(
            (performance.now() - this.position_cached_at_ms) * 1000));
        return position.timestamp + elapsed_us;
    }

    private extrapolate(position: Position, at_us: bigint): Position {
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

    private handle_update(update: ShipState) {
        if (this.state == undefined) {
            this.cache_state(update);
            return;
        }

        if (update.position) {
            this.state.position = update.position;
            this.cache_position(update.position);
        }
        if (update.weight) {
            this.state.weight = update.weight;
        }
        this.state.timestamp = update.timestamp;
        this.state_cached_at_ms = performance.now();
    }

    private async monitor_ship_state(): Promise<Status> {
        while (!this.stop_monitoring) {
            const [status, state] = await this.ship.get_state();
            if (status.is_ok() && state) {
                this.cache_state(state);
                await this.ship.monitoring(100, async (status, state) => {
                    if (status.is_ok() && state) {
                        this.handle_update(state);
                    }
                    return !this.stop_monitoring;
                });
            }
            if (this.stop_monitoring) {
                return Status.ok();
            }
            await new Promise((r) => setTimeout(r, 200));
        }
        return Status.ok();
    }

    private async monitor_nested_commutator(): Promise<Status> {
        while (!this.stop_commutator_monitoring) {
            const [status] = await this.commutator.get_all_modules_info();
            if (status.is_ok()) {
                await this.commutator.monitoring(async () => {
                    return !this.stop_commutator_monitoring;
                });
            }
            if (this.stop_commutator_monitoring) {
                return Status.ok();
            }
            await new Promise((r) => setTimeout(r, 1000));
        }
        return Status.ok();
    }

}
