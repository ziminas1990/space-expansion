import {
    copy_position,
    copy_vector,
    pack_position,
    pack_vector,
    Position,
    unpack_position,
    unpack_vector,
    update_position,
    Vector2D,
} from "./position.js";

export type ShipUpdate = {
    position?: Position;
    orientation?: Vector2D;
}

export type ShipPacked = ReturnType<Ship["pack"]>;

export class Ship {

    outdated: boolean = false;
    private position: Position;
    private orientation: Vector2D | undefined;

    static unpack(packed: ShipPacked): Ship {
        const [id, position, outdated, orientation] = packed;
        const ship = new Ship(
            id,
            unpack_position(position),
            unpack_vector(orientation),
        );
        ship.outdated = outdated;
        return ship;
    }

    constructor(
        private readonly id: string,
        position: Position,
        orientation?: Vector2D,
    )
    {
        this.position = copy_position(position);
        this.orientation = orientation === undefined
            ? undefined
            : copy_vector(orientation);
    }

    get_id(): string {
        return this.id;
    }

    get_position(): Position {
        return copy_position(this.position);
    }

    get_orientation(): Vector2D | undefined {
        if (this.orientation === undefined) {
            return undefined;
        }
        return copy_vector(this.orientation);
    }

    update(update: ShipUpdate): void {
        if (update.position !== undefined
            && this.position.timestamp < update.position.timestamp)
        {
            this.position = update_position(this.position, update.position);
        }
        if (update.orientation !== undefined) {
            this.orientation = copy_vector(update.orientation);
        }
    }

    pack() {
        return [
            this.id,
            pack_position(this.position),
            this.outdated,
            pack_vector(this.orientation),
        ] as const;
    }

}
