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
import { ShipUpdate } from "./ship.js";

export type PlayerShipPacked = ReturnType<PlayerShip["pack"]>;

export class PlayerShip {

    outdated: boolean = false;
    private position: Position;
    private orientation: Orientation | undefined;

    static unpack(packed: PlayerShipPacked): PlayerShip {
        const [id, position, radius, outdated, orientation, blueprint_name] = packed;
        const ship = new PlayerShip(
            id,
            unpack_position(position),
            radius,
            blueprint_name,
        );
        ship.orientation = unpack_orientation(orientation);
        ship.outdated = outdated;
        return ship;
    }

    constructor(
        private readonly id: string,
        position: Position,
        private radius: number,
        private readonly blueprint_name: string,
        orientation?: Vector2D,
    ) {
        this.position = copy_position(position);
        this.orientation = orientation === undefined
            ? undefined
            : orientation_from(orientation, position.timestamp);
    }

    get_id(): string {
        return this.id;
    }

    get_blueprint_name(): string {
        return this.blueprint_name;
    }

    get_position(): Position {
        return copy_position(this.position);
    }

    get_radius(): number {
        return this.radius;
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
            this.radius,
            this.outdated,
            pack_orientation(this.orientation),
            this.blueprint_name,
        ] as const;
    }

}
