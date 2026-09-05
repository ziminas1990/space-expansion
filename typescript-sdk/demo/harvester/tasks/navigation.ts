import {
    approach_to_plan,
    follow_flight_plan,
    prepare_flight_plan,
    type Position,
    type Ship,
    type SystemClock,
} from "@spx/sdk/highlevel";
import { almostNull } from "../util.js";
import { find_most_powerful_engine } from "../equipment.js";
import { BaseTask } from "./base_task.js";

export class MoveTo extends BaseTask {
    private readonly ship: Ship;
    private readonly target: Position;
    private readonly intercept_course: boolean;

    constructor(
        ship: Ship,
        target: Position,
        system_clock: SystemClock,
        intercept_course = true,
        name = "MoveTo",
    ) {
        super(name, system_clock);
        this.ship = ship;
        this.target = target;
        this.intercept_course = intercept_course;
    }

    protected async _impl(): Promise<boolean> {
        const engine = await find_most_powerful_engine(this.ship);
        if (!engine) {
            this.add_journal_record("Can't get engine!");
            return false;
        }

        const [spec_status, engine_spec] = await engine.get_specification();
        const [state_status, ship_state] = await this.ship.get_state();
        if (!spec_status.is_ok() || !engine_spec
            || !state_status.is_ok() || !ship_state
            || ship_state.weight === undefined)
        {
            this.add_journal_record("Can't get ship's state or engine spec!");
            return false;
        }

        const amax = engine_spec.max_thrust / ship_state.weight;

        await engine.set_thrust(0, 0, 0);

        const [time_status, now_us] = await this.system_clock.time();
        if (!time_status.is_ok() || now_us === undefined) {
            return false;
        }

        const [position_status, position] = await this.ship.get_position(
            now_us + 100_000n,
        );
        if (!position_status.is_ok() || !position) {
            return false;
        }

        const plan = this.intercept_course && !almostNull(this.target.velocity)
            ? approach_to_plan(position, this.target, amax)
            : prepare_flight_plan(
                position,
                { ...this.target, timestamp: position.timestamp },
                amax,
            );
        if (!plan) {
            this.add_journal_record("Can't build a flight plan!");
            return false;
        }

        const status = await follow_flight_plan(
            this.ship, engine, plan, this.system_clock);
        return status.is_ok();
    }
}
