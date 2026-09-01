import type { Position } from "../../types/common.js";
import type { ResourceItem, ResourceType } from "../../types/resources.js";
import type { Rect } from "./geometry.js";

export function makeResources(
    amounts: Partial<Record<ResourceType, number>>,
): ResourceItem[] {
    return Object.entries(amounts).flatMap(([resource_type, amount]) =>
        amount === undefined
            ? []
            : [{
                resource_type: resource_type as ResourceType,
                amount,
            }],
    );
}

export class Randomizer {
    constructor(private seed: number) {}

    randomValue(min: number, max: number): number {
        return min + (max - min) * this.next();
    }

    randomPosition(options: {
        rect?: Rect;
        center?: Position;
        radius?: number;
        minSpeed?: number;
        maxSpeed?: number;
    }): Position {
        const minSpeed = options.minSpeed ?? 0;
        const maxSpeed = options.maxSpeed ?? 0;
        const speed = this.randomValue(minSpeed, maxSpeed);
        const heading = this.randomValue(0, Math.PI * 2);
        const velocity: [number, number] = [
            speed * Math.cos(heading),
            speed * Math.sin(heading),
        ];

        if (options.rect !== undefined) {
            const { rect } = options;
            return {
                timestamp: 0n,
                point: [
                    this.randomValue(rect.left, rect.right),
                    this.randomValue(rect.bottom, rect.top),
                ],
                velocity,
            };
        }

        if (options.center === undefined || options.radius === undefined) {
            throw new Error(
                "randomPosition requires either rect or center and radius",
            );
        }

        const alfa = this.randomValue(0, Math.PI * 2);
        const r = Math.sqrt(this.randomValue(0, options.radius ** 2));
        return {
            timestamp: 0n,
            point: [
                options.center.point[0] + r * Math.cos(alfa),
                options.center.point[1] + r * Math.sin(alfa),
            ],
            velocity,
        };
    }

    shuffle<T>(items: T[]): T[] {
        for (let i = items.length - 1; i > 0; i -= 1) {
            const j = Math.floor(this.next() * (i + 1));
            const current = items[i]!;
            items[i] = items[j]!;
            items[j] = current;
        }
        return items;
    }

    // mulberry32: deterministic, independent of Math.random().
    private next(): number {
        this.seed |= 0;
        this.seed = (this.seed + 0x6D2B79F5) | 0;
        let t = Math.imul(this.seed ^ (this.seed >>> 15), 1 | this.seed);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    }
}
