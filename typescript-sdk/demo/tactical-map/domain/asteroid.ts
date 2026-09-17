import { copy_position, Position } from "./position.js";

export type AsteroidUpdate = {
    position?: Position;
    radius?: number;
}

export class Asteroid {

    outdated: boolean = false;

    constructor(
        private readonly id: string,
        private position: Position,
        private radius: number
    )
    {
        this.position = copy_position(position);
    }

    get_id(): string {
        return this.id;
    }

    get_position(): Position {
        return copy_position(this.position);
    }

    get_radius(): number {
        return this.radius;
    }

    update(update: AsteroidUpdate): void {
        if (update.position !== undefined
            && this.position.timestamp < update.position.timestamp)
        {
            this.position = copy_position(update.position);
        }
        if (update.radius !== undefined) {
            this.radius = update.radius;
        }
    }

}
