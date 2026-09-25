import type { HoverEngine, Ship } from "#sdk/midlevel/index.js";
import { Status } from "#sdk/types/status.js";
import type { FlightPlan } from "#sdk/utils/flight_planner.js";

// The navigation test substitutes a fast-forwarding clock with these two calls.
type FlightClock = {
    get_time(): Promise<[Status, { ingame_us: number } | undefined]>;
    wait_until(
        time_us: number,
        timeout_ms: number,
    ): Promise<[Status, unknown]>;
};

// Send the next maneuver this long before the server should apply it.
// The command still carries the maneuver's own timestamp.
const COMMAND_LEAD_US = 250_000;

// The hover engine is polled about every 25 ms, so a command is applied up to
// one poll after its timestamp. The burn then runs its full duration and ends
// that much later than the plan.
const APPLICATION_SLACK_US = 50_000;

// Follow a flight one maneuver at a time. Each rotate or thrust is sent
// 250 ms before it should take effect, then the clock waits for the next one.
// A turn's acknowledgement arrives only when the server applies it, so that
// wait stays off this loop and cannot delay the following command.
export async function follow_flight_plan(
    ship: Ship,
    engine: HoverEngine,
    plan: FlightPlan,
    clock: FlightClock,
    max_rotate_speed: number,
): Promise<Status> {
    const maneuvers = [...plan.maneuvers].sort(
        (left, right) => left.start_at - right.start_at,
    );
    const turns: Promise<Status>[] = [];
    let end = 0;

    for (const item of maneuvers) {
        const start = Math.round(item.start_at);
        const item_end = Math.round(
            item.start_at + item.maneuver.duration_ms * 1_000,
        );
        if (item_end > end) {
            end = item_end;
        }

        if (item.maneuver.kind === "rotate") {
            const ready = await wait_on(clock, start - COMMAND_LEAD_US);
            if (!ready.is_ok()) {
                return finish(
                    turns,
                    ready.wrap("failed to wait for the next maneuver"),
                );
            }
            turns.push(ship.rotate(
                item.maneuver.target[0],
                item.maneuver.target[1],
                max_rotate_speed,
                start,
            ));
            continue;
        }

        const thrust = Math.round(item.maneuver.thrust);
        const duration_ms = thrust === 0
            ? 0
            : Math.round(item.maneuver.duration_ms);
        if (thrust !== 0 && duration_ms <= 0) {
            continue;
        }

        const ready = await wait_on(clock, start - COMMAND_LEAD_US);
        if (!ready.is_ok()) {
            return finish(
                turns,
                ready.wrap("failed to wait for the next maneuver"),
            );
        }
        const status = await engine.set_thrust(thrust, duration_ms, start);
        if (!status.is_ok()) {
            return finish(turns, status.wrap("failed to set thrust"));
        }
    }

    const turned = await finish(turns, Status.ok());
    if (!turned.is_ok()) {
        return turned;
    }
    if (maneuvers.length === 0) {
        return Status.ok();
    }
    const arrived = await wait_on(clock, end + APPLICATION_SLACK_US);
    if (!arrived.is_ok()) {
        return arrived.wrap("failed to wait for arrival");
    }
    return Status.ok();
}

async function wait_on(clock: FlightClock, time_us: number): Promise<Status> {
    const at = Math.round(time_us);
    const [time_status, now] = await clock.get_time();
    if (!time_status.is_ok() || !now) {
        return time_status.wrap("failed to read the clock");
    }
    if (now.ingame_us >= at) {
        return Status.ok();
    }
    const remaining_ms = (at - now.ingame_us) / 1_000;
    // Ingame time can lag real time when the simulation is busy. The wait is
    // on the server clock, so the client timeout has to outlast that lag.
    const timeout_ms = Math.max(5_000, Math.ceil(remaining_ms * 3));
    const [status] = await clock.wait_until(at, timeout_ms);
    if (!status.is_ok()) {
        return status;
    }
    return Status.ok();
}

async function finish(turns: Promise<Status>[], status: Status): Promise<Status> {
    let result = status;
    for (const turn of turns) {
        const turn_status = await turn;
        if (!turn_status.is_ok() && result.is_ok()) {
            result = turn_status.wrap("failed to rotate");
        }
    }
    return result;
}
