import {
    follow_flight_plan,
    type Position,
    type Ship,
    type SystemClock,
} from "@spx/sdk/highlevel";
import { build_plan, type PlannedShip } from "@spx/sdk/utils";
import { find_most_powerful_hover_engine } from "../equipment.js";
import { BaseTask } from "./base_task.js";

// Plan from a second ahead. Each command is sent 250 ms before it is due,
// so planning has to finish before that first send.
const PLAN_LEAD_US = 1_000_000;

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
        const engine = await find_most_powerful_hover_engine(this.ship);
        if (!engine) {
            this.add_journal_record("Can't get hover engine!");
            return false;
        }

        const [spec_status, engine_spec] = await engine.get_specification();
        const [state_status, ship_state] = await this.ship.get_state(0);
        const [ship_spec_status, ship_spec] = await this.ship.get_specification();
        if (!spec_status.is_ok() || !engine_spec
            || !state_status.is_ok() || !ship_state
            || ship_state.weight === undefined
            || !ship_spec_status.is_ok() || !ship_spec)
        {
            this.add_journal_record("Can't get ship's state or hover engine spec!");
            return false;
        }

        const stopped = await engine.set_thrust(0, 0);
        if (!stopped.is_ok()) {
            this.add_journal_record("Can't stop the hover engine!");
            return false;
        }

        const [time_status, now_us] = await this.system_clock.time();
        if (!time_status.is_ok() || now_us === undefined) {
            return false;
        }

        const [position_status, position] = await this.ship.get_position(
            now_us + PLAN_LEAD_US,
            0,
        );
        if (!position_status.is_ok() || !position) {
            return false;
        }

        const flight_ship: PlannedShip = {
            mass: ship_state.weight,
            position,
            orientation: [ship_state.orientation[0], ship_state.orientation[1]],
            engine_max_thrust: engine_spec.max_thrust,
            max_rotate_speed: ship_spec.max_rotation_speed,
        };
        const target = this.intercept_course
            ? this.target
            : { ...this.target, timestamp: position.timestamp };

        let plan;
        try {
            plan = build_plan(flight_ship, target);
        } catch (error) {
            const reason = error instanceof Error ? error.message : String(error);
            this.add_journal_record(`Can't build a flight plan: ${reason}`);
            return false;
        }

        const status = await follow_flight_plan(
            this.ship.down_level("ship"),
            engine.down_level(),
            plan,
            this.system_clock.down_level(),
            ship_spec.max_rotation_speed,
        );
        if (!status.is_ok()) {
            this.add_journal_record(`Flight failed: ${status.what()}`);
            return false;
        }
        return true;
    }
}
