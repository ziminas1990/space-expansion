import {
    apply_orientation,
    copy_orientation,
    orientation_from,
    Orientation,
    pack_orientation,
    unpack_orientation,
} from "./orientation.js";
import {
    copy_position,
    pack_position,
    Position,
    unpack_position,
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
    private orientation: Orientation | undefined;

    static unpack(packed: ShipPacked): Ship {
        const [id, position, outdated, orientation] = packed;
        const ship = new Ship(
            id,
            unpack_position(position),
        );
        ship.orientation = unpack_orientation(orientation);
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
            : orientation_from(orientation, position.timestamp);
    }

    get_id(): string {
        return this.id;
    }

    get_position(): Position {
        return copy_position(this.position);
    }

    get_orientation(): Orientation | undefined {
        if (this.orientation === undefined) {
            return undefined;
        }
        return copy_orientation(this.orientation);
    }

    update(update: ShipUpdate): void {
        if (update.position !== undefined
            && this.position.timestamp < update.position.timestamp)
        {
            this.position = update_position(this.position, update.position);
        }
        if (update.orientation !== undefined) {
            const timestamp = update.position?.timestamp ?? this.position.timestamp;
            this.orientation = apply_orientation(
                this.orientation,
                update.orientation,
                timestamp,
            );
        }
    }

    pack() {
        return [
            this.id,
            pack_position(this.position),
            this.outdated,
            pack_orientation(this.orientation),
        ] as const;
    }

}
