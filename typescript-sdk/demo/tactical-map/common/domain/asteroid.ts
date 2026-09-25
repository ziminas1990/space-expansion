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

export type AsteroidUpdate = {
    position?: Position;
    radius?: number;
    orientation?: Vector2D;
}

export type AsteroidPacked = ReturnType<Asteroid["pack"]>;

export class Asteroid {

    outdated: boolean = false;
    private orientation: Vector2D | undefined;

    static unpack(packed: AsteroidPacked): Asteroid {
        const [id, position, radius, outdated, orientation] = packed;
        const asteroid = new Asteroid(
            id,
            unpack_position(position),
            radius,
            unpack_vector(orientation),
        );
        asteroid.outdated = outdated;
        return asteroid;
    }

    constructor(
        private readonly id: string,
        private position: Position,
        private radius: number,
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

    get_radius(): number {
        return this.radius;
    }

    get_orientation(): Vector2D | undefined {
        if (this.orientation === undefined) {
            return undefined;
        }
        return copy_vector(this.orientation);
    }

    update(update: AsteroidUpdate): void {
        if (update.position !== undefined
            && this.position.timestamp < update.position.timestamp)
        {
            this.position = update_position(this.position, update.position);
        }
        if (update.radius !== undefined) {
            this.radius = update.radius;
        }
        if (update.orientation !== undefined) {
            this.orientation = copy_vector(update.orientation);
        }
    }

    pack() {
        return [
            this.id,
            pack_position(this.position),
            this.radius,
            this.outdated,
            pack_vector(this.orientation),
        ] as const;
    }
}
