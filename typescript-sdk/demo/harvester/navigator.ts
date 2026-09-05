import type { Position, Ship as RemoteShip, SystemClock } from "@spx/sdk/highlevel";
import { MoveTo } from "./tasks/navigation.js";

export class Navigator {
    readonly name: string;
    private readonly ship: RemoteShip;
    private readonly system_clock: SystemClock;
    private task_move_to?: MoveTo;

    constructor(name: string, ship: RemoteShip, system_clock: SystemClock) {
        this.name = name;
        this.ship = ship;
        this.system_clock = system_clock;
    }

    interrupt(): void {
        this.task_move_to?.interrupt();
    }

    async move_to(position: Position, intercept_course = true): Promise<boolean> {
        this.interrupt();
        this.task_move_to = this.move_to_task(position, intercept_course);
        return await this.task_move_to.run();
    }

    private move_to_task(position: Position, intercept_course: boolean): MoveTo {
        return new MoveTo(
            this.ship,
            position,
            this.system_clock,
            intercept_course,
            `${this.name}/MovingTo`,
        );
    }
}
