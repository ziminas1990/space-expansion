import { copy_position, Position } from "./position.js";

export type ShipUpdate = {
    position?: Position;
}

export class Ship {

    outdated: boolean = false;
    private position: Position;

    constructor(
        private readonly id: string,
        position: Position
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

    update(update: ShipUpdate): void {
        if (update.position !== undefined
            && this.position.timestamp < update.position.timestamp)
        {
            this.position = copy_position(update.position);
        }
    }

}
