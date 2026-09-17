import { copy_position, pack_position, Position, unpack_position } from "./position.js";

export type AsteroidUpdate = {
    position?: Position;
    radius?: number;
}

export type AsteroidPacked = ReturnType<Asteroid["pack"]>;

export class Asteroid {

    outdated: boolean = false;

    static unpack(packed: AsteroidPacked): Asteroid {
        const [id, position, radius, outdated] = packed;
        const asteroid = new Asteroid(id, unpack_position(position), radius);
        asteroid.outdated = outdated;
        return asteroid;
    }

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

    pack() {
        return [
            this.id,
            pack_position(this.position),
            this.radius,
            this.outdated,
        ] as const;
    }
}
