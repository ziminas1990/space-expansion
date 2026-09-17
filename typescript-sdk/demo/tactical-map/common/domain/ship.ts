import {
    copy_position,
    pack_position,
    Position,
    unpack_position,
    update_position,
} from "./position.js";

export type ShipUpdate = {
    position?: Position;
}

export type ShipPacked = ReturnType<Ship["pack"]>;

export class Ship {

    outdated: boolean = false;
    private position: Position;

    static unpack(packed: ShipPacked): Ship {
        const [id, position, outdated] = packed;
        const ship = new Ship(id, unpack_position(position));
        ship.outdated = outdated;
        return ship;
    }

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
            this.position = update_position(this.position, update.position);
        }
    }

    pack() {
        return [
            this.id,
            pack_position(this.position),
            this.outdated,
        ] as const;
    }

}
