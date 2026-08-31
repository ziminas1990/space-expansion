import type { Ship } from "./modules.js";
import {
    PhysicalResources,
    type ResourceAmounts,
    ResourceType,
} from "./resources.js";

export class Vector {
    constructor(
        public x: number,
        public y: number,
    ) {}

    verify(): void {
        if (!Number.isFinite(this.x) || !Number.isFinite(this.y)) {
            throw new Error("Vector coordinates must be finite numbers");
        }
    }

    toPod(): { x: number; y: number } {
        this.verify();
        return { x: this.x, y: this.y };
    }
}

export class Position {
    constructor(
        public x: number,
        public y: number,
        public velocity: Vector = new Vector(0, 0),
    ) {}

    setPosition(x: number, y: number, velocity = new Vector(0, 0)): this {
        this.x = x;
        this.y = y;
        this.velocity = velocity;
        return this;
    }

    verify(): void {
        if (!Number.isFinite(this.x) || !Number.isFinite(this.y)) {
            throw new Error("Position coordinates must be finite numbers");
        }
        this.velocity.verify();
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            position: { x: this.x, y: this.y },
            velocity: this.velocity.toPod(),
        };
    }
}

export interface AsteroidOptions {
    position: Position;
    radius: number;
    composition: ResourceAmounts;
}

export class Asteroid {
    position: Position;
    radius: number;
    readonly composition: PhysicalResources;

    constructor(options: AsteroidOptions) {
        this.position = options.position;
        this.radius = options.radius;
        this.composition = new PhysicalResources(options.composition);
    }

    setPosition(position: Position): this {
        this.position = position;
        return this;
    }

    setRadius(radius: number): this {
        this.radius = radius;
        return this;
    }

    setComposition(resource: ResourceType, density: number): this {
        this.composition.set(resource, density);
        return this;
    }

    verify(): void {
        this.position.verify();
        if (this.radius <= 5) {
            throw new Error("Asteroid radius must be greater than 5");
        }
        this.composition.verify();

        const density = Object.values(this.composition.resources).reduce(
            (sum, value) => sum + value,
            0,
        );
        if (density <= 0 || density > 100) {
            throw new Error(
                "Asteroid composition density must be greater than 0 and at most 100",
            );
        }
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            ...this.position.toPod(),
            radius: this.radius,
            ...this.composition.toPod(),
        };
    }
}

export class Asteroids {
    readonly asteroids: Asteroid[];

    constructor(asteroids: readonly Asteroid[] = []) {
        this.asteroids = [...asteroids];
    }

    addAsteroid(asteroid: Asteroid): this {
        this.asteroids.push(asteroid);
        return this;
    }

    verify(): void {
        for (const asteroid of this.asteroids) {
            asteroid.verify();
        }
    }

    toPod(): Array<Record<string, unknown>> {
        this.verify();
        return this.asteroids.map((asteroid) => asteroid.toPod());
    }
}

export class World {
    constructor(public asteroids: Asteroids | null = null) {}

    setAsteroids(asteroids: Asteroids): this {
        this.asteroids = asteroids;
        return this;
    }

    verify(): void {
        this.asteroids?.verify();
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return this.asteroids === null
            ? {}
            : { Asteroids: this.asteroids.toPod() };
    }
}

export interface PlayerOptions {
    login: string;
    password: string;
    ships?: readonly Ship[];
}

export class Player {
    login: string;
    password: string;
    readonly ships = new Map<string, Ship>();

    constructor(options: PlayerOptions) {
        this.login = options.login;
        this.password = options.password;
        for (const ship of options.ships ?? []) {
            this.addShip(ship.shipName, ship);
        }
    }

    setCredentials(login: string, password: string): this {
        this.login = login;
        this.password = password;
        return this;
    }

    addShip(name: string, ship: Ship): this {
        const key = `${ship.shipType}/${name}`;
        if (this.ships.has(key)) {
            throw new Error(`Ship '${key}' is already configured`);
        }
        this.ships.set(key, ship);
        return this;
    }

    verify(): void {
        if (this.login.length <= 4) {
            throw new Error("Player login must be longer than 4 characters");
        }
        if (this.password.length <= 4) {
            throw new Error("Player password must be longer than 4 characters");
        }
        for (const [name, ship] of this.ships) {
            if (name.length === 0) {
                throw new Error("Ship name cannot be empty");
            }
            ship.verify();
        }
    }

    toPod(): Record<string, unknown> {
        this.verify();
        return {
            password: this.password,
            ships: Object.fromEntries(
                [...this.ships].map(([name, ship]) => [name, ship.toPod()]),
            ),
        };
    }
}
